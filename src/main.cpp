#include "timesync/timesync.hpp"

#include <metricq/logger/nitro.hpp>
#include <metricq/metadata.hpp>
#include <metricq/ostream.hpp>
#include <metricq/simple.hpp>
#include <metricq/simple_drain.hpp>
#include <metricq/types.hpp>

#include <scorep/plugin/plugin.hpp>

#include <nitro/lang/enumerate.hpp>

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include <cstdint>

using namespace scorep::plugin::policy;

using Log = metricq::logger::nitro::Log;

// Must be system clock for real epoch!
using local_clock = std::chrono::system_clock;

template <typename T, typename V>
std::vector<T> keys(std::map<T, V>& map)
{
    std::vector<T> ret;
    ret.reserve(map.size());
    for (auto const& elem : map)
    {
        ret.push_back(elem.first);
    }
    return ret;
};

struct Metric
{
    std::string name;
    bool use_timesync;
};

void replace_all(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

template <typename T, typename Policies>
using handle_oid_policy = object_id<Metric, T, Policies>;

class metricq_plugin : public scorep::plugin::base<metricq_plugin, async, once, post_mortem,
                                                   scorep_clock, handle_oid_policy>
{
public:
    metricq_plugin()
    : url_(scorep::environment_variable::get("SERVER")),
      token_(scorep::environment_variable::get("TOKEN", "scorepPlugin")),
      average_(std::stoi(scorep::environment_variable::get("AVERAGE", "0")))
    {
        metricq::logger::nitro::initialize();
    }

private:
    auto get_metadata(const std::string& s)
    {
        std::string selector = s;
        bool is_regex = (selector.find("*") != std::string::npos);

        if (!is_regex)
        {
            return metricq::get_metadata(url_, token_, std::vector<std::string>({ selector }));
        }

        replace_all(selector, ".", "\\.");
        replace_all(selector, "*", ".*");
        return metricq::get_metadata(url_, token_, selector);
    }

public:
    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& s)
    {
        auto metadata = get_metadata(s);
        std::vector<scorep::plugin::metric_property> result;
        for (const auto& elem : metadata)
        {
            const auto& name = elem.first;
            const auto& meta = elem.second;
            auto use_timesync = !std::isnan(meta.rate()) and meta.rate() > 1000;
            make_handle(name, Metric{ name, use_timesync });

            auto property = scorep::plugin::metric_property(name, meta.description(), meta.unit())
                                .value_double();

            if (average_)
            {
                property.absolute_last();
            }
            else
            {
                switch (meta.scope())
                {
                case metricq::Metadata::Scope::last:
                    property.absolute_last();
                    break;
                case metricq::Metadata::Scope::next:
                    property.absolute_next();
                    break;
                case metricq::Metadata::Scope::point:
                    property.absolute_point();
                    break;
                case metricq::Metadata::Scope::unknown:
                    property.absolute_point();
                    break;
                }
            }

            result.push_back(property);
        }

        return result;
    }

    void add_metric(Metric& metric)
    {
        metrics_.push_back(metric.name);
    }

    void start()
    {
        convert_.synchronize_point();
        auto timeout_str = scorep::environment_variable::get("TIMEOUT");
        auto timeout = metricq::duration_parse(timeout_str);
        if (timeout_str.empty())
        {
            Log::warn()
                << "No timeout specified, using 1 hour.\n"
                << "Please always specify " << scorep::environment_variable::name("TIMEOUT")
                << " (in seconds)\n"
                << "If your program runs longer than this, measurements will not be available.";
            timeout = std::chrono::hours(1);
        }
        else
        {
            try
            {
                timeout = std::chrono::seconds(std::stoll(timeout_str));
                if (timeout.count() <= 0)
                {
                    throw std::out_of_range("");
                }
            }
            catch (std::logic_error&)
            {
                Log::error() << "Invalid timeout specified in "
                             << scorep::environment_variable::name("TIMEOUT") << ", using 1 hour.";
                timeout = std::chrono::hours(1);
            }
        }
        queue_ = metricq::subscribe(url_, token_, metrics_, timeout);

        cc_time_sync_.sync_begin();
    }

    void stop()
    {
        convert_.synchronize_point();
        cc_time_sync_.sync_end();

        data_drain_ = std::make_unique<metricq::SimpleDrain>(token_, queue_);
        data_drain_->add(metrics_);
        data_drain_->connect(url_);
        Log::debug() << "starting data drain main loop.";
        data_drain_->main_loop();
        Log::debug() << "finished data drain main loop.";

        for (auto& metric : get_handles())
        {
            // XXX sync with first metric
            if (metric.use_timesync && !cc_synced_)
            {
                try
                {
                    Log::debug() << "Trying timesync with metric: " << metric.name;
                    auto& data = data_drain_->at(metric.name);

                    cc_time_sync_.find_offsets(data);
                    cc_synced_ = true;
                }
                catch (std::exception& e)
                {
                    Log::warn() << "Timesync failed with error: " << e.what();
                }
            }
        }
    }

    scorep::chrono::ticks convert_time_(metricq::TimePoint time, Metric& metric)
    {
        if (metric.use_timesync)
        {
            return convert_.to_ticks(cc_time_sync_.to_local(time));
        }
        else
        {
            return convert_.to_ticks(time);
        }
    }

    template <class Cursor>
    void get_all_values(Metric& metric, Cursor& c)
    {
        auto& data = data_drain_->at(metric.name);
        if (data.empty())
        {
            Log::error() << "no measurement data recorded for " << metric.name;
            return;
        }

        if (average_)
        {
            int count = 0;
            double sum = 0.;
            for (auto& tv : data)
            {
                sum += tv.value;
                count++;
                if (count == average_)
                {
                    c.write(convert_time_(tv.time, metric), sum / average_);
                    count = 0;
                    sum = 0.;
                }
            }
        }
        else
        {
            for (auto& tv : data)
            {
                c.write(convert_time_(tv.time, metric), tv.value);
            }
        }
    }

private:
    int average_;
    bool cc_synced_ = false;
    std::vector<std::string> metrics_;
    std::string url_;
    std::string token_;
    std::string queue_;
    std::map<std::string, std::vector<metricq::TimeValue>> metric_data_;
    scorep::chrono::time_convert<> convert_;
    timesync::CCTimeSync cc_time_sync_;
    std::unique_ptr<metricq::SimpleDrain> data_drain_;
};

SCOREP_METRIC_PLUGIN_CLASS(metricq_plugin, "metricq")

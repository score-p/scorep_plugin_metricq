#include "log.hpp"
#include "subscription_sink.hpp"
#include "unsubscription_sink.hpp"

#include <dataheap2/ostream.hpp>
#include <dataheap2/types.hpp>

#include <scorep/plugin/plugin.hpp>

#include <nitro/lang/enumerate.hpp>

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace scorep::plugin::policy;

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

class ScorepUnsubscriptionSink : public UnsubscriptionSink
{
public:
    ScorepUnsubscriptionSink(const std::string& manager_host, const std::string& token,
                             const std::string& queue,
                             std::map<std::string, std::vector<dataheap2::TimeValue>>& data)
    : UnsubscriptionSink(manager_host, token, queue, keys(data)), data_(data)
    {
    }

    void data_callback(const std::string& id, const dataheap2::DataChunk& chunk) override
    {
        auto& metric_data = data_[id];
        for (const auto& tv : chunk)
        {
            metric_data.push_back(tv);
        }
    }

private:
    std::map<std::string, std::vector<dataheap2::TimeValue>>& data_;
};

struct Metric
{
    std::string name;
};

template <typename T, typename Policies>
using handle_oid_policy = object_id<Metric, T, Policies>;

class dataheap2_plugin : public scorep::plugin::base<dataheap2_plugin, async, once, post_mortem,
                                                     scorep_clock, handle_oid_policy>
{
public:
    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& name)
    {
        make_handle(name, Metric{ name });
        return { scorep::plugin::metric_property(name).absolute_point().value_double() };
    }

    void add_metric(Metric& metric)
    {
        metric_data_[metric.name];
        metrics_.push_back(metric.name);
    }

    void start()
    {
        convert_.synchronize_point();
        SubscriptionSink sink(scorep::environment_variable::get("SERVER"),
                              scorep::environment_variable::get("TOKEN", "scorepPlugin"),
                              keys(metric_data_));
        sink.main_loop();
        queue_ = sink.queue_name;
    }

    void stop()
    {
        convert_.synchronize_point();
        ScorepUnsubscriptionSink sink(scorep::environment_variable::get("SERVER"),
                                      scorep::environment_variable::get("TOKEN", "scorepPlugin"),
                                      queue_, metric_data_);
        sink.main_loop();
    }

    template <class Cursor>
    void get_all_values(Metric& metric, Cursor& c)
    {
        for (auto& tv : metric_data_.at(metric.name))
        {
            c.write(convert_.to_ticks(tv.time), tv.value);
        }
    }

private:
    std::vector<std::string> metrics_;
    std::string queue_;
    std::map<std::string, std::vector<dataheap2::TimeValue>> metric_data_;
    scorep::chrono::time_convert<> convert_;
};

SCOREP_METRIC_PLUGIN_CLASS(dataheap2_plugin, "dataheap2")

#include "log.hpp"

#include <dataheap2/ostream.hpp>
#include <dataheap2/simple.hpp>
#include <dataheap2/simple_drain.hpp>
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
        metrics_.push_back(metric.name);
    }

    void start()
    {
        convert_.synchronize_point();
        queue_ = dataheap2::subscribe(scorep::environment_variable::get("SERVER"),
                                      scorep::environment_variable::get("TOKEN", "scorepPlugin"),
                                      metrics_);
    }

    void stop()
    {
        convert_.synchronize_point();
        data_drain_ = std::make_unique<dataheap2::SimpleDrain>(
            scorep::environment_variable::get("TOKEN", "scorepPlugin"), queue_);
        data_drain_->add(metrics_);
        data_drain_->connect(scorep::environment_variable::get("SERVER"));
        Log::debug() << "starting data drain main loop.";
        data_drain_->main_loop();
        Log::debug() << "finished data drain main loop.";
    }

    template <class Cursor>
    void get_all_values(Metric& metric, Cursor& c)
    {
        auto& data = data_drain_->at(metric.name);
        for (auto& tv : data)
        {
            c.write(convert_.to_ticks(tv.time), tv.value);
        }
    }

private:
    std::vector<std::string> metrics_;
    std::string queue_;
    std::map<std::string, std::vector<dataheap2::TimeValue>> metric_data_;
    scorep::chrono::time_convert<> convert_;
    std::unique_ptr<dataheap2::SimpleDrain> data_drain_;
};

SCOREP_METRIC_PLUGIN_CLASS(dataheap2_plugin, "dataheap2")

#pragma once

#include <dataheap2/sink.hpp>
#include <dataheap2/types.hpp>

#include <nlohmann/json_fwd.hpp>

#include <string>

using json = nlohmann::json;

class SubscriptionSink : public dataheap2::Sink
{
public:
    SubscriptionSink(const std::string& manager_host, const std::string& token);

    std::string queue_name;

private:
    void config_callback(const json& config) override;
    void sink_config_callback(const json& config) override;
    void ready_callback() override;
    void data_callback(const std::string& id, dataheap2::TimeValue tv) override;
};

#include "subscription_sink.hpp"
#include "log.hpp"

SubscriptionSink::SubscriptionSink(const std::string& manager_host, const std::string& token)
        : Sink(token)
{
    register_management_callback("subscribed", [this](auto& response) {
        Log::debug() << "subscribed: " << response.dump();
        queue_name = response["dataQueue"];
        close();
    });
    connect(manager_host);
}

void SubscriptionSink::sink_config_callback(const json& config)
{
}

void SubscriptionSink::config_callback(const json& config)
{
    send_management("subscribe", { { "metrics", { "dataDrop" } } });
}

void SubscriptionSink::ready_callback()
{
}

void SubscriptionSink::data_callback(const std::string&, dataheap2::TimeValue)
{
}
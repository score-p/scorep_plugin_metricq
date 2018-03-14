#pragma once

#include <dataheap2/sink.hpp>
#include <dataheap2/types.hpp>

#include <nlohmann/json_fwd.hpp>

#include <string>

using json = nlohmann::json;

class UnsubscriptionSink : public dataheap2::Sink
{
public:
    UnsubscriptionSink(const std::string& manager_host, const std::string& token,
                       const std::string& queue);
    ~UnsubscriptionSink();

private:
    void config_callback(const json& config) override;
    void sink_config_callback(const json& config) override;
    void ready_callback() override;

    uint64_t message_count;
};

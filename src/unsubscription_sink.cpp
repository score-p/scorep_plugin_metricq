#include "unsubscription_sink.hpp"
#include "log.hpp"

UnsubscriptionSink::UnsubscriptionSink(const std::string& manager_host, const std::string& token,
                                       const std::string& queue,
                                       const std::vector<std::string>& routing_keys)
: Sink(token), routing_keys_(routing_keys)
{
    data_queue_ = queue;
    connect(manager_host);
}

UnsubscriptionSink::~UnsubscriptionSink()
{
}

void UnsubscriptionSink::sink_config_callback(const json& config)
{
}

void UnsubscriptionSink::config_callback(const json& config)
{
    send_management("unsubscribe", { { "metrics", routing_keys_ }, { "dataQueue", data_queue_ } });

    const std::string& data_server_address_ = config["dataServerAddress"];
    data_connection_ =
        std::make_unique<AMQP::TcpConnection>(&handler, AMQP::Address(data_server_address_));
    data_channel_ = std::make_unique<AMQP::TcpChannel>(data_connection_.get());
    data_channel_->onError(
        [](const char* message) { std::cerr << "data channel error: " << message << std::endl; });
    data_channel_->declareQueue(data_queue_)
        .onSuccess([this](const std::string& name, int msgcount, int consumercount) {
            std::cerr << "setting up sink queue. msgcount: " << msgcount << ", consumercount"
                      << consumercount << std::endl;
            message_count = msgcount;

            if (message_count == 0)
            {
                std::cout << "NO data in our queue :-(" << std::endl;
                send_management("release", { { "dataQueue", data_queue_ } });
                stop();
                return;
            }

            auto start_cb = [](const std::string& consumertag) {
                std::cerr << "data consume operation started: " << consumertag << std::endl;
            };

            auto error_cb = [](const char* message) {
                std::cerr << "data consume operation failed: " << message << std::endl;
            };

            auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                     bool redelivered) {
                (void)redelivered;
                message_count--;
                data_callback(message);
                data_channel_->ack(deliveryTag);
                if (message_count == 0)
                {
                    send_management("release", { { "dataQueue", data_queue_ } });
                    stop();
                }
            };

            data_channel_->consume(name)
                .onReceived(message_cb)
                .onSuccess(start_cb)
                .onError(error_cb);
        });
    if (config.find("sinkConfig") != config.end())
    {
        sink_config_callback(config["sinkConfig"]);
    }
    ready_callback();
}

void UnsubscriptionSink::ready_callback()
{
}

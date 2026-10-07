#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace solix::lsp {

class JsonRpcTransport {
public:
    JsonRpcTransport(std::istream& in, std::ostream& out);

    // Reads the next JSON-RPC message from std::istream (blocking until a message is received or EOF)
    std::optional<nlohmann::json> read_message();

    // Writes a JSON-RPC message with Content-Length header framing to std::ostream
    void write_message(const nlohmann::json& message);

    // Helper for sending a response
    void send_response(const nlohmann::json& id, const nlohmann::json& result);

    // Helper for sending an error response
    void send_error(const nlohmann::json& id, int code, const std::string& message, const nlohmann::json& data = nullptr);

    // Helper for sending a notification
    void send_notification(const std::string& method, const nlohmann::json& params);

private:
    std::istream& in_;
    std::ostream& out_;
};

} // namespace solix::lsp

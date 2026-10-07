#include "solix/lsp/transport.hpp"
#include <sstream>

namespace solix::lsp {

JsonRpcTransport::JsonRpcTransport(std::istream& in, std::ostream& out)
    : in_(in), out_(out) {}

std::optional<nlohmann::json> JsonRpcTransport::read_message() {
    in_.clear();
    size_t content_length = 0;
    std::string line;

    // Read HTTP/MIME-style headers until empty line "\r" or ""
    while (std::getline(in_, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break; // Header section complete
        }

        const std::string cl_prefix = "Content-Length: ";
        if (line.rfind(cl_prefix, 0) == 0) {
            try {
                content_length = std::stoull(line.substr(cl_prefix.length()));
            } catch (...) {
                content_length = 0;
            }
        }
    }

    if (!in_ || content_length == 0) {
        return std::nullopt;
    }

    std::string body;
    body.resize(content_length);
    in_.read(&body[0], content_length);

    if (in_.gcount() < static_cast<std::streamsize>(content_length)) {
        return std::nullopt;
    }

    try {
        return nlohmann::json::parse(body);
    } catch (...) {
        return std::nullopt;
    }
}

void JsonRpcTransport::write_message(const nlohmann::json& message) {
    out_.clear();
    std::string body = message.dump();
    out_ << "Content-Length: " << body.size() << "\r\n\r\n" << body;
    out_.flush();
}

void JsonRpcTransport::send_response(const nlohmann::json& id, const nlohmann::json& result) {
    nlohmann::json resp = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };
    write_message(resp);
}

void JsonRpcTransport::send_error(const nlohmann::json& id, int code, const std::string& message, const nlohmann::json& data) {
    nlohmann::json err = {
        {"code", code},
        {"message", message}
    };
    if (!data.is_null()) {
        err["data"] = data;
    }
    nlohmann::json resp = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", err}
    };
    write_message(resp);
}

void JsonRpcTransport::send_notification(const std::string& method, const nlohmann::json& params) {
    nlohmann::json notif = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    };
    write_message(notif);
}

} // namespace solix::lsp

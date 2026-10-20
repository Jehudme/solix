#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>

namespace solix::lsp {

class DocumentStore {
public:
    DocumentStore() = default;

    void open_document(const std::string& uri, int version, const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);
        docs_[uri] = Entry{version, text};
    }

    void update_document(const std::string& uri, int version, const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);
        docs_[uri] = Entry{version, text};
    }

    void close_document(const std::string& uri) {
        std::lock_guard<std::mutex> lock(mutex_);
        docs_.erase(uri);
    }

    std::optional<std::string> get_document_text(const std::string& uri) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = docs_.find(uri);
        if (it != docs_.end()) {
            return it->second.text;
        }
        return std::nullopt;
    }

    std::optional<int> get_document_version(const std::string& uri) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = docs_.find(uri);
        if (it != docs_.end()) {
            return it->second.version;
        }
        return std::nullopt;
    }

    std::unordered_map<std::string, std::string> get_all_open_documents() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_map<std::string, std::string> res;
        for (const auto& [uri, entry] : docs_) {
            res[uri] = entry.text;
        }
        return res;
    }

private:
    struct Entry {
        int version{0};
        std::string text;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Entry> docs_;
};

} // namespace solix::lsp

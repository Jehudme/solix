#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace solix::lsp {

struct Position {
    int line{0};      // 0-based
    int character{0}; // 0-based

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, line, character)
};

struct Range {
    Position start;
    Position end;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Range, start, end)
};

struct Location {
    std::string uri;
    Range range;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Location, uri, range)
};

enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

struct DiagnosticRelatedInformation {
    Location location;
    std::string message;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DiagnosticRelatedInformation, location, message)
};

struct Diagnostic {
    Range range;
    int severity{1}; // 1 = Error, 2 = Warning
    std::string code;
    std::string source{"solix"};
    std::string message;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Diagnostic, range, severity, code, source, message)
};

struct PublishDiagnosticsParams {
    std::string uri;
    std::vector<Diagnostic> diagnostics;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PublishDiagnosticsParams, uri, diagnostics)
};

struct TextDocumentItem {
    std::string uri;
    std::string languageId;
    int version{0};
    std::string text;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentItem, uri, languageId, version, text)
};

struct VersionedTextDocumentIdentifier {
    std::string uri;
    int version{0};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VersionedTextDocumentIdentifier, uri, version)
};

struct TextDocumentContentChangeEvent {
    // For full document sync (TextDocumentSyncKind::Full), range is omitted and text is full content
    std::string text;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentContentChangeEvent, text)
};

// Converts file:// URI to std::filesystem::path or local path string
std::string uri_to_path(const std::string& uri);

// Converts local path string to file:// URI
std::string path_to_uri(const std::string& path);

} // namespace solix::lsp

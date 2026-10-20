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

struct MarkupContent {
    std::string kind{"markdown"}; // "plaintext" or "markdown"
    std::string value;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MarkupContent, kind, value)
};

struct Hover {
    MarkupContent contents;
    std::optional<Range> range;
};

inline void to_json(nlohmann::json& j, const Hover& h) {
    j = nlohmann::json{{"contents", h.contents}};
    if (h.range.has_value()) {
        j["range"] = h.range.value();
    }
}

enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};

struct CompletionItem {
    std::string label;
    int kind{1};
    std::string detail;
    std::string documentation;
    std::string insertText;
};

inline void to_json(nlohmann::json& j, const CompletionItem& ci) {
    j = nlohmann::json{
        {"label", ci.label},
        {"kind", ci.kind}
    };
    if (!ci.detail.empty()) j["detail"] = ci.detail;
    if (!ci.documentation.empty()) j["documentation"] = ci.documentation;
    if (!ci.insertText.empty()) j["insertText"] = ci.insertText;
}

struct CompletionList {
    bool isIncomplete{false};
    std::vector<CompletionItem> items;
};

inline void to_json(nlohmann::json& j, const CompletionList& cl) {
    j = nlohmann::json{
        {"isIncomplete", cl.isIncomplete},
        {"items", cl.items}
    };
}

struct ParameterInformation {
    std::string label;
    std::string documentation;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ParameterInformation, label, documentation)
};

struct SignatureInformation {
    std::string label;
    std::string documentation;
    std::vector<ParameterInformation> parameters;
    int activeParameter{0};
};

inline void to_json(nlohmann::json& j, const SignatureInformation& si) {
    j = nlohmann::json{
        {"label", si.label},
        {"parameters", si.parameters}
    };
    if (!si.documentation.empty()) j["documentation"] = si.documentation;
    if (si.activeParameter >= 0) j["activeParameter"] = si.activeParameter;
}

struct SignatureHelp {
    std::vector<SignatureInformation> signatures;
    int activeSignature{0};
    int activeParameter{0};
};

inline void to_json(nlohmann::json& j, const SignatureHelp& sh) {
    j = nlohmann::json{
        {"signatures", sh.signatures},
        {"activeSignature", sh.activeSignature},
        {"activeParameter", sh.activeParameter}
    };
}

enum class SymbolKind {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26
};

struct DocumentSymbol {
    std::string name;
    std::string detail;
    int kind{1};
    Range range;
    Range selectionRange;
    std::vector<DocumentSymbol> children;
};

inline void to_json(nlohmann::json& j, const DocumentSymbol& s) {
    j = nlohmann::json{
        {"name", s.name},
        {"kind", s.kind},
        {"range", s.range},
        {"selectionRange", s.selectionRange}
    };
    if (!s.detail.empty()) j["detail"] = s.detail;
    if (!s.children.empty()) {
        j["children"] = s.children;
    } else {
        j["children"] = nlohmann::json::array();
    }
}

// Converts file:// URI to std::filesystem::path or local path string
std::string uri_to_path(const std::string& uri);

// Converts local path string to file:// URI
std::string path_to_uri(const std::string& path);

} // namespace solix::lsp

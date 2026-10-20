#pragma once

#include "utilities/statements.hpp"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace solix::lsp {

class AstSpatialIndex {
public:
    AstSpatialIndex() = default;

    // Index all AST roots for a source path
    void index_source(const std::string& source_path, const std::vector<std::unique_ptr<Node>>& nodes);

    // Find the most specific (deepest) AST node spanning (line, col) (1-indexed)
    Node* find_node_at(const std::string& source_path, uint32_t line, uint32_t col) const;

    // Clear index for all or specific source
    void clear();
    void clear_source(const std::string& source_path);

private:
    void traverse_and_index(const std::string& source_path, Node* node);

    std::unordered_map<std::string, std::vector<Node*>> source_nodes_;
};

} // namespace solix::lsp

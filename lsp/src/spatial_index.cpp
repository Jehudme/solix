#include "solix/lsp/spatial_index.hpp"
#include "utilities/ast_visitor.hpp"
#include <filesystem>

namespace solix::lsp {

namespace {

// Helper to check if (line, col) is within [start, end]
bool contains_position(Node* node, uint32_t line, uint32_t col) {
    if (!node || node->line == 0) return false;

    uint32_t start_line = node->line;
    uint32_t start_col = node->column;
    uint32_t end_line = (node->end_line >= start_line) ? node->end_line : start_line;
    uint32_t end_col = (node->end_column > 0) ? node->end_column : (start_col + 1);

    if (line < start_line || line > end_line) return false;
    if (line == start_line && col < start_col) return false;
    if (line == end_line && col > end_col) return false;

    return true;
}

// Compute span area to prefer smallest / most specific node
uint64_t compute_span_size(Node* node) {
    if (!node) return UINT64_MAX;
    uint32_t start_line = node->line;
    uint32_t start_col = node->column;
    uint32_t end_line = (node->end_line >= start_line) ? node->end_line : start_line;
    uint32_t end_col = (node->end_column > 0) ? node->end_column : (start_col + 1);

    uint64_t line_diff = end_line - start_line;
    uint64_t col_diff = (end_col >= start_col) ? (end_col - start_col) : 1;
    return line_diff * 100000 + col_diff;
}

class AstCollectorVisitor : public NodeVisitor {
public:
    std::vector<Node*>& out_list;
    AstCollectorVisitor(std::vector<Node*>& list) : out_list(list) {}

    void collect(Node* n) {
        if (!n) return;
        out_list.push_back(n);
        n->accept(*this);
    }

    void visit(IdentifierNode& n) override {}
    void visit(LiteralNode& n) override {}

    void visit(BinaryExpression& n) override {
        collect(n.left.get());
        collect(n.right.get());
    }

    void visit(UnaryExpression& n) override {
        collect(n.operand.get());
    }

    void visit(AssignmentExpression& n) override {
        collect(n.target.get());
        collect(n.value.get());
    }

    void visit(ArrayAccessExpression& n) override {
        collect(n.array.get());
        collect(n.index.get());
    }

    void visit(MemberAccessExpression& n) override {
        collect(n.object.get());
    }

    void visit(MethodCallExpression& n) override {
        collect(n.callee.get());
        for (const auto& arg : n.arguments) collect(arg.get());
    }

    void visit(NewInstanceExpression& n) override {
        for (const auto& arg : n.arguments) collect(arg.get());
    }

    void visit(ArrayCreationExpression& n) override {
        collect(n.size.get());
    }

    void visit(ArrayLiteralExpression& n) override {
        for (const auto& el : n.elements) collect(el.get());
    }

    void visit(CastExpression& n) override {
        collect(n.expression.get());
    }

    void visit(InstanceofExpression& n) override {
        collect(n.expression.get());
    }

    void visit(TernaryExpression& n) override {
        collect(n.condition.get());
        collect(n.true_branch.get());
        collect(n.false_branch.get());
    }

    void visit(SizeOfExpression& n) override {
        collect(n.target_expr.get());
    }

    void visit(DefaultExpression& n) override {}

    void visit(LambdaExpression& n) override {
        for (const auto& p : n.parameters) collect(p.get());
        collect(n.body.get());
    }

    void visit(BlockStatement& n) override {
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(IfStatement& n) override {
        collect(n.condition.get());
        collect(n.then_branch.get());
        collect(n.else_branch.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(ForStatement& n) override {
        collect(n.initialization.get());
        collect(n.condition.get());
        collect(n.iteration.get());
        collect(n.body.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(WhileStatement& n) override {
        collect(n.condition.get());
        collect(n.body.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(DoWhileStatement& n) override {
        collect(n.body.get());
        collect(n.condition.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(SwitchStatement& n) override {
        collect(n.condition.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(CaseStatement& n) override {
        collect(n.case_value.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(TryStatement& n) override {
        collect(n.try_block.get());
        for (const auto& c : n.catch_clauses) collect(c.get());
        collect(n.finally_block.get());
    }

    void visit(CatchClause& n) override {
        collect(n.body.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(ThrowStatement& n) override {
        collect(n.exception_expression.get());
    }

    void visit(VariableDeclaration& n) override {
        collect(n.initializer.get());
    }

    void visit(ExpressionStatement& n) override {
        collect(n.expression.get());
    }

    void visit(ReturnStatement& n) override {
        collect(n.value.get());
    }

    void visit(BreakStatement& n) override {}
    void visit(ContinueStatement& n) override {}

    void visit(AssertStatement& n) override {
        collect(n.condition.get());
        collect(n.message.get());
    }

    void visit(ExitStatement& n) override {
        collect(n.exit_code.get());
    }

    void visit(PackageStatement& n) override {}
    void visit(AliasStatement& n) override {}
    void visit(ImportStatement& n) override {}

    void visit(EnumDeclaration& n) override {}

    void visit(ClassDeclaration& n) override {
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(FieldDeclaration& n) override {
        collect(n.initializer.get());
    }

    void visit(ConstructorDeclaration& n) override {
        for (const auto& p : n.parameters) collect(p.get());
        for (const auto& ch : n.children) collect(ch.get());
    }

    void visit(MethodDeclaration& n) override {
        for (const auto& p : n.parameters) collect(p.get());
        for (const auto& ch : n.children) collect(ch.get());
    }
};

} // namespace

void AstSpatialIndex::index_source(const std::string& source_path, const std::vector<std::unique_ptr<Node>>& nodes) {
    std::string norm_path = std::filesystem::path(source_path).lexically_normal().string();
    auto& list = source_nodes_[norm_path];
    list.clear();

    AstCollectorVisitor visitor(list);
    for (const auto& root : nodes) {
        visitor.collect(root.get());
    }
}

Node* AstSpatialIndex::find_node_at(const std::string& source_path, uint32_t line, uint32_t col) const {
    std::string norm_path = std::filesystem::path(source_path).lexically_normal().string();
    auto it = source_nodes_.find(norm_path);
    if (it == source_nodes_.end()) {
        return nullptr;
    }

    Node* best_node = nullptr;
    uint64_t best_size = UINT64_MAX;

    for (Node* n : it->second) {
        if (contains_position(n, line, col)) {
            uint64_t sz = compute_span_size(n);
            if (sz <= best_size) {
                best_size = sz;
                best_node = n;
            }
        }
    }

    return best_node;
}

void AstSpatialIndex::clear() {
    source_nodes_.clear();
}

void AstSpatialIndex::clear_source(const std::string& source_path) {
    std::string norm_path = std::filesystem::path(source_path).lexically_normal().string();
    source_nodes_.erase(norm_path);
}

} // namespace solix::lsp

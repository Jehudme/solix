#pragma once
#include "utilities/ast_visitor.hpp"
#include "utilities/statements.hpp"
#include <unordered_map>
#include <string>

namespace solix {

class TemplateSubstitutionVisitor : public NodeVisitor {
public:
    TemplateSubstitutionVisitor(const std::unordered_map<std::string, TypeInfo>& substitutions)
        : type_substitutions(substitutions) {}

    void execute(Node* root) {
        if (root) root->accept(*this);
    }

    void visit(IdentifierNode& n) override {}
    void visit(LiteralNode& n) override {}
    void visit(BinaryExpression& n) override;
    void visit(UnaryExpression& n) override;
    void visit(AssignmentExpression& n) override;
    void visit(ArrayAccessExpression& n) override;
    void visit(MemberAccessExpression& n) override;
    void visit(MethodCallExpression& n) override;
    void visit(NewInstanceExpression& n) override;
    void visit(ArrayCreationExpression& n) override;
    void visit(ArrayLiteralExpression& n) override;
    void visit(CastExpression& n) override;
    void visit(InstanceofExpression& n) override;
    void visit(TernaryExpression& n) override;

    void visit(BlockStatement& n) override;
    void visit(IfStatement& n) override;
    void visit(ForStatement& n) override;
    void visit(WhileStatement& n) override;
    void visit(DoWhileStatement& n) override;
    void visit(SwitchStatement& n) override;
    void visit(CaseStatement& n) override;
    void visit(VariableDeclaration& n) override;
    void visit(ExpressionStatement& n) override;
    void visit(ReturnStatement& n) override;
    void visit(BreakStatement& n) override;
    void visit(ContinueStatement& n) override;
    void visit(PackageStatement& n) override {}
    void visit(AliasStatement& n) override;

    void visit(EnumDeclaration& n) override {}
    void visit(ClassDeclaration& n) override;
    void visit(FieldDeclaration& n) override;
    void visit(ConstructorDeclaration& n) override;
    void visit(MethodDeclaration& n) override;

private:
    std::unordered_map<std::string, TypeInfo> type_substitutions;

    void substitute_type(TypeInfo& type) {
        if (type_substitutions.count(type.name)) {
            int original_depth = type.array_depth;
            type = type_substitutions.at(type.name);
            type.array_depth += original_depth;
        }
        for (auto& arg : type.type_args) {
            substitute_type(arg);
        }
    }
};

} // namespace solix

#pragma once

namespace solix {

struct IdentifierNode;
struct LiteralNode;
struct BinaryExpression;
struct UnaryExpression;
struct AssignmentExpression;
struct ArrayAccessExpression;
struct MemberAccessExpression;
struct MethodCallExpression;
struct NewInstanceExpression;
struct ArrayCreationExpression;
struct ArrayLiteralExpression;
struct CastExpression;
struct InstanceofExpression;
struct TernaryExpression;

struct BlockStatement;
struct IfStatement;
struct ForStatement;
struct WhileStatement;
struct DoWhileStatement;
struct SwitchStatement;
struct CaseStatement;
struct TryStatement;
struct CatchClause;
struct ThrowStatement;
struct VariableDeclaration;
struct ExpressionStatement;
struct ReturnStatement;
struct BreakStatement;
struct ContinueStatement;
struct PackageStatement;
struct AliasStatement;
struct ImportStatement;

struct EnumDeclaration;
struct ClassDeclaration;
struct FieldDeclaration;
struct ConstructorDeclaration;
struct MethodDeclaration;

struct NodeVisitor {
    virtual ~NodeVisitor() = default;

    virtual void visit(IdentifierNode& node) = 0;
    virtual void visit(LiteralNode& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(ArrayAccessExpression& node) = 0;
    virtual void visit(MemberAccessExpression& node) = 0;
    virtual void visit(MethodCallExpression& node) = 0;
    virtual void visit(NewInstanceExpression& node) = 0;
    virtual void visit(ArrayCreationExpression& node) = 0;
    virtual void visit(ArrayLiteralExpression& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(InstanceofExpression& node) = 0;
    virtual void visit(TernaryExpression& node) = 0;

    virtual void visit(BlockStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(DoWhileStatement& node) = 0;
    virtual void visit(SwitchStatement& node) = 0;
    virtual void visit(CaseStatement& node) = 0;
    virtual void visit(TryStatement& node) = 0;
    virtual void visit(CatchClause& node) = 0;
    virtual void visit(ThrowStatement& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(BreakStatement& node) = 0;
    virtual void visit(ContinueStatement& node) = 0;
    virtual void visit(PackageStatement& node) = 0;
    virtual void visit(AliasStatement& node) = 0;
    virtual void visit(ImportStatement& node) {}

    virtual void visit(EnumDeclaration& node) = 0;
    virtual void visit(ClassDeclaration& node) = 0;
    virtual void visit(FieldDeclaration& node) = 0;
    virtual void visit(ConstructorDeclaration& node) = 0;
    virtual void visit(MethodDeclaration& node) = 0;
};

} // namespace solix

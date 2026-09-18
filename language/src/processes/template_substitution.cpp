#include "solix/processes/template_substitution.hpp"

namespace solix {

void TemplateSubstitutionVisitor::visit(BinaryExpression& n) {
    if (n.left) n.left->accept(*this);
    if (n.right) n.right->accept(*this);
}
void TemplateSubstitutionVisitor::visit(UnaryExpression& n) {
    if (n.operand) n.operand->accept(*this);
}
void TemplateSubstitutionVisitor::visit(AssignmentExpression& n) {
    if (n.target) n.target->accept(*this);
    if (n.value) n.value->accept(*this);
}
void TemplateSubstitutionVisitor::visit(ArrayAccessExpression& n) {
    if (n.array) n.array->accept(*this);
    if (n.index) n.index->accept(*this);
}
void TemplateSubstitutionVisitor::visit(MemberAccessExpression& n) {
    if (n.object) n.object->accept(*this);
}
void TemplateSubstitutionVisitor::visit(MethodCallExpression& n) {
    if (n.callee) n.callee->accept(*this);
    for (auto& arg : n.arguments) {
        if (arg) arg->accept(*this);
    }
    for (auto& t : n.type_args) {
        substitute_type(t);
    }
}
void TemplateSubstitutionVisitor::visit(NewInstanceExpression& n) {
    substitute_type(n.type_info);
    for (auto& arg : n.arguments) {
        if (arg) arg->accept(*this);
    }
}
void TemplateSubstitutionVisitor::visit(ArrayCreationExpression& n) {
    substitute_type(n.type_info);
    if (n.size) n.size->accept(*this);
}
void TemplateSubstitutionVisitor::visit(ArrayLiteralExpression& n) {
    for (auto& elem : n.elements) {
        if (elem) elem->accept(*this);
    }
}
void TemplateSubstitutionVisitor::visit(CastExpression& n) {
    substitute_type(n.target_type);
    if (n.expression) n.expression->accept(*this);
}
void TemplateSubstitutionVisitor::visit(InstanceofExpression& n) {
    substitute_type(n.target_type);
    if (n.expression) n.expression->accept(*this);
}
void TemplateSubstitutionVisitor::visit(TernaryExpression& n) {
    if (n.condition) n.condition->accept(*this);
    if (n.true_branch) n.true_branch->accept(*this);
    if (n.false_branch) n.false_branch->accept(*this);
}
void TemplateSubstitutionVisitor::visit(BlockStatement& n) {
    for (auto& child : n.children) {
        if (child) child->accept(*this);
    }
}
void TemplateSubstitutionVisitor::visit(IfStatement& n) {
    if (n.condition) n.condition->accept(*this);
    if (n.then_branch) n.then_branch->accept(*this);
    if (n.else_branch) n.else_branch->accept(*this);
}
void TemplateSubstitutionVisitor::visit(ForStatement& n) {
    if (n.initialization) n.initialization->accept(*this);
    if (n.condition) n.condition->accept(*this);
    if (n.iteration) n.iteration->accept(*this);
    if (n.body) n.body->accept(*this);
}
void TemplateSubstitutionVisitor::visit(WhileStatement& n) {
    if (n.condition) n.condition->accept(*this);
    if (n.body) n.body->accept(*this);
}
void TemplateSubstitutionVisitor::visit(DoWhileStatement& n) {
    if (n.body) n.body->accept(*this);
    if (n.condition) n.condition->accept(*this);
}
void TemplateSubstitutionVisitor::visit(SwitchStatement& n) {
    if (n.condition) n.condition->accept(*this);
    for (auto& child : n.children) {
        if (child) child->accept(*this);
    }
}
void TemplateSubstitutionVisitor::visit(CaseStatement& n) {
    if (n.case_value) n.case_value->accept(*this);
    for (auto& child : n.children) {
        if (child) child->accept(*this);
    }
}
void TemplateSubstitutionVisitor::visit(VariableDeclaration& n) {
    substitute_type(n.type_info);
    if (n.initializer) n.initializer->accept(*this);
}
void TemplateSubstitutionVisitor::visit(ExpressionStatement& n) {
    if (n.expression) n.expression->accept(*this);
}
void TemplateSubstitutionVisitor::visit(ReturnStatement& n) {
    if (n.value) n.value->accept(*this);
}
void TemplateSubstitutionVisitor::visit(BreakStatement& n) {}
void TemplateSubstitutionVisitor::visit(ContinueStatement& n) {}
void TemplateSubstitutionVisitor::visit(AliasStatement& n) {
    substitute_type(n.target_type);
}
void TemplateSubstitutionVisitor::visit(ClassDeclaration& n) {
    for (auto& child : n.children) {
        if (child) child->accept(*this);
    }
}
void TemplateSubstitutionVisitor::visit(FieldDeclaration& n) {
    substitute_type(n.type_info);
    if (n.initializer) n.initializer->accept(*this);
}
void TemplateSubstitutionVisitor::visit(ConstructorDeclaration& n) {
    for (auto& param : n.parameters) {
        if (param) param->accept(*this);
    }
    for (auto& child : n.children) {
        if (child) child->accept(*this);
    }
}
void TemplateSubstitutionVisitor::visit(MethodDeclaration& n) {
    substitute_type(n.return_type);
    for (auto& param : n.parameters) {
        if (param) param->accept(*this);
    }
    for (auto& child : n.children) {
        if (child) child->accept(*this);
    }
}

} // namespace solix

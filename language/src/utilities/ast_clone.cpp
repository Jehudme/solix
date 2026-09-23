#include "solix/statements.hpp"

namespace solix {

static Token make_dummy_token(const Node* node) {
    Token t;
    t.type = TokenType::UNKNOWN_TOKEN;
    t.line = node->line;
    t.column = node->column;
    t.source = node->source;
    return t;
}

template <typename T>
static void copy_children(const Node* src, T* dst) {
    for (const auto& child : src->children) {
        if (child) {
            dst->children.push_back(child->clone());
        } else {
            dst->children.push_back(nullptr);
        }
    }
}

// ==========================================
// Expressions
// ==========================================

std::unique_ptr<Node> IdentifierNode::clone() const {
    auto cloned = std::make_unique<IdentifierNode>(make_dummy_token(this), name);
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> LiteralNode::clone() const {
    auto cloned = std::make_unique<LiteralNode>(make_dummy_token(this), value);
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> BinaryExpression::clone() const {
    auto cloned = std::make_unique<BinaryExpression>(
        make_dummy_token(this),
        left ? left->clone() : nullptr,
        op,
        right ? right->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> UnaryExpression::clone() const {
    auto cloned = std::make_unique<UnaryExpression>(
        make_dummy_token(this),
        op,
        operand ? operand->clone() : nullptr,
        is_prefix
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> AssignmentExpression::clone() const {
    auto cloned = std::make_unique<AssignmentExpression>(
        make_dummy_token(this),
        target ? target->clone() : nullptr,
        op,
        value ? value->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ArrayAccessExpression::clone() const {
    auto cloned = std::make_unique<ArrayAccessExpression>(
        make_dummy_token(this),
        array ? array->clone() : nullptr,
        index ? index->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> MemberAccessExpression::clone() const {
    auto cloned = std::make_unique<MemberAccessExpression>(
        make_dummy_token(this),
        object ? object->clone() : nullptr,
        member_name
    );
    cloned->is_scope_resolution = is_scope_resolution;
    cloned->enum_value = enum_value;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> MethodCallExpression::clone() const {
    auto cloned = std::make_unique<MethodCallExpression>(
        make_dummy_token(this),
        callee ? callee->clone() : nullptr
    );
    for (const auto& arg : arguments) {
        cloned->arguments.push_back(arg ? arg->clone() : nullptr);
    }
    cloned->is_virtual_call = is_virtual_call;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> NewInstanceExpression::clone() const {
    auto cloned = std::make_unique<NewInstanceExpression>(
        make_dummy_token(this),
        type_info
    );
    for (const auto& arg : arguments) {
        cloned->arguments.push_back(arg ? arg->clone() : nullptr);
    }
    cloned->is_virtual_call = is_virtual_call;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ArrayCreationExpression::clone() const {
    auto cloned = std::make_unique<ArrayCreationExpression>(
        make_dummy_token(this),
        type_info,
        size ? size->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ArrayLiteralExpression::clone() const {
    auto cloned = std::make_unique<ArrayLiteralExpression>(make_dummy_token(this));
    for (const auto& el : elements) {
        cloned->elements.push_back(el ? el->clone() : nullptr);
    }
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> CastExpression::clone() const {
    auto cloned = std::make_unique<CastExpression>(
        make_dummy_token(this),
        target_type,
        expression ? expression->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> InstanceofExpression::clone() const {
    auto cloned = std::make_unique<InstanceofExpression>(
        make_dummy_token(this),
        expression ? expression->clone() : nullptr,
        target_type
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> TernaryExpression::clone() const {
    auto cloned = std::make_unique<TernaryExpression>(
        make_dummy_token(this),
        condition ? condition->clone() : nullptr,
        true_branch ? true_branch->clone() : nullptr,
        false_branch ? false_branch->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

// ==========================================
// Statements
// ==========================================

std::unique_ptr<Node> BlockStatement::clone() const {
    auto cloned = std::make_unique<BlockStatement>(make_dummy_token(this));
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> IfStatement::clone() const {
    auto cloned = std::make_unique<IfStatement>(
        make_dummy_token(this),
        condition ? condition->clone() : nullptr,
        then_branch ? then_branch->clone() : nullptr,
        else_branch ? else_branch->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ForStatement::clone() const {
    auto cloned = std::make_unique<ForStatement>(make_dummy_token(this));
    cloned->initialization = initialization ? initialization->clone() : nullptr;
    cloned->condition = condition ? condition->clone() : nullptr;
    cloned->iteration = iteration ? iteration->clone() : nullptr;
    cloned->body = body ? body->clone() : nullptr;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> WhileStatement::clone() const {
    auto cloned = std::make_unique<WhileStatement>(make_dummy_token(this));
    cloned->condition = condition ? condition->clone() : nullptr;
    cloned->body = body ? body->clone() : nullptr;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> DoWhileStatement::clone() const {
    auto cloned = std::make_unique<DoWhileStatement>(make_dummy_token(this));
    cloned->body = body ? body->clone() : nullptr;
    cloned->condition = condition ? condition->clone() : nullptr;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> SwitchStatement::clone() const {
    auto cloned = std::make_unique<SwitchStatement>(make_dummy_token(this));
    cloned->condition = condition ? condition->clone() : nullptr;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> CaseStatement::clone() const {
    auto cloned = std::make_unique<CaseStatement>(make_dummy_token(this));
    cloned->case_value = case_value ? case_value->clone() : nullptr;
    cloned->is_default = is_default;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> VariableDeclaration::clone() const {
    auto cloned = std::make_unique<VariableDeclaration>(
        make_dummy_token(this),
        var_name,
        type_info
    );
    cloned->is_const = is_const;
    cloned->is_reference_type = is_reference_type;
    cloned->is_weak = is_weak;
    cloned->initializer = initializer ? initializer->clone() : nullptr;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ExpressionStatement::clone() const {
    auto cloned = std::make_unique<ExpressionStatement>(
        make_dummy_token(this),
        expression ? expression->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ReturnStatement::clone() const {
    auto cloned = std::make_unique<ReturnStatement>(
        make_dummy_token(this),
        value ? value->clone() : nullptr
    );
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> BreakStatement::clone() const {
    auto cloned = std::make_unique<BreakStatement>(make_dummy_token(this));
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ContinueStatement::clone() const {
    auto cloned = std::make_unique<ContinueStatement>(make_dummy_token(this));
    copy_children(this, cloned.get());
    return cloned;
}

// ==========================================
// Top Level Declarations
// ==========================================

std::unique_ptr<Node> PackageStatement::clone() const {
    auto cloned = std::make_unique<PackageStatement>(make_dummy_token(this), package_name);
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> AliasStatement::clone() const {
    auto cloned = std::make_unique<AliasStatement>(
        make_dummy_token(this),
        alias_name,
        target_type
    );
    cloned->template_parameters = template_parameters;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> EnumDeclaration::clone() const {
    auto cloned = std::make_unique<EnumDeclaration>(make_dummy_token(this), enum_name);
    cloned->access_modifier = access_modifier;
    cloned->members = members;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ClassDeclaration::clone() const {
    auto cloned = std::make_unique<ClassDeclaration>(make_dummy_token(this), class_name);
    cloned->template_parameters = template_parameters;
    cloned->base_class_name = base_class_name;
    cloned->access_modifier = access_modifier;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> FieldDeclaration::clone() const {
    auto cloned = std::make_unique<FieldDeclaration>(
        make_dummy_token(this),
        field_name,
        type_info
    );
    cloned->access_modifier = access_modifier;
    cloned->is_static = is_static;
    cloned->is_const = is_const;
    cloned->is_reference_type = is_reference_type;
    cloned->is_weak = is_weak;
    cloned->initializer = initializer ? initializer->clone() : nullptr;
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> ConstructorDeclaration::clone() const {
    auto cloned = std::make_unique<ConstructorDeclaration>(make_dummy_token(this), class_name);
    cloned->access_modifier = access_modifier;
    cloned->template_parameters = template_parameters;
    cloned->base_class_name = base_class_name;
    for (const auto& param : parameters) {
        if (param) {
            cloned->parameters.push_back(std::unique_ptr<VariableDeclaration>(
                static_cast<VariableDeclaration*>(param->clone().release())
            ));
        } else {
            cloned->parameters.push_back(nullptr);
        }
    }
    copy_children(this, cloned.get());
    return cloned;
}

std::unique_ptr<Node> MethodDeclaration::clone() const {
    auto cloned = std::make_unique<MethodDeclaration>(make_dummy_token(this), method_name, return_type);
    cloned->access_modifier = access_modifier;
    cloned->is_static = is_static;
    cloned->is_native = is_native;
    cloned->is_inline = is_inline;
    cloned->is_virtual = is_virtual;
    cloned->is_override = is_override;
    cloned->is_abstract = is_abstract;
    cloned->template_parameters = template_parameters;
    cloned->native_id = native_id;
    for (const auto& param : parameters) {
        if (param) {
            cloned->parameters.push_back(std::unique_ptr<VariableDeclaration>(
                static_cast<VariableDeclaration*>(param->clone().release())
            ));
        } else {
            cloned->parameters.push_back(nullptr);
        }
    }
    copy_children(this, cloned.get());
    return cloned;
}

} // namespace solix

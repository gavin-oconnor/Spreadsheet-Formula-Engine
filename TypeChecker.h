#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "GPFETypes.h"
#include "FunctionRegistry.h"
#include <unordered_map>
#include <string>
#include <iostream>

auto valid_numeric_operand = [](const TypeInfo &type)
{
    return (type.type == BaseType::CellRef || type.type == BaseType::Number || type.type == BaseType::Unknown);
};

auto is_error = [](const TypeInfo &type)
{
    return type.type == BaseType::Error;
};

auto is_range = [](const TypeInfo &type)
{
    return type.type == BaseType::Range;
};

auto valid_range_operand = [](const TypeInfo &type)
{
    return (type.type == BaseType::CellRef || type.type == BaseType::Unknown);
};

class TypeChecker
{
public:
    TypeInfo infer(ASTNode *node)
    {
        switch (node->type)
        {
        case ASTNodeType::Literal:
        {
            const auto &lit = std::get<Literal>(node->node);
            switch (lit.type)
            {
            case LiteralType::Numeric:
            {
                node->inferredType = {BaseType::Number};
                return node->inferredType;
            }
            case LiteralType::String:
            {
                node->inferredType = {BaseType::String};
                return node->inferredType;
            }
            default:
                node->inferredType = {BaseType::Error};
                return node->inferredType;
            }
        }
        case ASTNodeType::Unary:
        {
            const auto &unary_op = std::get<UnaryOperation>(node->node);
            auto operand = unary_op.operand.get();
            auto operand_type_info = infer(operand);
            switch (unary_op.op)
            {
            case UnaryOp::Minus:
            case UnaryOp::Plus:
            case UnaryOp::Percent:
            {
                if (!valid_numeric_operand(operand_type_info))
                {
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                node->inferredType = {BaseType::Number};
                return node->inferredType;
            }
            }
        }
        case ASTNodeType::Binary:
        {
            const auto &binary_op = std::get<BinaryOperation>(node->node);
            auto left = binary_op.left.get();
            auto right = binary_op.right.get();
            auto left_type_info = infer(left);
            auto right_type_info = infer(right);
            switch (binary_op.op)
            {
            case BinaryOp::Pow:
            case BinaryOp::Mul:
            case BinaryOp::Div:
            case BinaryOp::Add:
            case BinaryOp::Sub:
            {
                if (!valid_numeric_operand(left_type_info) || !valid_numeric_operand(right_type_info))
                {
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                node->inferredType = {BaseType::Number};
                return node->inferredType;
            }
            case BinaryOp::Less:
            case BinaryOp::Greater:
            case BinaryOp::Geq:
            case BinaryOp::Leq:
            {
                if (!valid_numeric_operand(left_type_info) || !valid_numeric_operand(right_type_info))
                {
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                node->inferredType = {BaseType::Bool};
                return node->inferredType;
            }
            case BinaryOp::Eq:
            case BinaryOp::Neq:
            {
                if (is_error(left_type_info) || is_error(right_type_info))
                {
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                node->inferredType = {BaseType::Bool};
                return node->inferredType;
            }
            case BinaryOp::Concat:
            {
                if (is_error(left_type_info) || is_error(right_type_info) || is_range(left_type_info) || is_range(right_type_info))
                {
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                node->inferredType = {BaseType::String};
                return node->inferredType;
            }
            case BinaryOp::Range:
            {
                if (!valid_range_operand(left_type_info) || !valid_range_operand(right_type_info))
                {
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                node->inferredType = {BaseType::Range};
                return node->inferredType;
            }
            }
        }
        case ASTNodeType::Reference:
        {
            const auto &reference = std::get<Reference>(node->node);
            switch (reference.type)
            {
            case ReferenceType::Cell:
            {
                node->inferredType = {BaseType::CellRef};
                return node->inferredType;
            }
            case ReferenceType::Range:
            {
                node->inferredType = {BaseType::Range};
                return node->inferredType;
            }
            default:
            {
                node->inferredType = {BaseType::Error};
                return node->inferredType;
            }
            }
        }
        case ASTNodeType::FunctionCall:
        {
            const auto &call = std::get<FunctionCall>(node->node);
            const auto *sig = funcs::lookup(call.identifier);
            if (!sig)
            {
                // diags.error(call.identifier span, "Unknown function");
                node->inferredType = {BaseType::Error};
                return node->inferredType;
            }

            if (sig->variableArity)
            {
                // require at least one Param (the tail spec)
                if (sig->params.empty())
                {
                    // diags.error(node->span, "Internal: variadic signature missing tail param");
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }

                const size_t fixed = sig->params.size() - 1; // 0..N fixed, last is tail
                if (call.args.size() < fixed)
                {
                    // diags.error(node->span, "Too few arguments to " + sig->name);
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                // fixed
                for (size_t i = 0; i < fixed; ++i)
                {
                    auto ti = infer(call.args[i].get());
                    if (!funcs::matchesParam(sig->params[i], ti.type))
                    {
                        // diags.error(call.args[i]->span, "Arg " + std::to_string(i+1) + " has incompatible type");
                        node->inferredType = {BaseType::Error};
                        return node->inferredType;
                    }
                }
                // vararg tail
                const auto &tail = sig->params.back();
                for (size_t i = fixed; i < call.args.size(); ++i)
                {
                    auto ti = infer(call.args[i].get());
                    if (!funcs::matchesParam(tail, ti.type))
                    {
                        // diags.error(call.args[i]->span, "Arg " + std::to_string(i+1) + " has incompatible type");
                        node->inferredType = {BaseType::Error};
                        return node->inferredType;
                    }
                }
            }
            else
            {
                if (sig->params.size() != call.args.size())
                {
                    // diags.error(node->span, "Wrong number of arguments to " + sig->name);
                    node->inferredType = {BaseType::Error};
                    return node->inferredType;
                }
                for (size_t i = 0; i < call.args.size(); ++i)
                {
                    auto ti = infer(call.args[i].get());
                    if (!funcs::matchesParam(sig->params[i], ti.type))
                    {
                        // diags.error(call.args[i]->span, "Arg " + std::to_string(i+1) + " has incompatible type");
                        node->inferredType = {BaseType::Error};
                        return node->inferredType;
                    }
                }
            }

            node->inferredType = {sig->returnType};
            return node->inferredType;
        }
        }
    }
};

#endif
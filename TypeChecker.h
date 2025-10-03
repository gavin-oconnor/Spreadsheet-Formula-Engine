#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "GPFETypes.h"
#include "FunctionRegistry.h"
#include <unordered_map>
#include <string>

std::unordered_map<std::string, FunctionSignature> functions;

auto valid_numeric_operand = [](const TypeInfo &type)
{
    return (type.type == BaseType::CellRef || type.type == BaseType::Number || type.type == BaseType::Unknown);
};

auto is_error = [](const TypeInfo &type)
{
    return type.type == BaseType::Error;
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
                if (is_error(left_type_info) || is_error(right_type_info))
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
            const auto &function_call = std::get<FunctionCall>(node->node);
            if (functions.find(function_call.identifier) == functions.end())
            {
                node->inferredType = {BaseType::Error};
                return node->inferredType;
            }
            throw std::runtime_error("FUNCTION TYPE CHECKING NOT IMPLEMENTED");
        }
        }
    }
};

#endif
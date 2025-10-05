#include "Evaluator.h"

Value Evaluator::evaluateNode(const ASTNode *node, std::map<RC, Value> &cell_map)
{
    if (node->inferredType.type == BaseType::Error)
    {
        return Error{ErrorCode::Value};
    }
    switch (node->type)
    {
    case ASTNodeType::Literal:
    {
        const auto &lit = std::get<Literal>(node->node);
        switch (lit.type)
        {
        case LiteralType::Numeric:
            return std::get<double>(lit.value);
        case LiteralType::String:
            return std::get<std::string>(lit.value);
        }
    }
    case ASTNodeType::Unary:
    {
        const auto &unary_op = std::get<UnaryOperation>(node->node);
        auto operand = unary_op.operand.get();
        Value operand_value = evaluateNode(operand, cell_map);
        if (!std::holds_alternative<double>(operand_value))
            return Error{ErrorCode::Value};
        double double_value = std::get<double>(operand_value);
        switch (unary_op.op)
        {
        case UnaryOp::Plus:
            return double_value;
        case UnaryOp::Minus:
            return -1.0 * double_value;
        case UnaryOp::Percent:
            return double_value / 100.0;
        default:
            return Error{ErrorCode::Value};
        }
    }
    default:
        return Error{ErrorCode::Value};
    }
}
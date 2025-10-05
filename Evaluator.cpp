#include "Evaluator.h"
inline std::string EVAL_to_text(const Value &v)
{
    return std::visit([](const auto &x) -> std::string
                      {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double>) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.15g", x);
            return std::string(buf);
        } else if constexpr (std::is_same_v<T, bool>) {
            return std::string(x ? "TRUE" : "FALSE");
        } else if constexpr (std::is_same_v<T, std::string>) {
            return x; // copy is fine
        } else if constexpr (std::is_same_v<T, RangeRef>) {
            return std::string("#VALUE!"); // or your chosen behavior
        } else { // Error
            return std::string("#VALUE!"); // or map ErrorCode -> text
        } }, v);
}

inline bool is_textish(const Value &v)
{
    return std::holds_alternative<double>(v) || std::holds_alternative<bool>(v) || std::holds_alternative<std::string>(v); // or Text
}

auto EVAL_valid_concat_operands = [](const Value &left, const Value &right)
{
    return is_textish(left) && is_textish(right);
};

auto EVAL_valid_numeric_operands = [](const Value &left, const Value &right)
{
    return (std::holds_alternative<double>(left) && std::holds_alternative<double>(right));
};

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
    case ASTNodeType::Binary:
    {
        const auto &binary_op = std::get<BinaryOperation>(node->node);
        auto left = binary_op.left.get();
        auto right = binary_op.right.get();
        Value evaluated_left = evaluateNode(left, cell_map);
        Value evaluated_right = evaluateNode(right, cell_map);
        switch (binary_op.op)
        {
        case BinaryOp::Add:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            return std::get<double>(evaluated_left) + std::get<double>(evaluated_right);
        }
        case BinaryOp::Sub:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            return std::get<double>(evaluated_left) + std::get<double>(evaluated_right);
        }
        case BinaryOp::Mul:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            return std::get<double>(evaluated_left) * std::get<double>(evaluated_right);
        }
        case BinaryOp::Div:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            if (std::get<double>(evaluated_right) == 0.0)
            {
                return Error{ErrorCode::Div0};
            }
            return std::get<double>(evaluated_left) / std::get<double>(evaluated_right);
        }
        case BinaryOp::Pow:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            if (std::get<double>(evaluated_left) == 0.0 && std::get<double>(evaluated_right) == 0.0)
            {
                return Error{ErrorCode::Num};
            }
            return std::get<double>(evaluated_left) / std::get<double>(evaluated_right);
        }
        case BinaryOp::Less:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            return std::get<double>(evaluated_left) < std::get<double>(evaluated_right);
        }
        case BinaryOp::Greater:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            return std::get<double>(evaluated_left) > std::get<double>(evaluated_right);
        }
        case BinaryOp::Leq:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            return std::get<double>(evaluated_left) <= std::get<double>(evaluated_right);
        }
        case BinaryOp::Geq:
        {
            if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            return std::get<double>(evaluated_left) >= std::get<double>(evaluated_right);
        }
        case BinaryOp::Eq:
        {
            throw std::runtime_error("EQ NOT DEFINED YET");
        }
        case BinaryOp::Neq:
        {
            throw std::runtime_error("EQ NOT DEFINED YET");
        }
        case BinaryOp::Concat:
        {
            if (!EVAL_valid_concat_operands(evaluated_left, evaluated_right))
                return Error{ErrorCode::Value};
            std::string a = EVAL_to_text(evaluated_left);
            std::string b = EVAL_to_text(evaluated_right);
            std::string out;
            out.reserve(a.size() + b.size());
            out.append(a);
            out.append(b);
            return out;
        }
        default:
            return Error{ErrorCode::Value};
        }
    }
    default:
        return Error{ErrorCode::Value};
    }
}
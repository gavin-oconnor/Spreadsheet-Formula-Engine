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

inline bool is_error(const Value &v)
{
    return std::holds_alternative<Error>(v);
}

inline bool is_textish(const Value &v)
{
    return std::holds_alternative<double>(v) || std::holds_alternative<bool>(v) || std::holds_alternative<std::string>(v); // or Text
}

// 0 --> not same type or wrong types, 1 --> double, 2 --> string, 3 --> bool
inline int get_equality_type(const Value &left, const Value &right)
{
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
        return 1;
    else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
        return 2;
    else if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right))
        return 3;
    return 0;
};

auto EVAL_valid_concat_operands = [](const Value &left, const Value &right)
{
    return is_textish(left) && is_textish(right);
};

auto EVAL_valid_numeric_operands = [](const Value &left, const Value &right)
{
    return (std::holds_alternative<double>(left) && std::holds_alternative<double>(right));
};

Value Evaluator::evalScalar(const ASTNode *node, std::map<RC, Value> &cell_map)
{
    return evaluateNode(node, cell_map, EvalNeed::Scalar);
}

Value Evaluator::evalRefLike(const ASTNode *node, std::map<RC, Value> &cell_map)
{
    return evaluateNode(node, cell_map, EvalNeed::RefLike);
}

Value Evaluator::evaluateNode(const ASTNode *node, std::map<RC, Value> &cell_map, EvalNeed need)
{
    // add check in each operator, if the Value of a node is unknown after evaluation just throw a #VALUE
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
        Value operand_value = evalScalar(operand, cell_map);
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
        // first check if ref
        const auto &binary_op = std::get<BinaryOperation>(node->node);
        auto left = binary_op.left.get();
        auto right = binary_op.right.get();
        if (binary_op.op == BinaryOp::Range)
        {
            // both values should be cell ref's (either literals or formulaic from indirect or similar)
            // can be a cell range if it's 1x1
            Value evaluated_left = evalRefLike(left, cell_map);
            Value evaluated_right = evalRefLike(right, cell_map);
            // pls fix add bounds checking on ranges
            if (!std::holds_alternative<RangeRef>(evaluated_left) || !std::holds_alternative<RangeRef>(evaluated_right))
            {
                return Error{ErrorCode::Ref};
            }
            RangeRef range_left = std::get<RangeRef>(evaluated_left);
            RangeRef range_right = std::get<RangeRef>(evaluated_right);
            RangeRef new_range{};
            new_range.top = std::min(range_left.top, range_right.top);
            new_range.bottom = std::max(range_left.bottom, range_right.bottom);
            new_range.left = std::min(range_left.left, range_right.right);
            new_range.right = std::max(range_left.left, range_right.right);
            return new_range;
        }
        else
        {
            Value evaluated_left = evalScalar(left, cell_map);
            Value evaluated_right = evalScalar(right, cell_map);
            if (is_error(evaluated_left) || is_error(evaluated_right))
                // pls fix
                return Error{ErrorCode::Value};
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
                int equality_type = get_equality_type(evaluated_left, evaluated_right);
                if (!equality_type)
                    return false;
                switch (equality_type)
                {
                case 1:
                {
                    double d_left = std::get<double>(evaluated_left);
                    double d_right = std::get<double>(evaluated_right);
                    return d_left == d_right;
                }
                case 2:
                {
                    std::string s_left = std::get<std::string>(evaluated_left);
                    std::string s_right = std::get<std::string>(evaluated_right);
                    return s_left == s_right;
                }
                case 3:
                {
                    bool b_left = std::get<bool>(evaluated_left);
                    bool b_right = std::get<bool>(evaluated_right);
                    return b_left == b_right;
                }
                default:
                    return Error{ErrorCode::Value};
                }
            }
            case BinaryOp::Neq:
            {
                int equality_type = get_equality_type(evaluated_left, evaluated_right);
                if (!equality_type)
                    return true;
                switch (equality_type)
                {
                case 1:
                {
                    double d_left = std::get<double>(evaluated_left);
                    double d_right = std::get<double>(evaluated_right);
                    return d_left != d_right;
                }
                case 2:
                {
                    std::string s_left = std::get<std::string>(evaluated_left);
                    std::string s_right = std::get<std::string>(evaluated_right);
                    return s_left != s_right;
                }
                case 3:
                {
                    bool b_left = std::get<bool>(evaluated_left);
                    bool b_right = std::get<bool>(evaluated_right);
                    return b_left != b_right;
                }
                default:
                    return true;
                }
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
    }
    default:
        return Error{ErrorCode::Value};
    }
}
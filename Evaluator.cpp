#include "Evaluator.h"
#include "FunctionRegistry.h"
#include <iostream>

inline BaseType typeOfValue(const Value &v)
{
    return std::visit(
        [](auto &&x) -> BaseType
        {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Number>)     return BaseType::Number;
        else if constexpr (std::is_same_v<T, Text>)  return BaseType::String;
        else if constexpr (std::is_same_v<T, Bool>)  return BaseType::Bool;
        else if constexpr (std::is_same_v<T, RangeRef>) return BaseType::Range;
        else if constexpr (std::is_same_v<T, Error>) return BaseType::Error;
        else if constexpr (std::is_same_v<T, Blank>) return BaseType::Blank;
        else return BaseType::Unknown; }, v);
}

bool argsMatchSignature(const funcs::FunctionSignature *sig, const std::vector<Value> &args)
{
    if (!sig->variableArity && args.size() != sig->params.size())
        return false;

    size_t fixed = sig->variableArity
                       ? (sig->params.size() ? sig->params.size() - 1 : 0)
                       : sig->params.size();

    for (size_t i = 0; i < fixed; ++i)
    {
        if (!funcs::matchesParam(sig->params[i], typeOfValue(args[i])))
            return false;
    }

    if (sig->variableArity && sig->params.size())
    {
        const auto &variadic = sig->params.back();
        for (size_t i = fixed; i < args.size(); ++i)
            if (!funcs::matchesParam(variadic, typeOfValue(args[i])))
                return false;
    }

    return true;
}

inline Text EVAL_to_text(const Value &v)
{
    return std::visit([](const auto &x) -> Text
                      {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Number>) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.15g", x);
            return std::string(buf);
        } else if constexpr (std::is_same_v<T, Bool>) {
            return std::string(x ? "TRUE" : "FALSE");
        } else if constexpr (std::is_same_v<T, Text>) {
            return x; // copy is fine
        } else if constexpr (std::is_same_v<T, RangeRef>) {
            return std::string("#VALUE!");
        }
        else if constexpr (std::is_same_v<T, Blank>)
        {
            return std::string("");
        }
        else
        {                             
            return std::string("#VALUE!"); 
        } }, v);
}

inline bool is_error(const Value &v)
{
    return std::holds_alternative<Error>(v);
}

inline bool is_textish(const Value &v)
{
    return std::holds_alternative<Number>(v) || std::holds_alternative<Bool>(v) || std::holds_alternative<Text>(v) || std::holds_alternative<Blank>(v); // or Text
}

// 0 --> not same type or wrong types, 1 --> double, 2 --> string, 3 --> bool
inline int get_equality_type(const Value &left, const Value &right)
{
    if (std::holds_alternative<Number>(left) && std::holds_alternative<Number>(right))
        return 1;
    else if (std::holds_alternative<Text>(left) && std::holds_alternative<Text>(right))
        return 2;
    else if (std::holds_alternative<Bool>(left) && std::holds_alternative<Bool>(right))
        return 3;
    else if (std::holds_alternative<Blank>(left) && std::holds_alternative<Blank>(right))
        return 4;
    return 0;
};

auto EVAL_valid_concat_operands = [](const Value &left, const Value &right)
{
    return is_textish(left) && is_textish(right);
};

auto EVAL_valid_numeric_operands = [](const Value &left, const Value &right)
{
    return (!std::holds_alternative<Number>(left) || !std::holds_alternative<Blank>(left) || !std::holds_alternative<Number>(right) || !std::holds_alternative<Blank>(right));
};

Value Evaluator::evalScalar(const ASTNode *node, EvalContext &evalCtx)
{
    return evaluateNode(node, EvalNeed::Scalar, evalCtx);
}

Value Evaluator::evalRefLike(const ASTNode *node, EvalContext &evalCtx)
{
    return evaluateNode(node, EvalNeed::RefLike, evalCtx);
}

Value Evaluator::evaluateNode(const ASTNode *node, EvalNeed need, EvalContext &evalCtx)
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
            return std::get<Number>(lit.value);
        case LiteralType::String:
            return std::get<Text>(lit.value);
        }
    }
    case ASTNodeType::Unary:
    {
        const auto &unary_op = std::get<UnaryOperation>(node->node);
        auto operand = unary_op.operand.get();
        Value operand_value = evalScalar(operand, evalCtx);
        if (!std::holds_alternative<Number>(operand_value))
            return Error{ErrorCode::Value};
        Number double_value = std::get<Number>(operand_value);
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
            Value evaluated_left = evalRefLike(left, evalCtx);
            Value evaluated_right = evalRefLike(right, evalCtx);
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
            Value evaluated_left_no_coerce = evalScalar(left, evalCtx);
            Value evaluated_right_no_coerce = evalScalar(right, evalCtx);
            Value evaluated_left;
            Value evaluated_right;
            if (is_error(evaluated_left_no_coerce) || is_error(evaluated_right_no_coerce))
                // pls fix
                return Error{ErrorCode::Value};
            if (std::holds_alternative<Blank>(evaluated_left_no_coerce))
                evaluated_left = 0.0;
            else
                evaluated_left = evaluated_left_no_coerce;
            if (std::holds_alternative<Blank>(evaluated_right_no_coerce))
                evaluated_right = 0.0;
            else
                evaluated_right = evaluated_right_no_coerce;
            switch (binary_op.op)
            {
            case BinaryOp::Add:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                return std::get<Number>(evaluated_left) + std::get<Number>(evaluated_right);
            }
            case BinaryOp::Sub:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                return std::get<Number>(evaluated_left) - std::get<Number>(evaluated_right);
            }
            case BinaryOp::Mul:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                return std::get<Number>(evaluated_left) * std::get<Number>(evaluated_right);
            }
            case BinaryOp::Div:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                if (std::get<Number>(evaluated_right) == 0.0)
                {
                    return Error{ErrorCode::Div0};
                }
                return std::get<Number>(evaluated_left) / std::get<Number>(evaluated_right);
            }
            case BinaryOp::Pow:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                if (std::get<Number>(evaluated_left) == 0.0 && std::get<Number>(evaluated_right) == 0.0)
                {
                    return Error{ErrorCode::Num};
                }
                // pls fix POW
                return std::get<Number>(evaluated_left) / std::get<Number>(evaluated_right);
            }
            case BinaryOp::Less:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                return std::get<Number>(evaluated_left) < std::get<Number>(evaluated_right);
            }
            case BinaryOp::Greater:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                return std::get<Number>(evaluated_left) > std::get<Number>(evaluated_right);
            }
            case BinaryOp::Leq:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                return std::get<Number>(evaluated_left) <= std::get<Number>(evaluated_right);
            }
            case BinaryOp::Geq:
            {
                if (!EVAL_valid_numeric_operands(evaluated_left, evaluated_right))
                    return Error{ErrorCode::Value};
                return std::get<Number>(evaluated_left) >= std::get<Number>(evaluated_right);
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
                    Number d_left = std::get<Number>(evaluated_left);
                    Number d_right = std::get<Number>(evaluated_right);
                    std::cout << "checking double equality\n";
                    std::cout << "D_LEFT = " << d_left << "\n";
                    std::cout << "D_RIGHT = " << d_right << "\n";

                    return d_left == d_right;
                }
                case 2:
                {
                    Text s_left = std::get<Text>(evaluated_left);
                    Text s_right = std::get<Text>(evaluated_right);
                    return s_left == s_right;
                }
                case 3:
                {
                    Bool b_left = std::get<Bool>(evaluated_left);
                    Bool b_right = std::get<Bool>(evaluated_right);
                    return b_left == b_right;
                }
                case 4:
                    return true;
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
                    Number d_left = std::get<Number>(evaluated_left);
                    Number d_right = std::get<Number>(evaluated_right);
                    return d_left != d_right;
                }
                case 2:
                {
                    Text s_left = std::get<Text>(evaluated_left);
                    Text s_right = std::get<Text>(evaluated_right);
                    return s_left != s_right;
                }
                case 3:
                {
                    Bool b_left = std::get<Bool>(evaluated_left);
                    Bool b_right = std::get<Bool>(evaluated_right);
                    return b_left != b_right;
                }
                case 4:
                    return false;
                default:
                    return true;
                }
            }
            case BinaryOp::Concat:
            {
                if (!EVAL_valid_concat_operands(evaluated_left_no_coerce, evaluated_right_no_coerce))
                    return Error{ErrorCode::Value};
                Text a = EVAL_to_text(evaluated_left_no_coerce);
                Text b = EVAL_to_text(evaluated_right_no_coerce);
                Text out;
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
    case ASTNodeType::Reference:
    {
        if (need == EvalNeed::RefLike)
        {
            const auto &reference = std::get<Reference>(node->node);
            switch (reference.type)
            {
            case ReferenceType::Cell:
            {
                // pls fix bounds checking
                CellReference ref = std::get<CellReference>(reference.ref);
                RangeRef range_ref;
                range_ref.left = ref.col;
                range_ref.right = ref.col;
                range_ref.top = ref.row;
                range_ref.bottom = ref.row;
                return range_ref;
            }
            case ReferenceType::Range:
            {
                RangeReference ref = std::get<RangeReference>(reference.ref);
                RangeRef range_ref;
                range_ref.left = ref.left;
                range_ref.right = ref.right;
                range_ref.top = ref.top;
                range_ref.bottom = ref.bottom;
                return range_ref;
            }
            default:
                return Error{ErrorCode::Value};
            }
        }
        else if (need == EvalNeed::Scalar)
        {
            const auto &reference = std::get<Reference>(node->node);
            switch (reference.type)
            {
            case ReferenceType::Cell:
            {
                // pls fix bounds checking
                CellReference ref = std::get<CellReference>(reference.ref);
                auto it = evalCtx.cell_map->find(RC{ref.row, ref.col});
                if (it != evalCtx.cell_map->end())
                {
                    return it->second;
                }
                else
                {
                    return Blank{};
                }
            }
            case ReferenceType::Range:
            {
                return Error{ErrorCode::Value};
            }
            default:
                return Error{ErrorCode::Value};
            }
        }
    }
    case ASTNodeType::FunctionCall:
    {
        const auto &function_call = std::get<FunctionCall>(node->node);
        std::vector<Value> evaluated_args;
        // evaluate args
        for (int i = 0; i < function_call.args.size(); i++)
        {
            evaluated_args.push_back(evalScalar(function_call.args[i].get(), evalCtx));
        }

        // type check args
        const auto *sig = funcs::lookup(function_call.identifier);
        if (!sig)
            return Error{ErrorCode::Name};
        if (!argsMatchSignature(sig, evaluated_args))
            return Error{ErrorCode::Value};
        return sig->eval_function(evaluated_args, evalCtx);
    }
    default:
        return Error{ErrorCode::Value};
    }
}

/*

bool argsMatchSignature(
    const funcs::FunctionSignature& sig,
    const std::vector<Value>& args)
{
    if (!sig.variableArity && args.size() != sig.params.size())
        return false;

    size_t fixed = sig.variableArity
        ? (sig.params.size() ? sig.params.size() - 1 : 0)
        : sig.params.size();

    for (size_t i = 0; i < fixed; ++i) {
        if (!funcs::matchesParam(sig.params[i], typeOfValue(args[i])))
            return false;
    }

    if (sig.variableArity && sig.params.size()) {
        const auto& variadic = sig.params.back();
        for (size_t i = fixed; i < args.size(); ++i)
            if (!funcs::matchesParam(variadic, typeOfValue(args[i])))
                return false;
    }

    return true;
}


*/
#include "FunctionRegistry.h"
#include <unordered_map>
#include <iostream>

Value fn_SUM(const std::vector<Value> &args, EvalContext &evalCtx)
{
    std::cout << "STARTING SUM\n";
    Number sum = 0;
    for (int i = 0; i < args.size(); i++)
    {
        if (std::holds_alternative<Number>(args[i]))
        {
            sum += std::get<Number>(args[i]);
        }
        else if (std::holds_alternative<RangeRef>(args[i]))
        {
            RangeRef range = std::get<RangeRef>(args[i]);
            for (int r = range.left; r <= range.right; r++)
            {
                for (int c = range.top; c <= range.bottom; c++)
                {
                    auto it = evalCtx.cell_map->find(RC{r, c});
                    if (it != evalCtx.cell_map->end())
                    {
                        if (std::holds_alternative<Number>(it->second))
                            sum += std::get<Number>(it->second);
                    }
                }
            }
        }
    }
    std::cout << "RETURNING FROM SUM\n";
    return sum;
};

Value fn_LEN(const std::vector<Value> &args, EvalContext &evalCtx)
{
    // pls fix len should attemp type coercion
    if (!args.size())
        throw std::runtime_error("LEN should never receive no args and reach this pont of execution");
    if (std::holds_alternative<Text>(args[0]))
    {
        return 1.0 * std::get<Text>(args[0]).size();
    }
    return 0.0;
};

Value fn_IF(const std::vector<Value> &args, EvalContext &evalCtx)
{
    if (args.size() != 3)
        throw std::runtime_error("IF should never not receive 3 args and reach this pont of execution");
    if (std::holds_alternative<Bool>(args[0]))
    {
        std::cout << std::get<Bool>(args[0]) << "\n";
        if (std::get<Bool>(args[0]))
        {
            return args[1];
        }
        else
        {
            return args[2];
        }
    }
    return Blank{};
}

namespace funcs
{

    static const std::unordered_map<std::string, FunctionSignature> &table()
    {
        static const std::unordered_map<std::string, FunctionSignature> k = {
            {"SUM", FunctionSignature{"SUM", {Param{{ArgKind::Number, ArgKind::Ref, ArgKind::Range}}}, true, BaseType::Number, fn_SUM}},
            {"LEN", FunctionSignature{"LEN", {Param{{ArgKind::Text}}}, false, BaseType::Number, fn_LEN}},
            {"IF", FunctionSignature{"IF", {Param{{ArgKind::Bool}}, Param{{ArgKind::AnyScalar}}, Param{{ArgKind::AnyScalar}}}, false, BaseType::Unknown, fn_IF}},
        };
        return k;
    }

    const FunctionSignature *lookup(std::string_view name)
    {
        const auto &k = table();
        if (auto it = k.find(std::string(name)); it != k.end())
            return &it->second;
        return nullptr;
    }

} // namespace funcs
#include "FunctionRegistry.h"
#include <unordered_map>

namespace funcs
{

    static const std::unordered_map<std::string, FunctionSignature> &table()
    {
        static const std::unordered_map<std::string, FunctionSignature> k = {
            // SUM: variadic; each arg can be Number, Ref (cell), or Range
            {"SUM", FunctionSignature{
                        "SUM",
                        {Param{{ArgKind::Number, ArgKind::Ref, ArgKind::Range}}}, // tail spec
                        /*variableArity=*/true,
                        BaseType::Number}},
            // LEN(text) -> number
            {"LEN", FunctionSignature{"LEN", {Param{{ArgKind::Text}}}, false, BaseType::Number}},
            // IF(bool, any, any) -> (return type refined later)
            {"IF", FunctionSignature{"IF", {Param{{ArgKind::Bool}}, Param{{ArgKind::AnyScalar}}, Param{{ArgKind::AnyScalar}}}, false, BaseType::Unknown}},
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
#ifndef FUNCTION_REGISTRY_H
#define FUNCTION_REGISTRY_H

#include <string>
#include <vector>
#include "GPFETypes.h"
#include "EvalTypes.h"

namespace funcs
{

    enum class ArgKind
    {
        Number,
        Text,
        Bool,
        AnyScalar,
        Ref,
        Range
    };

    struct Param
    {
        std::vector<ArgKind> anyOf; // allowed kinds
    };

    inline bool kindMatches(ArgKind k, BaseType t)
    {
        switch (k)
        {
        case ArgKind::Number:
            return t == BaseType::Number;
        case ArgKind::Text:
            return t == BaseType::String;
        case ArgKind::Bool:
            return t == BaseType::Bool;
        case ArgKind::AnyScalar:
            return t == BaseType::Number || t == BaseType::String || t == BaseType::Bool;
        case ArgKind::Ref:
            return t == BaseType::CellRef; // (no RefAny yet)
        case ArgKind::Range:
            return t == BaseType::Range;
        }
        return false;
    }

    // allow Unknown and CellRef-as-scalar
    inline bool matchesParam(const Param &p, BaseType t)
    {
        if (t == BaseType::Unknown)
            return true; // defer to runtime
        // implicit deref: allow CellRef where scalar is expected
        if (t == BaseType::CellRef)
        {
            for (auto k : p.anyOf)
            {
                if (k == ArgKind::Ref)
                    return true;
                if (k == ArgKind::Number || k == ArgKind::Text || k == ArgKind::Bool || k == ArgKind::AnyScalar)
                    return true; // treat as scalar via deref at eval
            }
            return false;
        }
        for (auto k : p.anyOf)
            if (kindMatches(k, t))
                return true;
        return false;
    }

    using EvalFn = Value (*)(const std::vector<Value> &, EvalContext &evalCtx);

    struct FunctionSignature
    {
        std::string name;
        std::vector<Param> params;
        bool variableArity;
        BaseType returnType;
        EvalFn eval_function;
    };

    const FunctionSignature *lookup(std::string_view name);
};

#endif
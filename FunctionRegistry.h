#ifndef FUNCTION_REGISTRY_H
#define FUNCTION_REGISTRY_H

#include <string>
#include <vector>
#include "GPFETypes.h"

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

struct FunctionSignature
{
    std::string name;
    std::vector<Param> params;
    bool variableArity;
    BaseType returnType;
};

#endif
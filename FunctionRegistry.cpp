#include "FunctionRegistry.h"
#include <unordered_map>

const FunctionSignature SUM_SIG{
    "SUM",
    {Param{{ArgKind::Number, ArgKind::Ref, ArgKind::Range}}}, // tail spec
    /*variableArity=*/true,
    BaseType::Number};

// LEN(text) -> number
const FunctionSignature LEN_SIG{
    "LEN",
    {Param{{ArgKind::Text}}},
    false,
    BaseType::Number};

// IF(bool, any, any) -> best-effort (you can refine return type later)
const FunctionSignature IF_SIG{
    "IF",
    {Param{{ArgKind::Bool}}, Param{{ArgKind::AnyScalar}}, Param{{ArgKind::AnyScalar}}},
    false,
    BaseType::Unknown};

static const std::unordered_map<std::string, FunctionSignature> functions = {
    {"SUM", SUM_SIG}, {"LEN", LEN_SIG}, {"IF", IF_SIG}};
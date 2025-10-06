#ifndef EVAL_TYPES_H
#define EVAL_TYPES_H

#include <string>
#include <map>

enum class ErrorCode
{
    None,
    Value,
    Div0,
    Ref,
    Name,
    Num,
    Cycle,
    NA
};

struct Error
{
    ErrorCode code;
};

enum class EvalNeed
{
    Scalar,
    RefLike
};

using Number = double;
using Bool = bool;
using Text = std::string;
struct RangeRef
{
    int left, right, top, bottom;
};

using Blank = std::monostate;

using Value = std::variant<Number, Bool, Text, RangeRef, Error, Blank>;

using RC = std::pair<int, int>;

struct EvalContext
{
    std::map<RC, Value> *cell_map;
};

#endif
#include "GPFETypes.h"
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

using Value = std::variant<Number, Bool, Text, RangeRef, Error>;

using RC = std::pair<int, int>;

class Evaluator
{
public:
    Value evaluateNode(const ASTNode *node, std::map<RC, Value> &cell_map, EvalNeed need);
    Value evalScalar(const ASTNode *node, std::map<RC, Value> &cell_map);
    Value evalRefLike(const ASTNode *node, std::map<RC, Value> &cell_map);
};
#include "GPFETypes.h"
#include "EvalTypes.h"
#include <map>

class Evaluator
{
public:
    Value evaluateNode(const ASTNode *node, EvalNeed need, EvalContext &evalCtx);
    Value evalScalar(const ASTNode *node, EvalContext &evalCtx);
    Value evalRefLike(const ASTNode *node, EvalContext &evalCtx);
};
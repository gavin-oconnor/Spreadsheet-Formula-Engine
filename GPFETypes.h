#ifndef GPFE_TYPES_H
#define GPFE_TYPES_H

#include <string>
#include <vector>
#include <memory>
#include <variant>

struct Span
{
    int start;
    int end;
};

enum class BaseType
{
    Number,
    Bool,
    String,
    CellRef,
    Range,
    Unknown,
    Error
};

struct TypeInfo
{
    BaseType type;
};

enum TOKEN_TYPE
{
    REFERENCE_TOKEN,
    NUMBER_TOKEN,
    STRING_TOKEN,
    COMMA_TOKEN,
    PREFIX_ADD_OPERATOR_TOKEN,
    PREFIX_SUB_OPERATOR_TOKEN,
    ADD_OPERATOR_TOKEN,
    SUB_OPERATOR_TOKEN,
    MULT_OPERATOR_TOKEN,
    DIV_OPERATOR_TOKEN,
    POW_OPERATOR_TOKEN,
    LESS_OPERATOR_TOKEN,
    GREATER_OPERATOR_TOKEN,
    EQ_OPERATOR_TOKEN,
    NEQ_OPERATOR_TOKEN,
    GEQ_OPERATOR_TOKEN,
    LEQ_OPERATOR_TOKEN,
    RANGE_OPERATOR_TOKEN,
    CONCAT_OPERATOR_TOKEN,
    PERCENT_OPERATOR_TOKEN,
    IDENT_TOKEN,
    LPAREN_TOKEN,
    RPAREN_TOKEN,
    EOF_TOKEN
};

enum class LiteralType
{
    String,
    Numeric,
};

enum class UnaryOp
{
    Plus,    // +x
    Minus,   // -x
    Percent, // x%
};

enum class BinaryOp
{
    Add,
    Sub,
    Mul,
    Div,
    Pow,
    Less,
    Greater,
    Eq,
    Neq,
    Leq,
    Geq,
    Range,  // :
    Concat, // &
};

enum class ReferenceType
{
    Cell,
    Range,
};

enum class ASTNodeType
{
    Literal,
    Reference,
    Unary,
    Binary,
    FunctionCall,
};

struct Literal
{
    LiteralType type;
    std::variant<std::string, double> value;
};

struct CellReference
{
    int row;
    int col;
};

struct RangeReference
{
    int top;
    int left;
    int bottom;
    int right;
};

struct Reference
{
    ReferenceType type;
    std::variant<CellReference, RangeReference> ref;
};

struct ASTNode;

struct UnaryOperation
{
    UnaryOp op;
    std::unique_ptr<ASTNode> operand;
};

struct BinaryOperation
{
    BinaryOp op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
};

struct FunctionCall
{
    std::string identifier;
    std::vector<std::unique_ptr<ASTNode>> args;
};

struct ASTNode
{
    ASTNodeType type;
    std::variant<Literal, Reference, UnaryOperation, BinaryOperation, FunctionCall> node;
    TypeInfo inferredType;
};

struct Token
{
    TOKEN_TYPE type;
    std::string token_type_string;
    std::string token;
    Span span;
};

#endif
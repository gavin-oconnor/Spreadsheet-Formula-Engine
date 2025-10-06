#ifndef GPFE_HELPERS_H
#define GPFE_HELPERS_H

#include <string>
#include "GPFETypes.h"

inline int colLetterToNumber(const std::string &col)
{
    int result = 0;
    for (char c : col)
    {
        if (!std::isalpha(c))
            continue; // skip non-letters if you want safety
        char upper = std::toupper(c);
        int digit = (upper - 'A' + 1); // A=1, B=2, ..., Z=2
        result = result * 26 + digit;
    }
    return result;
}

inline std::string BinaryOpToString(BinaryOp op)
{
    switch (op)
    {
    case BinaryOp::Pow:
        return "POW";
    case BinaryOp::Mul:
        return "MUL";
    case BinaryOp::Div:
        return "DIV";
    case BinaryOp::Add:
        return "ADD";
    case BinaryOp::Sub:
        return "SUB";
    case BinaryOp::Concat:
        return "CONCAT";
    case BinaryOp::Eq:
        return "EQUAL";
    case BinaryOp::Neq:
        return "NOT EQUAL";
    case BinaryOp::Less:
        return "LESS THAN";
    case BinaryOp::Greater:
        return "GREATER THAN";
    case BinaryOp::Geq:
        return "GREATER THAN EQUAL";
    case BinaryOp::Leq:
        return "LESS THAN EQUAL";
    case BinaryOp::Range:
        return "RANGE";
    }
    return "";
}

inline std::string UnaryOpToString(UnaryOp op)
{
    switch (op)
    {
    case UnaryOp::Plus:
        return "PLUS";
    case UnaryOp::Minus:
        return "MINUS";
    case UnaryOp::Percent:
        return "PERCENT";
    }
    return "";
}

inline std::string BaseTypeToString(BaseType base_type)
{
    switch (base_type)
    {
    case BaseType::Bool:
        return "BOOL";
    case BaseType::CellRef:
        return "CellRef";
    case BaseType::Error:
        return "Error";
    case BaseType::Number:
        return "Number";
    case BaseType::Range:
        return "Range";
    case BaseType::String:
        return "String";
    case BaseType::Unknown:
        return "Unknown";
    case BaseType::Blank:
        return "Blank";
    default:
        return "UNIDENTIFIED TYPE";
    }
}

#endif
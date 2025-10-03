#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "GPFETypes.h"

namespace lexemes
{
    constexpr std::string_view valid_after_number = ",)+-/*^%&";
    constexpr std::string_view valid_after_indentifier = ",()+-/*^:";
    constexpr std::string_view valid_after_reference = ",:)+-/*^>=<";
    constexpr std::string_view operators = "+-/*^:&";
    constexpr std::string_view binary_math_operators = "/*^";
    constexpr std::string_view comparison_operators = "<=>=<>";
}
class Lexer
{
public:
    Lexer(const std::string &input) : input_(input) {};
    std::vector<Token> tokenize();

private:
    std::string_view input_;
};

#endif
#ifndef PARSER_H
#define PARSER_H

#include "GPFETypes.h"

class Parser
{
public:
    Parser(const std::vector<Token> &tokens) : tokens_(tokens), position_(0) {};
    ~Parser() = default;
    ASTNode parse();

private:
    const std::vector<Token> &tokens_;
    size_t position_;
    const Token &peek(int offset = 0) const;
    Token consume();
    ASTNode nud(const Token &token);
    ASTNode led(const Token &token, std::unique_ptr<ASTNode> left);
    ASTNode parse_expression(int binding_power);
    bool at_end() const { return position_ >= tokens_.size() || peek().type == EOF_TOKEN; }
};

inline int operator_precedence(TOKEN_TYPE t)
{
    switch (t)
    {
    case PERCENT_OPERATOR_TOKEN:
        return 100;
    case POW_OPERATOR_TOKEN:
        return 90;
    case PREFIX_ADD_OPERATOR_TOKEN:
        return 80;
    case PREFIX_SUB_OPERATOR_TOKEN:
        return 80;
    case MULT_OPERATOR_TOKEN:
        return 70;
    case DIV_OPERATOR_TOKEN:
        return 70;
    case ADD_OPERATOR_TOKEN:
        return 60;
    case SUB_OPERATOR_TOKEN:
        return 60;
    case CONCAT_OPERATOR_TOKEN:
        return 50;
    case LESS_OPERATOR_TOKEN:
        return 40;
    case GREATER_OPERATOR_TOKEN:
        return 40;
    case GEQ_OPERATOR_TOKEN:
        return 40;
    case LEQ_OPERATOR_TOKEN:
        return 40;
    case EQ_OPERATOR_TOKEN:
        return 40;
    case NEQ_OPERATOR_TOKEN:
        return 40;
    case RANGE_OPERATOR_TOKEN:
        return 30;
    default:
        return 0;
    }
    return 0;
}

#endif
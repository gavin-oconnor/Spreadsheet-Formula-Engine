#include "Lexer.h"
#include <iostream>

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;
    int open_parens = 0;
    for (int i = 0; i < input_.size(); i++)
    {
        if (input_[i] == '"')
        {
            int start = i;
            bool string_closed = false;
            std::string value = "";
            i++;
            while (i < input_.size())
            {
                // possible ending
                if (input_[i] == '"')
                {
                    // check for escaped quote
                    if (i != input_.size() - 1 && input_[i + 1] == '"')
                    {
                        // escaped quote
                        value.push_back('"');
                        i += 2;
                    }
                    else
                    {
                        string_closed = true;
                        tokens.push_back({STRING_TOKEN, "STRING", value, {start, i}});
                        break;
                    }
                }
                else
                {
                    value.push_back(input_[i]);
                    i += 1;
                }
            }
            if (!string_closed)
                throw std::runtime_error("STRING NOT TERMINATED");
        }
        if (isdigit(input_[i]) || input_[i] == '.')
        {
            int start = i;
            bool token_pushed = false;
            bool exponent_last = false;
            bool exponent_seen = false;
            bool decimal_last = input_[i] == '.';
            bool decimal_seen = input_[i] == '.';
            std::string value = "";
            value.push_back(input_[i]);
            i++;
            while (i < input_.size())
            {
                // check if valid follower
                if (isdigit(input_[i]))
                {
                    value.push_back(input_[i]);
                    i++;
                    decimal_last = false;
                    exponent_last = false;
                }
                else if (input_[i] == '.')
                {
                    if (exponent_seen)
                        throw std::runtime_error("EXPONENT CANNOT HAVE DECIMAL POINTS");
                    if (decimal_seen)
                        throw std::runtime_error("NUMBER CANNOT HAVE MULTIPLE DECIMAL POINTS");

                    value.push_back('.');
                    decimal_seen = true;
                    decimal_last = true;
                    i++;
                }
                else if (input_[i] == 'e' || input_[i] == 'E')
                {
                    if (decimal_last)
                        throw std::runtime_error("CANNOT USE SCIENTIFIC NOTATION DIRECTLY AFTER DECIMAL");
                    if (exponent_seen)
                        throw std::runtime_error("CANNOT USE SCIENTIFIC NOTATIOn TWICE IN NUMBER");

                    // check that we either have -digit(s) or digit(s)
                    if (i == input_.size() - 1)
                        throw std::runtime_error("CANNOT END NUMBER WITH SCIENTIFIC NOTATION E");
                    value.push_back('E');
                    if (input_[i + 1] == '-')
                    {
                        if (i == input_.size() - 2)
                            throw std::runtime_error("CANNOT END NUMBER WITH SCIENTIFIC NOTATION E");
                        if (!isdigit(input_[i + 2]))
                            throw std::runtime_error("CANNOT END NUMBER WITH SCIENTIFIC NOTATION E- FOLLOWED BY NO DIGIT");
                        value.push_back('-');
                        value.push_back(input_[i + 2]);
                        i += 2;
                    }
                    exponent_seen = true;
                    exponent_last = true;
                    decimal_seen = false;
                    i++;
                }
                else
                {
                    if (i < input_.size())
                    {
                        // either end of string or valid following character
                        if (lexemes::valid_after_number.find(input_[i]) != std::string::npos)
                        {
                            tokens.push_back({NUMBER_TOKEN, "NUMBER", value, {start, i - 1}});
                            token_pushed = true;
                            i--;
                            break;
                        }
                        else
                        {
                            throw std::runtime_error("INVALID NUMBER ENDING");
                        }
                    }
                }
            }
            if (!token_pushed)
            {
                tokens.push_back({NUMBER_TOKEN, "NUMBER", value, {start, i - 1}});
                i--;
            }
        }
        else if (input_[i] == '%')
        {
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH %");
            if (tokens[tokens.size() - 1].type != LPAREN_TOKEN && tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != NUMBER_TOKEN && tokens[tokens.size() - 1].type != RPAREN_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR %");
            tokens.push_back({PERCENT_OPERATOR_TOKEN, "PERCENT OPERATOR", "%", {i, i}});
        }
        else if (input_[i] == ')')
        {
            if (open_parens == 0)
                throw std::runtime_error("CLOSING PAREN WITHOUT OPEN");
            tokens.push_back({RPAREN_TOKEN, "RPAREN", ")", {i, i}});
        }
        else if (input_[i] == '(')
        {
            if (i == input_.size() - 1)
                throw std::runtime_error("CANNOT END FORMULA WITH OPEN PAREN");
            open_parens++;
            tokens.push_back({LPAREN_TOKEN, "LPAREN", "(", {i, i}});
        }
        else if (input_[i] == ',')
        {
            if (open_parens == 0)
                throw std::runtime_error("COMMAS ARE NOT VALID OUTSIDE OF AN EXPRESSION");
            if (!tokens.size())
                throw std::runtime_error("CANNOT START FORMULA WITH COMMA");
            if ((ADD_OPERATOR_TOKEN <= tokens[tokens.size() - 1].type && tokens[tokens.size() - 1].type <= PERCENT_OPERATOR_TOKEN) || tokens[tokens.size() - 1].type == LPAREN_TOKEN)
                throw std::runtime_error("CANNOT FOLLOW OPERATOR OR LPAREN WITH COMMA");
            tokens.push_back({COMMA_TOKEN, "COMMA", ",", {i, i}});
        }
        else if (input_[i] == '=')
        {
            if (i == tokens.size() - 1)
                throw std::runtime_error("CAN'T END EXPRESSION WITH =");
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH =");
            if (tokens[tokens.size() - 1].type == RPAREN_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR =");
            tokens.push_back({EQ_OPERATOR_TOKEN, "EQ OPERATOR", "=", {i, i}});
        }
        else if (input_[i] == '<')
        {
            if (i == tokens.size() - 1)
                throw std::runtime_error("CAN'T END EXPRESSION WITH <");
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH <");
            if (tokens[tokens.size() - 1].type == RPAREN_TOKEN && tokens[tokens.size() - 1].type == STRING_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR <");
            if (input_[i + 1] == '>')
            {
                i++;
                if (i == tokens.size() - 1)
                    throw std::runtime_error("CAN'T END EXPRESSION WITH <");
                tokens.push_back({NEQ_OPERATOR_TOKEN, "NEQ OPERATOR", "<>", {i - 1, i}});
            }
            else if (input_[i + 1] == '=')
            {
                i++;
                if (i == tokens.size() - 1)
                    throw std::runtime_error("CAN'T END EXPRESSION WITH <=");
                tokens.push_back({LEQ_OPERATOR_TOKEN, "LEQ OPERATOR", "<=", {i - 1, i}});
            }
            else
            {
                tokens.push_back({LESS_OPERATOR_TOKEN, "LESS OPERATOR", "<", {i, i}});
            }
        }
        else if (input_[i] == '>')
        {
            if (i == tokens.size() - 1)
                throw std::runtime_error("CAN'T END EXPRESSION WITH <");
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH <");
            if (tokens[tokens.size() - 1].type == RPAREN_TOKEN && tokens[tokens.size() - 1].type == STRING_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR <");
            else if (input_[i + 1] == '=')
            {
                i++;
                if (i == tokens.size() - 1)
                    throw std::runtime_error("CAN'T END EXPRESSION WITH >=");
                tokens.push_back({GEQ_OPERATOR_TOKEN, "GEQ OPERATOR", ">=", {i - 1, i}});
            }
            else
            {
                tokens.push_back({GREATER_OPERATOR_TOKEN, "GREATER OPERATOR", ">", {i, i}});
            }
        }
        else if (lexemes::operators.find(input_[i]) != std::string::npos)
        {
            // /*^"
            if (lexemes::binary_math_operators.find(input_[i]) != std::string::npos)
            {
                if (!tokens.size())
                    throw std::runtime_error("BINARY OPERATOR MUST HAVE LEFT OPERAND");
                if (tokens[tokens.size() - 1].type != NUMBER_TOKEN && tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != RPAREN_TOKEN)
                    throw std::runtime_error("INVALID LEFT OPERATOR FOR BINARY OPERAND");
                std::string token;
                token.push_back(input_[i]);
                switch (input_[i])
                {
                case '/':
                    tokens.push_back({DIV_OPERATOR_TOKEN, "DIV OPERATOR", token, {i, i}});
                    break;
                case '*':
                    tokens.push_back({MULT_OPERATOR_TOKEN, "MULT OPERATOR", token, {i, i}});
                    break;
                case '^':
                    tokens.push_back({POW_OPERATOR_TOKEN, "POW OPERATOR", token, {i, i}});
                    break;
                }
            }
            else
            {
                if (input_[i] == '&')
                {
                    if (!tokens.size())
                        throw std::runtime_error("CONCAT OPERATOR MUST HAVE LEFT OPERAND");
                    if (tokens[tokens.size() - 1].type != NUMBER_TOKEN && tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != RPAREN_TOKEN && tokens[tokens.size() - 1].type != STRING_TOKEN)
                        throw std::runtime_error("INVALID LEFT OPERATOR FOR CONCAT OPERATION");
                    std::string token;
                    token.push_back(input_[i]);
                    tokens.push_back({CONCAT_OPERATOR_TOKEN, "CONCAT OPERATOR", token, {i, i}});
                }
                if (input_[i] == ':')
                {
                    if (!tokens.size())
                        throw std::runtime_error("CELL RANGE APPEND OPERATOR NEEDS LEFT OPERAND");
                    if (tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != RPAREN_TOKEN)
                        throw std::runtime_error("CELL RANGE APPEND OPERATOR NEEDS LEFT OPERAND (OF TYPE CELL REFERENCE_TOKEN OR EXPRESSION THAT RESOLVES TO CELL REFERENCE_TOKEN)");
                    std::string token;
                    token.push_back(input_[i]);
                    tokens.push_back({RANGE_OPERATOR_TOKEN, "RANGE OPERATOR", token, {i, i}});
                }
                else if (input_[i] == '+' || input_[i] == '-')
                {
                    if (tokens.size() && tokens[tokens.size() - 1].type == STRING_TOKEN)
                        throw std::runtime_error("STRING CANNOT BE FOLLOWED BY +/-");
                    std::string token;
                    token.push_back(input_[i]);
                    if (input_[i] == '+')
                        tokens.push_back({ADD_OPERATOR_TOKEN, "ADD OPERATOR", token, {i, i}});
                    else
                        tokens.push_back({SUB_OPERATOR_TOKEN, "ADD OPERATOR", token, {i, i}});
                }
            }
        }
        // cell ref or indentifier
        else if (('a' <= input_[i] && input_[i] <= 'z') || ('A' <= input_[i] && input_[i] <= 'Z'))
        {
            int start = i;
            bool token_pushed = false;
            bool is_cell_ref = false;
            std::string value;
            bool should_break_outside = false;
            value.push_back(input_[i]);
            i++;
            while (i < input_.size())
            {
                if (isdigit(input_[i]))
                {
                    // cell ref
                    is_cell_ref = true;
                    value.push_back(input_[i]);
                    i++;
                    while (i < input_.size())
                    {
                        if (isdigit(input_[i]))
                        {
                            value.push_back(input_[i]);
                            i++;
                        }
                        else if (lexemes::valid_after_reference.find(input_[i]) != std::string::npos)
                        {
                            tokens.push_back({REFERENCE_TOKEN, "REFERENCE_TOKEN", value, {start, i - 1}});
                            i--;
                            should_break_outside = true;
                            token_pushed = true;
                            break;
                        }
                        else
                            throw std::runtime_error("INVALID CELL REFERENCE_TOKEN");
                    }
                }
                else if (('a' <= input_[i] && input_[i] <= 'z') || ('A' <= input_[i] && input_[i] <= 'Z'))
                {
                    // identifier
                    value.push_back(input_[i]);
                    i++;
                }
                else
                {
                    if (lexemes::valid_after_indentifier.find(input_[i]) != std::string::npos)
                    {
                        tokens.push_back({IDENT_TOKEN, "IDENT", value, {start, i - 1}});
                        i--;
                        token_pushed = true;
                        break;
                    }
                    else
                        throw std::runtime_error("INVALID FOLLOWER TO IDENTIFIER");
                    // end of identifier, valid followers are operators, comma, and both paren
                }
                if (should_break_outside)
                    break;
            }
            if (!token_pushed)
            {
                if (is_cell_ref)
                {
                    tokens.push_back({REFERENCE_TOKEN, "REFERENCE_TOKEN", value, {start, i}});
                }
                else
                {
                    tokens.push_back({IDENT_TOKEN, "IDENT", value, {start, i}});
                }
            }
        }
    }
    tokens.push_back({EOF_TOKEN, "EOF", "EOF", {-1, -1}});
    return tokens;
}
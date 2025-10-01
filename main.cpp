#include <regex>
#include <vector>
#include <iostream>
#include <stdexcept>

enum TOKEN_TYPE
{
    REFERENCE,
    NUMBER, // done
    STRING, // done
    COMMA,  // done
    OPERATOR,
    IDENT,
    LPAREN, // done
    RPAREN, // done
};

struct Token
{
    TOKEN_TYPE type;
    std::string token_type_string;
    std::string token;
};

std::string valid_after_number = ",)+-/*^";
// : is not included below because we won't reach that branch with a cell ref
std::string valid_after_indentifier = ",()+-/*^";
std::string valid_after_reference = ",:)+-/*^";
std::string operators = "+-/*^:";
std::string binary_math_operators = "/*^";

// scan based tokenizer

std::vector<Token> tokenize(std::string input)
{
    std::vector<Token> tokens;
    int open_parens = 0;
    for (int i = 0; i < input.size(); i++)
    {
        std::cout << i << " " << input[i] << "\n";
        if (input[i] == '"')
        {
            bool string_closed = false;
            std::string value = "";
            i++;
            while (i < input.size())
            {
                // possible ending
                if (input[i] == '"')
                {
                    // check for escaped quote
                    if (i != input.size() - 1 && input[i + 1] == '"')
                    {
                        // escaped quote
                        value.push_back('"');
                        i += 2;
                    }
                    else
                    {
                        string_closed = true;
                        tokens.push_back({STRING, "STRING", value});
                        i += 1;
                        break;
                    }
                }
                else
                {
                    value.push_back(input[i]);
                    i += 1;
                }
            }
            if (!string_closed)
                throw std::runtime_error("STRING NOT TERMINATED");
        }
        if (isdigit(input[i]) || input[i] == '.')
        {
            bool token_pushed = false;
            bool exponent_last = false;
            bool exponent_seen = false;
            bool decimal_last = input[i] == '.';
            bool decimal_seen = input[i] == '.';
            std::string value = "";
            value.push_back(input[i]);
            i++;
            while (i < input.size())
            {
                // check if valid follower
                //||  || (!decimal_last && (input[i] == 'e' || input[i] == 'E')))
                if (isdigit(input[i]))
                {
                    value.push_back(input[i]);
                    i++;
                    decimal_last = false;
                    exponent_last = false;
                }
                else if (input[i] == '.')
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
                else if (input[i] == 'e' || input[i] == 'E')
                {
                    if (decimal_last)
                        throw std::runtime_error("CANNOT USE SCIENTIFIC NOTATION DIRECTLY AFTER DECIMAL");
                    if (exponent_seen)
                        throw std::runtime_error("CANNOT USE SCIENTIFIC NOTATIOn TWICE IN NUMBER");

                    // check that we either have -digit(s) or digit(s)
                    if (i == input.size() - 1)
                        throw std::runtime_error("CANNOT END NUMBER WITH SCIENTIFIC NOTATION E");
                    value.push_back('E');
                    if (input[i + 1] == '-')
                    {
                        if (i == input.size() - 2)
                            throw std::runtime_error("CANNOT END NUMBER WITH SCIENTIFIC NOTATION E");
                        if (!isdigit(input[i + 2]))
                            throw std::runtime_error("CANNOT END NUMBER WITH SCIENTIFIC NOTATION E- FOLLOWED BY NO DIGIT");
                        value.push_back('-');
                        value.push_back(input[i + 2]);
                        i += 2;
                    }
                    exponent_seen = true;
                    exponent_last = true;
                    decimal_seen = false;
                    i++;
                }
                else
                {
                    if (i < input.size())
                    {
                        if (input[i] == '%')
                        {
                            // either end of string or valid following character
                            if (i == input.size() - 1 || valid_after_number.find(input[i + 1]) != std::string::npos)
                            {
                                value.push_back('%');
                                tokens.push_back({NUMBER, "NUMBER", value});
                                token_pushed = true;
                                break;
                            }
                            else
                            {
                                throw std::runtime_error("INVALID NUMBER ENDING");
                            }
                        }
                        else if (valid_after_number.find(input[i]) != std::string::npos)
                        {
                            tokens.push_back({NUMBER, "NUMBER", value});
                            token_pushed = true;
                            i--;
                            break;
                        }
                        else
                        {
                            throw std::runtime_error("INVALID NUMBER ENDING");
                        }
                    }
                    // skip whitespace then check chars for valid followers to a number
                }
            }
            if (!token_pushed)
            {
                tokens.push_back({NUMBER, "NUMBER", value});
                i--;
            }
        }
        else if (input[i] == ')')
        {
            if (open_parens == 0)
                throw std::runtime_error("CLOSING PAREN WITHOUT OPEN");
            tokens.push_back({RPAREN, "RPAREN", ")"});
        }
        else if (input[i] == '(')
        {
            if (i == input.size() - 1)
                throw std::runtime_error("CANNOT END FORMULA WITH OPEN PAREN");
            open_parens++;
            tokens.push_back({LPAREN, "LPAREN", "("});
        }
        else if (input[i] == ',')
        {
            if (open_parens == 0)
                throw std::runtime_error("COMMAS ARE NOT VALID OUTSIDE OF AN EXPRESSION");
            if (!tokens.size())
                throw std::runtime_error("CANNOT START FORMULA WITH COMMA");
            if (tokens[tokens.size() - 1].type == OPERATOR || tokens[tokens.size() - 1].type == LPAREN)
                throw std::runtime_error("CANNOT FOLLOW OPERATOR OR LPAREN WITH COMMA");
            tokens.push_back({COMMA, "COMMA", ","});
        }
        else if (operators.find(input[i]) != std::string::npos)
        {
            if (binary_math_operators.find(input[i]) != std::string::npos)
            {
                if (!tokens.size())
                    throw std::runtime_error("BINARY OPERATOR MUST HAVE LEFT OPERAND");
                if (tokens[tokens.size() - 1].type != NUMBER && tokens[tokens.size() - 1].type != REFERENCE && tokens[tokens.size() - 1].type != RPAREN)
                    throw std::runtime_error("INVALID LEFT OPERATOR FOR BINARY OPERAND");
                std::string token;
                token.push_back(input[i]);
                tokens.push_back({OPERATOR, "OPERATOR", token});
            }
            else
            {
                if (input[i] == ':')
                {
                    if (!tokens.size())
                        throw std::runtime_error("CELL RANGE APPEND OPERATOR NEEDS LEFT OPERAND");
                    if (tokens[tokens.size() - 1].type != REFERENCE)
                        throw std::runtime_error("CELL RANGE APPEND OPERATOR NEEDS LEFT OPERAND (OF TYPE CELL REFERENCE)");
                    std::string token;
                    token.push_back(input[i]);
                    tokens.push_back({OPERATOR, "OPERATOR", token});
                }
                else if (input[i] == '+' || input[i] == '-')
                {
                    if (tokens.size() && tokens[tokens.size() - 1].type == STRING)
                        throw std::runtime_error("STRING CANNOT BE FOLLOWED BY +/-");
                    std::string token;
                    token.push_back(input[i]);
                    tokens.push_back({OPERATOR, "OPERATOR", token});
                }
            }
        }
        // cell ref or indentifier
        else if (('a' <= input[i] && input[i] <= 'z') || ('A' <= input[i] && input[i] <= 'Z'))
        {
            bool token_pushed = false;
            bool is_cell_ref = false;
            std::string value;
            bool should_break_outside = false;
            value.push_back(input[i]);
            i++;
            while (i < input.size())
            {
                std::cout << i << " " << input[i] << "\n";
                if (isdigit(input[i]))
                {
                    std::cout << "branch1\n";
                    // cell ref
                    is_cell_ref = true;
                    value.push_back(input[i]);
                    i++;
                    while (i < input.size())
                    {
                        if (isdigit(input[i]))
                        {
                            value.push_back(input[i]);
                            i++;
                        }
                        else if (valid_after_reference.find(input[i]) != std::string::npos)
                        {
                            tokens.push_back({REFERENCE, "REFERENCE", value});
                            i--;
                            should_break_outside = true;
                            token_pushed = true;
                            break;
                        }
                        else
                            throw std::runtime_error("INVALID CELL REFERENCE");
                    }
                }
                else if (('a' <= input[i] && input[i] <= 'z') || ('A' <= input[i] && input[i] <= 'Z'))
                {
                    std::cout << "branch2\n";
                    // identifier
                    value.push_back(input[i]);
                    i++;
                }
                else
                {
                    std::cout << "branch3\n";
                    if (valid_after_indentifier.find(input[i]) != std::string::npos)
                    {
                        tokens.push_back({IDENT, "IDENT", value});
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
                    tokens.push_back({REFERENCE, "REFERENCE", value});
                }
                else
                {
                    tokens.push_back({IDENT, "IDENT", value});
                }
            }
        }
    }
    return tokens;
}

int main()
{
    std::vector<Token> tokens = tokenize("A1");
    for (int i = 0; i < tokens.size(); i++)
    {
        std::cout << tokens[i].token_type_string << " " << tokens[i].token << "\n";
    }
}
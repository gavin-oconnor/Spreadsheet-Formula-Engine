#include <regex>
#include <vector>
#include <variant>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <cctype>
#include "GPFETypes.h"
#include "Parser.h"
#include <format>
#include "GPFEHelpers.h"

// need literal, function call, operator, reference

std::string valid_after_number = ",)+-/*^%&";
// : is not included below because we won't reach that branch with a cell ref
std::string valid_after_indentifier = ",()+-/*^:";
std::string valid_after_reference = ",:)+-/*^>=<";
std::string operators = "+-/*^:&";
std::string binary_math_operators = "/*^";
std::string comparison_operators = "<=>=<>";

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
                        tokens.push_back({STRING_TOKEN, "STRING", value});
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
                        // either end of string or valid following character
                        if (i == input.size() - 1)
                        {
                            value.push_back('%');
                            tokens.push_back({NUMBER_TOKEN, "NUMBER", value});
                            token_pushed = true;
                            break;
                        }
                        else if (valid_after_number.find(input[i]) != std::string::npos)
                        {
                            tokens.push_back({NUMBER_TOKEN, "NUMBER", value});
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
                tokens.push_back({NUMBER_TOKEN, "NUMBER", value});
                i--;
            }
        }
        else if (input[i] == '%')
        {
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH %");
            if (tokens[tokens.size() - 1].type != LPAREN_TOKEN && tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != NUMBER_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR %");
            tokens.push_back({PERCENT_OPERATOR_TOKEN, "PERCENT OPERATOR", "%"});
        }
        else if (input[i] == ')')
        {
            if (open_parens == 0)
                throw std::runtime_error("CLOSING PAREN WITHOUT OPEN");
            tokens.push_back({RPAREN_TOKEN, "RPAREN", ")"});
        }
        else if (input[i] == '(')
        {
            if (i == input.size() - 1)
                throw std::runtime_error("CANNOT END FORMULA WITH OPEN PAREN");
            open_parens++;
            tokens.push_back({LPAREN_TOKEN, "LPAREN", "("});
        }
        else if (input[i] == ',')
        {
            if (open_parens == 0)
                throw std::runtime_error("COMMAS ARE NOT VALID OUTSIDE OF AN EXPRESSION");
            if (!tokens.size())
                throw std::runtime_error("CANNOT START FORMULA WITH COMMA");
            if ((ADD_OPERATOR_TOKEN <= tokens[tokens.size() - 1].type && tokens[tokens.size() - 1].type <= PERCENT_OPERATOR_TOKEN) || tokens[tokens.size() - 1].type == LPAREN_TOKEN)
                throw std::runtime_error("CANNOT FOLLOW OPERATOR OR LPAREN WITH COMMA");
            tokens.push_back({COMMA_TOKEN, "COMMA", ","});
        }
        else if (input[i] == '=')
        {
            if (i == tokens.size() - 1)
                throw std::runtime_error("CAN'T END EXPRESSION WITH =");
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH =");
            if (tokens[tokens.size() - 1].type == RPAREN_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR =");
            tokens.push_back({EQ_OPERATOR_TOKEN, "EQ OPERATOR", "="});
        }
        else if (input[i] == '<')
        {
            if (i == tokens.size() - 1)
                throw std::runtime_error("CAN'T END EXPRESSION WITH <");
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH <");
            if (tokens[tokens.size() - 1].type == RPAREN_TOKEN && tokens[tokens.size() - 1].type == STRING_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR <");
            if (input[i + 1] == '>')
            {
                i++;
                if (i == tokens.size() - 1)
                    throw std::runtime_error("CAN'T END EXPRESSION WITH <");
                tokens.push_back({NEQ_OPERATOR_TOKEN, "NEQ OPERATOR", "<>"});
            }
            else if (input[i + 1] == '=')
            {
                i++;
                if (i == tokens.size() - 1)
                    throw std::runtime_error("CAN'T END EXPRESSION WITH <=");
                tokens.push_back({LEQ_OPERATOR_TOKEN, "LEQ OPERATOR", "<="});
            }
            else
            {
                tokens.push_back({LESS_OPERATOR_TOKEN, "LESS OPERATOR", "<"});
            }
        }
        else if (input[i] == '>')
        {
            if (i == tokens.size() - 1)
                throw std::runtime_error("CAN'T END EXPRESSION WITH <");
            if (!tokens.size())
                throw std::runtime_error("CAN'T START EXPRESSION WITH <");
            if (tokens[tokens.size() - 1].type == RPAREN_TOKEN && tokens[tokens.size() - 1].type == STRING_TOKEN)
                throw std::runtime_error("INCORRECT LEFT OPERAND FOR <");
            else if (input[i + 1] == '=')
            {
                i++;
                if (i == tokens.size() - 1)
                    throw std::runtime_error("CAN'T END EXPRESSION WITH >=");
                tokens.push_back({GEQ_OPERATOR_TOKEN, "GEQ OPERATOR", ">="});
            }
            else
            {
                tokens.push_back({GREATER_OPERATOR_TOKEN, "GREATER OPERATOR", ">"});
            }
        }
        else if (operators.find(input[i]) != std::string::npos)
        {
            // /*^&"
            if (binary_math_operators.find(input[i]) != std::string::npos)
            {
                if (!tokens.size())
                    throw std::runtime_error("BINARY OPERATOR MUST HAVE LEFT OPERAND");
                if (tokens[tokens.size() - 1].type != NUMBER_TOKEN && tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != RPAREN_TOKEN)
                    throw std::runtime_error("INVALID LEFT OPERATOR FOR BINARY OPERAND");
                std::string token;
                token.push_back(input[i]);
                switch (input[i])
                {
                case '/':
                    tokens.push_back({DIV_OPERATOR_TOKEN, "DIV OPERATOR", token});
                    break;
                case '*':
                    tokens.push_back({MULT_OPERATOR_TOKEN, "MULT OPERATOR", token});
                    break;
                case '^':
                    tokens.push_back({POW_OPERATOR_TOKEN, "POW OPERATOR", token});
                    break;
                }
            }
            else
            {
                if (input[i] == '&')
                {
                    if (!tokens.size())
                        throw std::runtime_error("CONCAT OPERATOR MUST HAVE LEFT OPERAND");
                    if (tokens[tokens.size() - 1].type != NUMBER_TOKEN && tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != RPAREN_TOKEN && tokens[tokens.size() - 1].type != STRING_TOKEN)
                        throw std::runtime_error("INVALID LEFT OPERATOR FOR CONCAT OPERATION");
                    std::string token;
                    token.push_back(input[i]);
                    tokens.push_back({CONCAT_OPERATOR_TOKEN, "CONCAT OPERATOR", token});
                }
                if (input[i] == ':')
                {
                    if (!tokens.size())
                        throw std::runtime_error("CELL RANGE APPEND OPERATOR NEEDS LEFT OPERAND");
                    if (tokens[tokens.size() - 1].type != REFERENCE_TOKEN && tokens[tokens.size() - 1].type != RPAREN_TOKEN)
                        throw std::runtime_error("CELL RANGE APPEND OPERATOR NEEDS LEFT OPERAND (OF TYPE CELL REFERENCE_TOKEN OR EXPRESSION THAT RESOLVES TO CELL REFERENCE_TOKEN)");
                    std::string token;
                    token.push_back(input[i]);
                    tokens.push_back({RANGE_OPERATOR_TOKEN, "RANGE OPERATOR", token});
                }
                else if (input[i] == '+' || input[i] == '-')
                {
                    if (tokens.size() && tokens[tokens.size() - 1].type == STRING_TOKEN)
                        throw std::runtime_error("STRING CANNOT BE FOLLOWED BY +/-");
                    std::string token;
                    token.push_back(input[i]);
                    if (input[i] == '+')
                        tokens.push_back({ADD_OPERATOR_TOKEN, "ADD OPERATOR", token});
                    else
                        tokens.push_back({SUB_OPERATOR_TOKEN, "ADD OPERATOR", token});
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
                            tokens.push_back({REFERENCE_TOKEN, "REFERENCE_TOKEN", value});
                            i--;
                            should_break_outside = true;
                            token_pushed = true;
                            break;
                        }
                        else
                            throw std::runtime_error("INVALID CELL REFERENCE_TOKEN");
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
                        tokens.push_back({IDENT_TOKEN, "IDENT", value});
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
                    tokens.push_back({REFERENCE_TOKEN, "REFERENCE_TOKEN", value});
                }
                else
                {
                    tokens.push_back({IDENT_TOKEN, "IDENT", value});
                }
            }
        }
    }
    tokens.push_back({EOF_TOKEN, "EOF", "EOF"});
    return tokens;
}

// function print_ast(node, indent):
//     pad = " " * indent
//     if node is Literal:
//         print(pad + "Literal(" + value + ")")
//     elif node is Reference:
//         print(pad + "Reference(" + cell + ")")
//     elif node is Unary:
//         print(pad + "Unary(" + op + ")")
//         print_ast(node.operand, indent + 2)
//     elif node is Binary:
//         print(pad + "Binary(" + op + ")")
//         print_ast(node.left, indent + 2)
//         print_ast(node.right, indent + 2)
//     elif node is FunctionCall:
//         print(pad + "Function(" + name + ")")
//         for arg in node.args:
//             print_ast(arg, indent + 2)

void print_ast(ASTNode *node, int indent)
{
    std::string padding;
    for (int i = 0; i < indent; i++)
    {
        padding.push_back(' ');
    }
    switch (node->type)
    {
    case ASTNodeType::Literal:
    {
        const auto &lit = std::get<Literal>(node->node);
        if (lit.type == LiteralType::Numeric)
        {
            std::cout << std::format("{}LITERAL({})\n", padding, std::get<double>(lit.value));
        }
        else
        {
            std::cout << std::format("{}LITERAL({})\n", padding, std::get<std::string>(lit.value));
        }
        return;
    }
    case ASTNodeType::Binary:
    {
        const auto &binary_op = std::get<BinaryOperation>(node->node);
        std::cout << std::format("{}OPERATOR({})\n", padding, BinaryOpToString(binary_op.op));
        print_ast(binary_op.left.get(), 2);
        print_ast(binary_op.right.get(), 2);
        return;
    }
    case ASTNodeType::Unary:
    {
        const auto &unary_op = std::get<UnaryOperation>(node->node);
        std::cout << std::format("{}OPERATOR({})\n", padding, UnaryOpToString(unary_op.op));
        print_ast(unary_op.operand.get(), 2);
        return;
    }
    case ASTNodeType::FunctionCall:
    {
        const auto &function_call = std::get<FunctionCall>(node->node);
        std::cout << std::format("{}FUNCTION({})\n", padding, function_call.identifier);
        for (int i = 0; i < function_call.args.size(); i++)
        {
            print_ast(function_call.args[i].get(), 2);
        }
        return;
    }
    case ASTNodeType::Reference:
    {
        const auto &reference = std::get<Reference>(node->node);
        if (reference.type == ReferenceType::Cell)
        {
            const auto &cell_ref = std::get<CellReference>(reference.ref);
            std::cout << std::format("{}REFERENCE({},{})\n", padding, cell_ref.row, cell_ref.col);
        }
        return;
    }
    default:
        return;
    }
}

int main()
{
    std::vector<Token> tokens = tokenize("5&\"Hello\"");
    for (int i = 0; i < tokens.size(); i++)
    {
        std::cout << tokens[i].token_type_string << " " << tokens[i].token << "\n";
    }
    Parser parser(tokens);
    ASTNode root = parser.parse();
    print_ast(&root, 0);

    return 0;
}
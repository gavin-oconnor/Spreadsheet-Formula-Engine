#include <regex>
#include <vector>
#include <variant>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <cctype>

int colLetterToNumber(const std::string &col)
{
    int result = 0;
    for (char c : col)
    {
        if (!std::isalpha(c))
            continue; // skip non-letters if you want safety
        char upper = std::toupper(c);
        int digit = (upper - 'A' + 1); // A=1, B=2, ..., Z=26
        result = result * 26 + digit;
    }
    return result;
}

// Add,
//     Sub,
//     Mul,
//     Div,
//     Pow,
//     Less,
//     Greater,
//     Eq,
//     Neq,
//     Leq,
//     Geq,
//     Range,  // :
//     Concat, // &

enum TOKEN_TYPE
{
    REFERENCE_TOKEN,
    NUMBER_TOKEN,
    STRING_TOKEN,
    COMMA_TOKEN,
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
    LPAREN_TOKEN, // done
    RPAREN_TOKEN, // done
};

// need literal, function call, operator, reference

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

// === Leaf payloads ===
struct Literal
{
    LiteralType type;
    std::variant<std::string, double> value;
};

struct CellReference
{
    int row;
    int col;
    // add absolute flags later if needed
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
    ASTNode *operand;
};

struct BinaryOperation
{
    BinaryOp op;
    ASTNode *left;
    ASTNode *right;
};

struct FunctionCall
{
    std::string identifier;
    std::vector<ASTNode *> args;
};

struct ASTNode
{
    ASTNodeType type;
    std::variant<Literal, Reference, UnaryOperation, BinaryOperation, FunctionCall> node;
};

struct Token
{
    TOKEN_TYPE type;
    std::string token_type_string;
    std::string token;
};

ASTNode nud(Token token)
{
    ASTNode node;
    Literal lit;
    Reference ref;
    CellReference cell_ref;
    std::string cell_ref_letters;
    int cell_ref_numbers = 0;
    switch (token.type)
    {
    case NUMBER_TOKEN:
        lit.type = LiteralType::Numeric;
        lit.value = std::stod(token.token);
        node.type = ASTNodeType::Literal;
        node.node = std::move(lit);
        return node;
    case STRING_TOKEN:
        lit.type = LiteralType::String;
        lit.value = std::move(token.token);
        node.type = ASTNodeType::Literal;
        node.node = std::move(lit);
        return node;
    case REFERENCE_TOKEN:
        ref.type = ReferenceType::Cell;

        for (int i = 0; i < token.token.size(); i++)
        {
            // 0 shouldn't be allowed in cell ref but this logic clips it
            if (isdigit(token.token[i]))
            {
                cell_ref_numbers *= 10;
                cell_ref_letters += std::stoi("" + token.token[i]);
            }
            else
            {
                cell_ref_letters.push_back(token.token[i]);
            }
        }
        cell_ref.row = cell_ref_numbers;
        cell_ref.col = colLetterToNumber(cell_ref_letters);
        ref.ref = std::move(cell_ref);
        node.type = ASTNodeType::Reference;
        node.node = std::move(ref);
    case LPAREN_TOKEN:
        static_assert(1 == 2);
        // continue parsing
    case ADD_OPERATOR_TOKEN:
        static_assert(1 == 2);
        // parse next sub expression and wrap
    case SUB_OPERATOR_TOKEN:
        static_assert(1 == 2);
        // parse next sub expression and wrap
    default:
        throw std::runtime_error("PARSING ERROR");
    }
}

std::string valid_after_number = ",)+-/*^%";
// : is not included below because we won't reach that branch with a cell ref
std::string valid_after_indentifier = ",()+-/*^:";
std::string valid_after_reference = ",:)+-/*^>=<";
std::string operators = "+-/*^:";
std::string binary_math_operators = "/*^&";
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
                case '&':
                    tokens.push_back({CONCAT_OPERATOR_TOKEN, "CONCAT OPERATOR", token});
                    break;
                }
            }
            else
            {
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
    return tokens;
}

int operator_precendence(TOKEN_TYPE t)
{
    switch (t)
    {
    case PERCENT_OPERATOR_TOKEN:
        return 100;
    case POW_OPERATOR_TOKEN:
        return 90;
    case MULT_OPERATOR_TOKEN:
        return 80;
    case DIV_OPERATOR_TOKEN:
        return 80;
    case ADD_OPERATOR_TOKEN:
        return 70;
    case SUB_OPERATOR_TOKEN:
        return 70;
    case CONCAT_OPERATOR_TOKEN:
        return 60;
    case LESS_OPERATOR_TOKEN:
        return 50;
    case GREATER_OPERATOR_TOKEN:
        return 50;
    case GEQ_OPERATOR_TOKEN:
        return 50;
    case LEQ_OPERATOR_TOKEN:
        return 50;
    case EQ_OPERATOR_TOKEN:
        return 50;
    case NEQ_OPERATOR_TOKEN:
        return 50;
    case RANGE_OPERATOR_TOKEN:
        return 40;
    }
    return 0;
}

ASTNode parse(std::vector<Token> &tokens)
{
}

int main()
{
    std::vector<Token> tokens = tokenize("INDIRECT(\"A1\"):B2>=5");
    for (int i = 0; i < tokens.size(); i++)
    {
        std::cout << tokens[i].token_type_string << " " << tokens[i].token << "\n";
    }
    return 0;
}
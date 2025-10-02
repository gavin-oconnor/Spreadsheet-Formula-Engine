#include "Parser.h"
#include "GPFEHelpers.h"
#include <memory>
#include <format>
#include <iostream>

Token Parser::consume()
{
    return tokens_[position_++];
}

const Token &Parser::peek(int offset) const
{
    if (offset < 0)
        throw std::runtime_error("OFFSET CANNOT BE LESS THAN ZERO");
    if (position_ + offset >= tokens_.size())
        throw std::runtime_error("POSITION + OFFSET OUT OF RANGE");
    return tokens_[position_ + offset];
}

ASTNode Parser::nud(const Token &token)
{
    ASTNode node;
    switch (token.type)
    {
    case NUMBER_TOKEN:
    {
        Literal lit;
        lit.type = LiteralType::Numeric;
        lit.value = std::stod(token.token);
        node.type = ASTNodeType::Literal;
        node.node = std::move(lit);
        return node;
    }
    case STRING_TOKEN:
    {
        Literal lit;
        lit.type = LiteralType::String;
        lit.value = token.token;
        node.type = ASTNodeType::Literal;
        node.node = std::move(lit);
        return node;
    }
    case REFERENCE_TOKEN:
    {
        Reference ref;
        ref.type = ReferenceType::Cell;

        std::string colLetters;
        int row = 0;

        int i = 0;
        const std::string &s = token.token;
        int size = token.token.size();
        while (i < size && std::isalpha(static_cast<unsigned char>(s[i])))
        {
            colLetters.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(s[i]))));
            ++i;
        }
        if (colLetters.empty())
            throw std::runtime_error("INVALID CELL REF: MISSING COLUMN");
        if (i == s.size() || !std::isdigit(static_cast<unsigned char>(s[i])))
            throw std::runtime_error("INVALID CELL REF: MISSING ROW");
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
        {
            row = row * 10 + (s[i] - '0');
            ++i;
        }
        if (i != s.size())
            throw std::runtime_error("INVALID CELL REF: TRAILING CHARS");
        if (row <= 0)
            throw std::runtime_error("INVALID CELL REF: ROW MUST BE ≥ 1");
        CellReference cell_ref = {row, colLetterToNumber(colLetters)};
        ref.ref = std::move(cell_ref);
        node.type = ASTNodeType::Reference;
        node.node = std::move(ref);
        return node;
    }
    case LPAREN_TOKEN:
    {
        ASTNode inner = parse_expression(0);
        if (peek().type != RPAREN_TOKEN)
            throw std::runtime_error("EXPECTED A CLOSING PARENTHESIS");
        consume(); // eat ')'
        return inner;
    }
    case ADD_OPERATOR_TOKEN:
    {
        UnaryOperation unary_op;
        auto sub = std::make_unique<ASTNode>(parse_expression(operator_precedence(PREFIX_ADD_OPERATOR_TOKEN)));
        unary_op.op = UnaryOp::Plus;
        unary_op.operand = std::move(sub);
        node.type = ASTNodeType::Unary;
        node.node = std::move(unary_op);
        return node;
    }
    case SUB_OPERATOR_TOKEN:
    {
        UnaryOperation unary_op;
        auto sub = std::make_unique<ASTNode>(parse_expression(operator_precedence(PREFIX_SUB_OPERATOR_TOKEN)));
        unary_op.op = UnaryOp::Minus;
        unary_op.operand = std::move(sub);
        node.type = ASTNodeType::Unary;
        node.node = std::move(unary_op);
        return node;
    }
    case IDENT_TOKEN:
    {
        if (peek().type != LPAREN_TOKEN)
            throw std::runtime_error("FUNCTION MUST BE FOLLOWED BY '('");
        consume(); // '('

        FunctionCall call;
        call.identifier = token.token;

        if (peek().type != RPAREN_TOKEN)
        {
            while (true)
            {
                auto arg = std::make_unique<ASTNode>(parse_expression(0));
                call.args.push_back(std::move(arg));

                if (peek().type == COMMA_TOKEN)
                {
                    consume();
                    continue;
                }
                if (peek().type == RPAREN_TOKEN)
                    break;
                throw std::runtime_error("EXPECTED ',' OR ')' IN FUNCTION CALL");
            }
        }
        consume(); // ')'

        node.type = ASTNodeType::FunctionCall;
        node.node = std::move(call);
        return node;
    }

    default:
        throw std::runtime_error("PARSING ERROR");
    }
}

ASTNode Parser::led(const Token &token, std::unique_ptr<ASTNode> left)
{
    ASTNode node;
    auto make_binary = [&](BinaryOp op, int prec)
    {
        ASTNode right = parse_expression(prec);
        BinaryOperation b{op, std::move(left),
                          std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(b);
        return std::move(node);
    };
    switch (token.type)
    {
    case PERCENT_OPERATOR_TOKEN:
    {
        UnaryOperation unary_op{UnaryOp::Percent, std::move(left)};
        node.type = ASTNodeType::Unary;
        node.node = std::move(unary_op);
        return node;
    }
    case POW_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(POW_OPERATOR_TOKEN);
        // bp - 1 because of right associative
        ASTNode right = parse_expression(bp - 1);
        BinaryOperation binary_op{
            BinaryOp::Pow,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case MULT_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Mul, operator_precedence(MULT_OPERATOR_TOKEN));
    }
    case DIV_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Div, operator_precedence(DIV_OPERATOR_TOKEN));
    }
    case ADD_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Add, operator_precedence(ADD_OPERATOR_TOKEN));
    }
    case SUB_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Sub, operator_precedence(SUB_OPERATOR_TOKEN));
    }
    case CONCAT_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Concat, operator_precedence(CONCAT_OPERATOR_TOKEN));
    }
    case LESS_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Less, operator_precedence(LESS_OPERATOR_TOKEN));
    }
    case GREATER_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Greater, operator_precedence(GREATER_OPERATOR_TOKEN));
    }
    case LEQ_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Leq, operator_precedence(LEQ_OPERATOR_TOKEN));
    }
    case GEQ_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Geq, operator_precedence(GEQ_OPERATOR_TOKEN));
    }
    case EQ_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Eq, operator_precedence(EQ_OPERATOR_TOKEN));
    }
    case NEQ_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Neq, operator_precedence(NEQ_OPERATOR_TOKEN));
    }
    case RANGE_OPERATOR_TOKEN:
    {
        return make_binary(BinaryOp::Range, operator_precedence(RANGE_OPERATOR_TOKEN));
    }
    default:
        throw std::runtime_error(std::format("Unexpected Operator of type: {}", token.token));
    }
}

ASTNode Parser::parse()
{
    auto root = parse_expression(0);
    std::cout << position_ << "\n";
    if (!at_end())
    {
        std::cerr << "Remaining token: " << peek().token << " at position " << position_ << "\n";
        throw std::runtime_error("PARSING DID NOT REACH EOF");
    }
    return root;
}

ASTNode Parser::parse_expression(int binding_power)
{
    Token token = consume();
    if (token.type == EOF_TOKEN)
        throw std::runtime_error("Unexpected end of input");
    ASTNode left = nud(token);
    while (true)
    {
        Token next = peek();
        if (next.type == EOF_TOKEN)
            break; // stop at EOF

        int prec = operator_precedence(next.type);
        if (binding_power >= prec)
            break;

        consume();
        left = led(next, std::make_unique<ASTNode>(std::move(left)));
    }

    return left;
}

/*


ASTNode Parser::led(const Token &token, std::unique_ptr<ASTNode> left)
{
    ASTNode node;
    auto make_binary = [&](BinaryOp op, int prec)
    {
        ASTNode right = parse_expression(prec);
        BinaryOperation b{op, std::move(left),
                          std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(b);
        return std::move(node);
    };
    switch (token.type)
    {
    case PERCENT_OPERATOR_TOKEN:
    {
        UnaryOperation unary_op{UnaryOp::Percent, std::move(left)};
        node.type = ASTNodeType::Unary;
        node.node = std::move(unary_op);
        return node;
    }
    case POW_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(POW_OPERATOR_TOKEN);
        // bp - 1 because of right associative
        ASTNode right = parse_expression(bp - 1);
        BinaryOperation binary_op{
            BinaryOp::Pow,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case MULT_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(MULT_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Mul,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case DIV_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(DIV_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Div,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case ADD_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(ADD_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Add,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case SUB_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(SUB_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Sub,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case CONCAT_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(CONCAT_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Concat,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case LESS_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(LESS_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Less,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case GREATER_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(GREATER_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Greater,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case LEQ_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(LEQ_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Leq,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case GEQ_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(GEQ_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Geq,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case EQ_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(EQ_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Eq,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case NEQ_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(NEQ_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Neq,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    case RANGE_OPERATOR_TOKEN:
    {
        int bp = operator_precedence(RANGE_OPERATOR_TOKEN);
        ASTNode right = parse_expression(bp);
        BinaryOperation binary_op{
            BinaryOp::Range,
            std::move(left),
            std::make_unique<ASTNode>(std::move(right))};
        node.type = ASTNodeType::Binary;
        node.node = std::move(binary_op);
        return node;
    }
    default:
        throw std::runtime_error(std::format("Unexpected Operator of type: {}", token.token));
    }
}

*/
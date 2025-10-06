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
#include "Lexer.h"
#include <format>
#include "GPFEHelpers.h"
#include "TypeChecker.h"
#include "Evaluator.h"
#include "EvalTypes.h"

// need literal, function call, operator, reference

// scan based tokenizer

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
            std::cout << std::format("{}LITERAL({}): {}\n", padding, std::get<double>(lit.value), BaseTypeToString(node->inferredType.type));
        }
        else
        {
            std::cout << std::format("{}LITERAL({}): {}\n", padding, std::get<std::string>(lit.value), BaseTypeToString(node->inferredType.type));
        }
        return;
    }
    case ASTNodeType::Binary:
    {
        const auto &binary_op = std::get<BinaryOperation>(node->node);
        std::cout << std::format("{}OPERATOR({}): {}\n", padding, BinaryOpToString(binary_op.op), BaseTypeToString(node->inferredType.type));
        print_ast(binary_op.left.get(), padding.size() + 2);
        print_ast(binary_op.right.get(), padding.size() + 2);
        return;
    }
    case ASTNodeType::Unary:
    {
        const auto &unary_op = std::get<UnaryOperation>(node->node);
        std::cout << std::format("{}OPERATOR({}): {}\n", padding, UnaryOpToString(unary_op.op), BaseTypeToString(node->inferredType.type));
        print_ast(unary_op.operand.get(), padding.size() + 2);
        return;
    }
    case ASTNodeType::FunctionCall:
    {
        const auto &function_call = std::get<FunctionCall>(node->node);
        std::cout << std::format("{}FUNCTION({}): {}\n", padding, function_call.identifier, BaseTypeToString(node->inferredType.type));
        for (int i = 0; i < function_call.args.size(); i++)
        {
            print_ast(function_call.args[i].get(), padding.size() + 2);
        }
        return;
    }
    case ASTNodeType::Reference:
    {
        const auto &reference = std::get<Reference>(node->node);
        if (reference.type == ReferenceType::Cell)
        {
            const auto &cell_ref = std::get<CellReference>(reference.ref);
            std::cout << std::format("{}REFERENCE({},{}): {}\n", padding, cell_ref.row, cell_ref.col, BaseTypeToString(node->inferredType.type));
        }
        return;
    }
    default:
        return;
    }
}

int main()
{
    Lexer lexer("IF(5-5=0,0,5)");
    std::vector<Token> tokens = lexer.tokenize();
    for (int i = 0; i < tokens.size(); i++)
    {
        std::cout << tokens[i].token_type_string << " " << tokens[i].token << "\n";
    }
    Parser parser(tokens);
    ASTNode root = parser.parse();
    TypeChecker type_checker;
    type_checker.infer(&root);
    print_ast(&root, 0);
    Evaluator evaluator;
    EvalContext evalCtx;
    evalCtx.cell_map = new std::map<RC, Value>();
    evalCtx.cell_map->insert(std::make_pair(RC{1, 1}, Number{5}));
    Value evaluated_expr = evaluator.evaluateNode(&root, EvalNeed::Scalar, evalCtx);
    if (std::holds_alternative<double>(evaluated_expr))
    {
        double returned_value = std::get<double>(evaluated_expr);
        std::cout << returned_value << "\n";
    }
    if (std::holds_alternative<bool>(evaluated_expr))
    {
        if (std::get<bool>(evaluated_expr))
            std::cout << "TRUE\n";
        else
            std::cout << "FALSE\n";
    }
    if (std::holds_alternative<std::string>(evaluated_expr))
    {
        std::cout << std::get<std::string>(evaluated_expr) << "\n";
    }
    return 0;
}
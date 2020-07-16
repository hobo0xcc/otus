#pragma once

#include "error.hpp"
#include "lexer.hpp"

#include <cstdlib>
#include <iostream>
#include <utility>

typedef enum NodeType {
    ND_NUMBER,
    ND_FLOAT,
    ND_STRING,
    ND_VAR,
    ND_BIN,
    ND_IF,
    ND_LET_IN,
    ND_LET_FUN,
    ND_LET_EXTERN,
    ND_APP,
    ND_COMPOUND,
    ND_NEW,
    ND_UNARY,
} NodeType;

typedef enum OpType {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_ADDF,
    OP_SUBF,
    OP_MULF,
    OP_DIVF,
    OP_MODF,
    OP_GREATER,
    OP_LESS,
    OP_GREATER_EQ,
    OP_LESS_EQ,
    OP_EQ,
    OP_NOT_EQ,
    OP_PTR_ASSIGN,
    OP_DEREF,
    OP_NOT,
    OP_SEMICOLON,
} OpType;

typedef enum TypeKind {
    TY_INT,
    TY_STRING,
    TY_BOOL,
    TY_FLOAT,
    TY_PTR,
    TY_VOID,
    TY_FUN,
    TY_VAR,
    TY_UNKNOWN,
} TypeKind;

typedef struct Type Type;

struct Type {
    TypeKind kind;
    std::string typevar;
    std::vector<Type *> arg_types;
    Type *ret_type;
    Type *ptr_to;

    Type() : kind{TY_UNKNOWN} {};
    Type(TypeKind kind) : kind{kind} {};
    void print_type() {
        if (kind == TY_VAR) {
            std::cout << typevar;
        } else if (kind == TY_BOOL) {
            std::cout << "bool";
        } else if (kind == TY_INT) {
            std::cout << "int";
        } else if (kind == TY_FLOAT) {
            std::cout << "float";
        } else if (kind == TY_STRING) {
            std::cout << "string";
        } else if (kind == TY_VOID) {
            std::cout << "void";
        } else if (kind == TY_PTR) {
            ptr_to->print_type();
            std::cout << "*";
        } else if (kind == TY_FUN) {
            std::cout << "(";
            for (Type *arg_type : arg_types) {
                arg_type->print_type();
                std::cout << " -> ";
            }
            ret_type->print_type();
            std::cout << ")";
        } else if (kind == TY_UNKNOWN) {
            std::cout << "unknown";
        }
    }
};

void print_op(OpType type);

typedef struct Node Node;

struct Node {
    NodeType type;
    Type *type_kind;
    struct {
        int number;
        double float_number;
        std::string ident;
        std::string str;
        struct {
            Node *lhs;
            Node *rhs;
            OpType op;
        } bin;
        struct {
            Node *expr;
            OpType op;
        } unary;
        struct {
            Node *cond;
            Node *then_expr;
            Node *else_expr;
        } if_expr;
        struct {
            std::string name;
            Node *body;
            Node *next_expr;
        } let_in;
        struct {
            std::string name;
            std::vector<std::string> args;
            std::vector<Type *> arg_types;
            Node *body;
        } let_fun;
        struct {
            std::string name;
            std::vector<Node *> args;
        } app_expr;
        struct {
            std::string name;
            std::vector<std::string> args;
            std::vector<Type *> arg_types;
            Type *ret_type;
        } let_extern;
        struct {
            std::vector<Node *> exprs;
        } compound;
        struct {
            Type *ty;
        } new_expr;
    };

    Node(NodeType type) : type{type}, type_kind{new Type()} {};
    void print_node() {
        if (type == ND_NUMBER) {
            std::cout << number;
        } else if (type == ND_STRING) {
            std::cout << "\"" << str << "\"";
        } else if (type == ND_VAR) {
            std::cout << ident;
        } else if (type == ND_BIN) {
            std::cout << "(";
            bin.lhs->print_node();
            print_op(bin.op);
            bin.rhs->print_node();
            std::cout << ")";
        } else if (type == ND_IF) {
            std::cout << "(if ";
            if_expr.cond->print_node();
            std::cout << " ";
            if_expr.then_expr->print_node();
            std::cout << " ";
            if_expr.else_expr->print_node();
            std::cout << ")";
        } else if (type == ND_LET_IN) {
            std::cout << "(let " << let_in.name;
            std::cout << " ";
            let_in.body->print_node();
            std::cout << " ";
            let_in.next_expr->print_node();
            std::cout << ")";
        } else if (type == ND_LET_FUN) {
            std::cout << "(let_fun " << let_fun.name << " ";
            for (std::string name : let_fun.args) {
                std::cout << name << " ";
            }

            let_fun.body->print_node();
            std::cout << ")";
        } else if (type == ND_LET_EXTERN) {
            std::cout << "(let_extern " << let_extern.name << " ";
            for (std::string name : let_extern.args) {
                std::cout << name << " ";
            }
            std::cout << ")";
        } else if (type == ND_APP) {
            std::cout << "(" << app_expr.name << " ";
            int size = app_expr.args.size();
            for (int i = 0; i < size; i++) {
                Node *node = app_expr.args[i];
                node->print_node();
                if (i != size - 1) {
                    std::cout << " ";
                }
            }
            std::cout << ")";
        } else if (type == ND_COMPOUND) {
            std::cout << "{";
            for (Node *expr : compound.exprs) {
                expr->print_node();
                std::cout << "; ";
            }
            std::cout << "}";
        } else if (type == ND_NEW) {
            std::cout << "(new ";
            new_expr.ty->print_type();
            std::cout << ")";
        } else if (type == ND_UNARY) {
            print_op(unary.op);
            unary.expr->print_node();
        } else {
            std::cout << "unknown";
        }
    }
};

class Parser {
  private:
    std::vector<Token> tokens;
    int cur;
    Token eof_tok;

    Token eat();
    Token peek(int offset);
    Token curr();
    bool match(TokenType type);
    bool peek_match(int offset, TokenType type);
    Token expect(TokenType type);

  public:
    Parser(std::vector<Token> tokens);
    Type *get_type_from_string(std::string ty);
    std::pair<std::string, Type *> type_specifier();
    std::pair<std::string, Type *> argument();
    Node *primary_expr();
    Node *unary_expr();
    Node *mul_expr();
    Node *add_expr();
    Node *rel_expr();
    Node *equal_expr();
    Node *assign_expr();
    Node *if_expr();
    Node *let_fun(std::string name);
    Node *let_in();
    Node *let_extern();
    Node *compound();
    Node *new_expr();
    Node *expr();
    Node *toplevel_expr();
    std::vector<Node *> parse_all();
};

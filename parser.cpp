#include "parser.hpp"
#include "lexer.hpp"
#include <cstdlib>
#include <string>
#include <utility>

Parser::Parser(std::vector<Token> tokens)
    : tokens{tokens}, cur{0}, eof_tok{Token(TK_EOF, "\0")} {}

Token Parser::eat() {
    if (tokens.size() <= cur)
        return eof_tok;
    return tokens[cur++];
}

Token Parser::peek(int offset) {
    if (tokens.size() <= cur + offset)
        return eof_tok;
    return tokens[cur + offset];
}

Token Parser::curr() {
    if (tokens.size() <= cur)
        return eof_tok;
    return tokens[cur];
}

bool Parser::match(TokenType type) {
    if (curr().get_type() != type) {
        return false;
    }

    return true;
}

bool Parser::peek_match(int offset, TokenType type) {
    if (peek(offset).get_type() != type) {
        return false;
    }

    return true;
}

Token Parser::expect(TokenType type) {
    if (curr().get_type() != type) {
        Token tk(type, "");
        error("expected %s but got %s", tk.get_type_str().c_str(),
              curr().get_type_str().c_str());
    }

    return eat();
}

Type *Parser::get_type_from_string(std::string ty) {
    TypeKind kind;
    if (ty == "int") {
        kind = TY_INT;
    } else if (ty == "string") {
        kind = TY_STRING;
    } else if (ty == "bool") {
        kind = TY_BOOL;
    } else if (ty == "float") {
        kind = TY_FLOAT;
    } else if (ty == "void") {
        kind = TY_VOID;
    } else {
        error("unknown type: %s", ty.c_str());
    }

    Type *type = new Type(kind);
    return type;
}

std::pair<std::string, Type *> Parser::type_specifier() {
    expect(TK_LPAREN);
    Token tk = expect(TK_IDENT);
    expect(TK_COLON);
    Token type_id = expect(TK_IDENT);
    Type *type = get_type_from_string(type_id.to_str());
    expect(TK_RPAREN);
    return std::make_pair(tk.to_str(), type);
}

std::pair<std::string, Type *> Parser::argument() {
    if (match(TK_LPAREN)) {
        return type_specifier();
    } else {
        Token tk = expect(TK_IDENT);
        return std::make_pair(tk.to_str(), new Type());
    }
}

Node *Parser::primary_expr() {
    if (match(TK_NUMBER)) {
        Node *num = new Node(ND_NUMBER);
        num->number = std::stoi(eat().to_str());
        return num;
    } else if (match(TK_FLOAT)) {
        Node *float_num = new Node(ND_FLOAT);
        float_num->float_number = std::stof(eat().to_str());
        return float_num;
    } else if (match(TK_STRING)) {
        Node *node = new Node(ND_STRING);
        node->str = eat().to_str();
        return node;
    } else if (match(TK_IDENT)) {
        if (peek_match(1, TK_LPAREN)) {
            Node *apply = new Node(ND_APP);
            apply->app_expr.name = eat().to_str();
            expect(TK_LPAREN);
            std::vector<Node *> args;
            while (!match(TK_RPAREN)) {
                args.push_back(expr());
                if (match(TK_RPAREN)) {
                    break;
                } else if (match(TK_COMMA)) {
                    eat();
                    continue;
                } else {
                    error("expected LPAREN or COMMA but got %s",
                          curr().get_type_str().c_str());
                }
            }
            expect(TK_RPAREN);
            apply->app_expr.args = args;
            return apply;
        }
        Node *var = new Node(ND_VAR);
        var->ident = eat().to_str();
        return var;
    } else if (match(TK_LPAREN)) {
        eat();
        Node *exp = expr();
        expect(TK_RPAREN);
        return exp;
    } else if (match(TK_TRUE)) {
        eat();
        Node *b = new Node(ND_BOOL);
        b->bool_val = true;
        return b;
    } else if (match(TK_FALSE)) {
        eat();
        Node *b = new Node(ND_BOOL);
        b->bool_val = false;
        return b;
    } else {
        error("unknown token: %s", curr().to_str().c_str());
        return NULL;
    }
}

Node *Parser::unary_expr() {
    if (match(TK_SHARP)) {
        eat();
        Node *expr = primary_expr();
        Node *u = new Node(ND_UNARY);
        u->unary.expr = expr;
        u->unary.op = OP_DEREF;
        return u;
    } else if (match(TK_NOT)) {
        eat();
        Node *expr = primary_expr();
        Node *u = new Node(ND_UNARY);
        u->unary.expr = expr;
        u->unary.op = OP_NOT;
        return u;
    } else {
        return primary_expr();
    }
}

Node *Parser::mul_expr() {
    Node *lhs = unary_expr();
    for (;;) {
        if (match(TK_ASTERISK) || match(TK_SLASH) || match(TK_PERCENT) ||
            match(TK_ASTERISK_DOT) || match(TK_SLASH_DOT) ||
            match(TK_PERCENT_DOT)) {
            Node *expr = new Node(ND_BIN);
            if (match(TK_ASTERISK))
                expr->bin.op = OP_MUL;
            else if (match(TK_SLASH))
                expr->bin.op = OP_DIV;
            else if (match(TK_PERCENT))
                expr->bin.op = OP_MOD;
            else if (match(TK_ASTERISK_DOT))
                expr->bin.op = OP_MULF;
            else if (match(TK_SLASH_DOT))
                expr->bin.op = OP_DIVF;
            else if (match(TK_PERCENT_DOT))
                expr->bin.op = OP_MODF;

            eat();
            Node *rhs = unary_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::add_expr() {
    Node *lhs = mul_expr();
    for (;;) {
        if (match(TK_PLUS) || match(TK_MINUS) || match(TK_PLUS_DOT) ||
            match(TK_MINUS_DOT)) {
            Node *expr = new Node(ND_BIN);
            if (match(TK_PLUS))
                expr->bin.op = OP_ADD;
            else if (match(TK_MINUS))
                expr->bin.op = OP_SUB;
            else if (match(TK_PLUS_DOT))
                expr->bin.op = OP_ADDF;
            else if (match(TK_MINUS_DOT))
                expr->bin.op = OP_SUBF;

            eat();
            Node *rhs = mul_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::rel_expr() {
    Node *lhs = add_expr();
    for (;;) {
        if (match(TK_GREATER) || match(TK_LESS) || match(TK_GREATER_EQ) ||
            match(TK_LESS_EQ)) {
            Node *expr = new Node(ND_BIN);
            if (match(TK_GREATER)) {
                expr->bin.op = OP_GREATER;
            } else if (match(TK_LESS)) {
                expr->bin.op = OP_LESS;
            } else if (match(TK_GREATER_EQ)) {
                expr->bin.op = OP_GREATER_EQ;
            } else if (match(TK_LESS_EQ)) {
                expr->bin.op = OP_LESS_EQ;
            }

            eat();
            Node *rhs = add_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::equal_expr() {
    Node *lhs = rel_expr();
    for (;;) {
        if (match(TK_EQ) || match(TK_NOT_EQ)) {
            Node *expr = new Node(ND_BIN);
            if (match(TK_EQ))
                expr->bin.op = OP_EQ;
            else
                expr->bin.op = OP_NOT_EQ;

            eat();
            Node *rhs = rel_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::bitwise_and_expr() {
    Node *lhs = equal_expr();
    for (;;) {
        if (match(TK_BITAND)) {
            Node *expr = new Node(ND_BIN);
            expr->bin.op = OP_BITAND;

            eat();
            Node *rhs = equal_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::bitwise_xor_expr() {
    Node *lhs = bitwise_and_expr();
    for (;;) {
        if (match(TK_BITXOR)) {
            Node *expr = new Node(ND_BIN);
            expr->bin.op = OP_BITXOR;

            eat();
            Node *rhs = bitwise_and_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::bitwise_or_expr() {
    Node *lhs = bitwise_xor_expr();
    for (;;) {
        if (match(TK_BITOR)) {
            Node *expr = new Node(ND_BIN);
            expr->bin.op = OP_BITOR;

            eat();
            Node *rhs = bitwise_xor_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::logical_and_expr() {
    Node *lhs = bitwise_or_expr();
    for (;;) {
        if (match(TK_LOGAND)) {
            Node *expr = new Node(ND_BIN);
            expr->bin.op = OP_LOGAND;

            eat();
            Node *rhs = bitwise_or_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::logical_or_expr() {
    Node *lhs = logical_and_expr();
    for (;;) {
        if (match(TK_LOGOR)) {
            Node *expr = new Node(ND_BIN);
            expr->bin.op = OP_LOGOR;

            eat();
            Node *rhs = logical_and_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::assign_expr() {
    Node *lhs = logical_or_expr();
    for (;;) {
        if (match(TK_PTR_ASSIGN)) {
            Node *expr = new Node(ND_BIN);
            expr->bin.op = OP_PTR_ASSIGN;

            eat();
            Node *rhs = logical_or_expr();
            expr->bin.lhs = lhs;
            expr->bin.rhs = rhs;
            lhs = expr;
        } else {
            break;
        }
    }

    return lhs;
}

Node *Parser::if_expr() {
    expect(TK_IF);
    Node *cond = expr();
    expect(TK_THEN);
    Node *then_expr = toplevel_expr();
    expect(TK_ELSE);
    Node *else_expr = toplevel_expr();
    Node *if_node = new Node(ND_IF);
    if_node->if_expr.cond = cond;
    if_node->if_expr.then_expr = then_expr;
    if_node->if_expr.else_expr = else_expr;
    return if_node;
}

Node *Parser::let_fun(std::string name) {
    Node *let = new Node(ND_LET_FUN);
    std::vector<std::string> args;
    std::vector<Type *> types;
    while (!match(TK_ASSIGN)) {
        auto name_and_type = argument();
        args.push_back(name_and_type.first);
        types.push_back(name_and_type.second);
    }
    expect(TK_ASSIGN);

    Node *body = expr();
    let->let_fun.name = name;
    let->let_fun.args = args;
    let->let_fun.arg_types = types;
    let->let_fun.body = body;
    return let;
}

Node *Parser::let_extern() {
    expect(TK_EXTERN);
    Token id = expect(TK_IDENT);
    std::vector<std::string> args;
    std::vector<Type *> types;
    while (match(TK_LPAREN)) {
        auto name_and_type = type_specifier();
        args.push_back(name_and_type.first);
        types.push_back(name_and_type.second);
    }
    expect(TK_COLON);
    Token tk = expect(TK_IDENT);
    Type *ret_type = get_type_from_string(tk.to_str());

    Node *node = new Node(ND_LET_EXTERN);
    node->let_extern.name = id.to_str();
    node->let_extern.args = args;
    node->let_extern.arg_types = types;
    node->let_extern.ret_type = ret_type;
    return node;
}

Node *Parser::let_in() {
    expect(TK_LET);
    if (match(TK_EXTERN)) {
        return let_extern();
    }
    Token var = expect(TK_IDENT);
    if (!match(TK_ASSIGN)) {
        return let_fun(var.to_str());
    }
    expect(TK_ASSIGN);
    Node *exp = expr();
    expect(TK_IN);
    Node *next_exp = expr();

    Node *node = new Node(ND_LET_IN);
    node->let_in.name = var.to_str();
    node->let_in.body = exp;
    node->let_in.next_expr = next_exp;
    return node;
}

Node *Parser::compound() {
    expect(TK_LBRACE);
    std::vector<Node *> exprs;
    while (!match(TK_RBRACE)) {
        exprs.push_back(expr());
    }
    expect(TK_RBRACE);

    Node *node = new Node(ND_COMPOUND);
    node->compound.exprs = exprs;
    return node;
}

Node *Parser::new_expr() {
    expect(TK_NEW);
    Type *ty = get_type_from_string(expect(TK_IDENT).to_str());
    Type *ptr = new Type(TY_PTR);
    ptr->ptr_to = ty;
    Node *node = new Node(ND_NEW);
    node->new_expr.ty = ptr;
    return node;
}

Node *Parser::expr() {
    if (match(TK_IF)) {
        return if_expr();
    } else if (match(TK_LET)) {
        return let_in();
    } else if (match(TK_LBRACE)) {
        return compound();
    } else if (match(TK_NEW)) {
        return new_expr();
    }

    return assign_expr();
}

Node *Parser::toplevel_expr() {
    Node *lhs = expr();
    for (;;) {
        if (match(TK_SEMICOLON)) {
            eat();
            Node *node = new Node(ND_BIN);
            node->bin.op = OP_SEMICOLON;
            Node *rhs = expr();
            node->bin.lhs = lhs;
            node->bin.rhs = rhs;
            lhs = node;
        } else {
            break;
        }
    }

    return lhs;
}

std::vector<Node *> Parser::parse_all() {
    std::vector<Node *> nodes;
    while (!match(TK_EOF)) {
        nodes.push_back(toplevel_expr());
    }

    return nodes;
}

void print_op(OpType type) {
    if (type == OP_ADD) {
        std::cout << "+";
    } else if (type == OP_SUB) {
        std::cout << "-";
    } else if (type == OP_MUL) {
        std::cout << "*";
    } else if (type == OP_DIV) {
        std::cout << "/";
    } else if (type == OP_GREATER) {
        std::cout << ">";
    } else if (type == OP_LESS) {
        std::cout << "<";
    } else if (type == OP_GREATER_EQ) {
        std::cout << ">=";
    } else if (type == OP_LESS_EQ) {
        std::cout << "<=";
    } else if (type == OP_EQ) {
        std::cout << "==";
    } else if (type == OP_NOT_EQ) {
        std::cout << "!=";
    } else if (type == OP_MOD) {
        std::cout << "%";
    } else if (type == OP_PTR_ASSIGN) {
        std::cout << ":=";
    } else if (type == OP_DEREF) {
        std::cout << "*";
    } else if (type == OP_PTR_ASSIGN) {
        std::cout << ":=";
    } else if (type == OP_LOGAND) {
        std::cout << "&&";
    } else if (type == OP_LOGOR) {
        std::cout << "||";
    } else if (type == OP_BITAND) {
        std::cout << "&";
    } else if (type == OP_BITOR) {
        std::cout << "|";
    } else if (type == OP_BITXOR) {
        std::cout << "^";
    } else {
        std::cout << "unknown op";
    }
}

#pragma once

#include <cctype>
#include <string>
#include <vector>

#include "error.hpp"

typedef enum TokenType {
    TK_EOF,
    TK_NUMBER,
    TK_FLOAT,
    TK_STRING,
    TK_IDENT,
    TK_IF,
    TK_THEN,
    TK_ELSE,
    TK_LET,
    TK_IN,
    TK_EXTERN,
    TK_NEW,
    TK_PLUS,
    TK_MINUS,
    TK_ASTERISK,
    TK_SLASH,
    TK_PERCENT,
    TK_PLUS_DOT,
    TK_MINUS_DOT,
    TK_ASTERISK_DOT,
    TK_SLASH_DOT,
    TK_PERCENT_DOT,
    TK_ASSIGN,
    TK_COMMA,
    TK_COLON,
    TK_SEMICOLON,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_GREATER,
    TK_LESS,
    TK_GREATER_EQ,
    TK_LESS_EQ,
    TK_EQ,
    TK_NOT_EQ,
    TK_NOT,
    TK_PTR_ASSIGN,
    TK_SHARP,
    TK_LOGAND,
    TK_BITAND,
    TK_LOGOR,
    TK_BITOR,
    TK_BITXOR,
    TK_TRUE,
    TK_FALSE,
} TokenType;

class Token {
  protected:
    TokenType tok_type;
    std::string str;

  public:
    Token(TokenType tok_type, std::string str);
    TokenType get_type();
    std::string get_type_str();
    std::string to_str();
};

class Lexer {
  protected:
    std::string source;
    int cur;

    char read();
    char curr();
    char peek(int offset);
    static bool isident(int c);
    Token next();

  public:
    Lexer(std::string source);
    std::vector<Token> tokenize();
};

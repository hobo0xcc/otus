#include "lexer.hpp"
#include <cctype>

Token::Token(TokenType tok_type, std::string str)
    : str(str), tok_type{tok_type} {}

TokenType Token::get_type() { return tok_type; }

std::string Token::get_type_str() {
    switch (tok_type) {
    case TK_EOF:
        return "EOF";
    case TK_NUMBER:
        return "NUMBER";
    case TK_STRING:
        return "STRING";
    case TK_IDENT:
        return "IDENT";
    case TK_IF:
        return "IF";
    case TK_THEN:
        return "THEN";
    case TK_ELSE:
        return "ELSE";
    case TK_LET:
        return "LET";
    case TK_IN:
        return "IN";
    case TK_EXTERN:
        return "extern";
    case TK_PLUS:
        return "PLUS";
    case TK_MINUS:
        return "MINUS";
    case TK_ASTERISK:
        return "ASTERISK";
    case TK_SLASH:
        return "SLASH";
    case TK_PERCENT:
        return "PERCENT";
    case TK_ASSIGN:
        return "ASSIGN";
    case TK_COMMA:
        return "COMMA";
    case TK_COLON:
        return "COLON";
    case TK_SEMICOLON:
        return "SEMICOLON";
    case TK_LPAREN:
        return "LPAREN";
    case TK_RPAREN:
        return "RPAREN";
    case TK_LBRACE:
        return "LBRACE";
    case TK_RBRACE:
        return "RBRACE";
    case TK_GREATER:
        return "GREATER";
    case TK_LESS:
        return "LESS";
    case TK_GREATER_EQ:
        return "GREATER_EQ";
    case TK_LESS_EQ:
        return "LESS_EQ";
    default:
        return "UNKNOWN";
    }
}

std::string Token::to_str() { return str; }

Lexer::Lexer(std::string source) : source{source}, cur{0} {}

char Lexer::read() {
    if (source.size() <= cur) {
        return '\0';
    }

    return source[cur++];
}

char Lexer::curr() {
    if (source.size() <= cur) {
        return '\0';
    }

    return source[cur];
}

char Lexer::peek(int offset) {
    if (source.size() <= cur + offset) {
        return '\0';
    }

    return source[cur + offset];
}

bool Lexer::isident(int c) {
    if (std::isalnum(c) || c == '_')
        return true;
    return false;
}

Token Lexer::next() {
    auto lexing = [&](auto f) {
        int begin = cur;
        while (f(curr()))
            read();
        int end = cur;
        std::string str = source.substr(begin, end - begin);
        return str;
    };
    while (curr()) {
        if (std::isspace(curr())) {
            lexing(std::isspace);
            continue;
        }

        if (std::isdigit(curr())) {
            TokenType type = TK_NUMBER;
            int begin = cur;
            while (std::isdigit(curr()) || curr() == '.') {
                if (curr() == '.') {
                    if (type == TK_FLOAT) {
                        error("multiple dots in float number.");
                    }
                    type = TK_FLOAT;
                }
                read();
            }
            int end = cur;
            std::string result = source.substr(begin, end - begin);
            return Token(type, result);
        } else if (std::isalpha(curr()) || curr() == '_') {
            std::string result = lexing(isident);
            TokenType type = TK_IDENT;
            if (result == "if")
                type = TK_IF;
            else if (result == "then")
                type = TK_THEN;
            else if (result == "else")
                type = TK_ELSE;
            else if (result == "let")
                type = TK_LET;
            else if (result == "in")
                type = TK_IN;
            else if (result == "extern")
                type = TK_EXTERN;
            else if (result == "new")
                type = TK_NEW;
            return Token(type, result);
        } else if (curr() == '"') {
            read();
            int begin = cur;
            while (curr() && curr() != '"')
                read();
            if (curr() != '"') {
                error("string quote not closed");
            }
            int end = cur;
            read();
            std::string str = source.substr(begin, end - begin);
            return Token(TK_STRING, str);
        }

        // Symbols
        if (curr() == '+') {
            read();
            if (curr() == '.') {
                read();
                return Token(TK_PLUS_DOT, "+.");
            }
            return Token(TK_PLUS, "+");
        } else if (curr() == '-') {
            read();
            if (curr() == '.') {
                read();
                return Token(TK_MINUS_DOT, "-.");
            }
            return Token(TK_MINUS, "-");
        } else if (curr() == '*') {
            read();
            if (curr() == '.') {
                read();
                return Token(TK_ASTERISK_DOT, "*.");
            }
            return Token(TK_ASTERISK, "*");
        } else if (curr() == '/') {
            read();
            if (curr() == '.') {
                read();
                return Token(TK_SLASH_DOT, "/.");
            }
            return Token(TK_SLASH, "/");
        } else if (curr() == '%') {
            read();
            if (curr() == '.') {
                read();
                return Token(TK_PERCENT_DOT, "%.");
            }
            return Token(TK_PERCENT, "%");
        } else if (curr() == ',') {
            read();
            return Token(TK_COMMA, ",");
        } else if (curr() == '=') {
            read();
            if (curr() == '=') {
                read();
                return Token(TK_EQ, "==");
            } else {
                return Token(TK_ASSIGN, "=");
            }
        } else if (curr() == ':') {
            read();
            if (curr() == '=') {
                read();
                return Token(TK_PTR_ASSIGN, ":=");
            }
            return Token(TK_COLON, ":");
        } else if (curr() == ';') {
            read();
            return Token(TK_SEMICOLON, ";");
        } else if (curr() == '(') {
            read();
            return Token(TK_LPAREN, "(");
        } else if (curr() == ')') {
            read();
            return Token(TK_RPAREN, ")");
        } else if (curr() == '{') {
            read();
            return Token(TK_LBRACE, "{");
        } else if (curr() == '}') {
            read();
            return Token(TK_RBRACE, "}");
        } else if (curr() == '!') {
            read();
            if (curr() == '=') {
                read();
                return Token(TK_NOT_EQ, "!=");
            } else {
                return Token(TK_NOT, "!");
            }
        } else if (curr() == '>') {
            read();
            if (curr() == '=') {
                read();
                return Token(TK_GREATER_EQ, ">=");
            } else {
                return Token(TK_GREATER, ">");
            }
        } else if (curr() == '<') {
            read();
            if (curr() == '=') {
                read();
                return Token(TK_LESS_EQ, "<=");
            } else {
                return Token(TK_LESS, "<");
            }
        } else if (curr() == '#') {
            read();
            return Token(TK_SHARP, "#");
        } else {
            error("unknown character: %c", curr());
        }
    }

    return Token(TK_EOF, "\0");
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token tk(TK_EOF, "\0");
    while ((tk = next()).get_type() != TK_EOF) {
        tokens.push_back(tk);
    }
    tokens.push_back(tk);

    return tokens;
}

#include "parser.hpp"

#include <map>
#include <string>

using Subst = std::map<std::string, Type *>;

class Equation {
  private:
    Type *lhs;
    Type *rhs;
    Node *node;

  public:
    Equation(Type *lhs, Type *rhs, Node *node);
    Type *get_lhs();
    Type *get_rhs();
    Node *get_node();
};

typedef struct TypingEnv TypingEnv;

struct TypingEnv {
    std::map<std::string, Type *> named_map;
    TypingEnv *parent;

    TypingEnv(TypingEnv *parent) : parent{parent} {};
    Type *get(std::string key) {
        if (named_map.find(key) == named_map.end()) {
            if (parent != NULL) {
                return parent->get(key);
            } else {
                return NULL;
            }
        } else {
            return named_map[key];
        }
    }
    void set(std::string key, Type *item) { named_map[key] = item; }
};

typedef enum UnifyResult {
    UNIFY_FAIL,
} UnifyResult;

class Typing {
  private:
    std::vector<Node *> nodes;
    int typevar_i;
    std::vector<Equation> equations;
    Type *int_type = new Type(TY_INT);
    Type *bool_type = new Type(TY_BOOL);
    Type *string_type = new Type(TY_STRING);
    Type *float_type = new Type(TY_FLOAT);

  public:
    Typing(std::vector<Node *> nodes);
    std::string new_typevar();
    void annotate(Node *node, TypingEnv *e);
    void equate(Node *node, TypingEnv *e);
    void unify(Type *x, Type *y, Subst &subst);
    void unify_variable(Type *v, Type *x, Subst &subst);
    bool occurs_check(Type *v, Type *t, Subst &subst);
    Type *get_real_type(Type *v, Subst subst);
    void set_type(Subst subst, Node *node);
    std::vector<Node *> infer();
    void print_equations();
    void print_node_with_type(Node *node);
};

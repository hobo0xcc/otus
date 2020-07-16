#include "typing.hpp"
#include "parser.hpp"
#include <iostream>
#include <ostream>

Equation::Equation(Type *lhs, Type *rhs, Node *node)
    : lhs{lhs}, rhs{rhs}, node{node} {}

Type *Equation::get_lhs() { return lhs; }

Type *Equation::get_rhs() { return rhs; }

Node *Equation::get_node() { return node; }

Typing::Typing(std::vector<Node *> nodes) : nodes{nodes}, typevar_i{0} {}

std::string Typing::new_typevar() {
    std::string buf;
    buf.push_back('t');
    buf.append(std::to_string(typevar_i++));
    return buf;
}

void Typing::annotate(Node *node, TypingEnv *e) {
    if (node->type == ND_NUMBER) {
        node->type_kind->kind = TY_INT;
    } else if (node->type == ND_STRING) {
        node->type_kind->kind = TY_STRING;
    } else if (node->type == ND_VAR) {
        Type *ty;
        if ((ty = e->get(node->ident)) == NULL) {
            error("variable or function not found: %s", node->ident.c_str());
        }

        node->type_kind = ty;
    } else if (node->type == ND_FLOAT) {
        node->type_kind->kind = TY_FLOAT;
    } else if (node->type == ND_BIN) {
        annotate(node->bin.lhs, e);
        annotate(node->bin.rhs, e);
        node->type_kind->kind = TY_VAR;
        node->type_kind->typevar = new_typevar();
    } else if (node->type == ND_UNARY) {
        annotate(node->unary.expr, e);
        node->type_kind->kind = TY_VAR;
        node->type_kind->typevar = new_typevar();
    } else if (node->type == ND_IF) {
        annotate(node->if_expr.cond, e);
        annotate(node->if_expr.then_expr, e);
        annotate(node->if_expr.else_expr, e);
        node->type_kind->kind = TY_VAR;
        node->type_kind->typevar = new_typevar();
    } else if (node->type == ND_LET_IN) {
        annotate(node->let_in.body, e);
        e->set(node->let_in.name, node->let_in.body->type_kind);
        annotate(node->let_in.next_expr, e);
        node->type_kind = node->let_in.next_expr->type_kind;
    } else if (node->type == ND_LET_FUN) {
        TypingEnv *new_env = new TypingEnv(NULL);
        int arg_len = node->let_fun.args.size();
        std::vector<Type *> arg_types;
        for (int i = 0; i < arg_len; i++) {
            Type *ty = new Type(TY_VAR);
            ty->typevar = new_typevar();
            std::string name = node->let_fun.args[i];
            arg_types.push_back(ty);
            new_env->set(name, ty);
        }

        Type *tvar = new Type(TY_VAR);
        tvar->typevar = new_typevar();
        node->type_kind = tvar;
        node->let_fun.arg_types = arg_types;
        annotate(node->let_fun.body, new_env);
    } else if (node->type == ND_LET_EXTERN) {
        int arg_len = node->let_extern.args.size();
        std::vector<Type *> arg_types;
        for (int i = 0; i < arg_len; i++) {
            Type *ty = new Type(TY_VAR);
            ty->typevar = new_typevar();
            arg_types.push_back(ty);
        }

        Type *tvar = new Type(TY_VAR);
        tvar->typevar = new_typevar();
        node->type_kind = tvar;
        node->let_fun.arg_types = arg_types;
    } else if (node->type == ND_APP) {
        for (Node *node : node->app_expr.args) {
            annotate(node, e);
        }

        Type *tvar = new Type(TY_VAR);
        tvar->typevar = new_typevar();
        node->type_kind = tvar;
    } else if (node->type == ND_COMPOUND) {
        int size = node->compound.exprs.size();
        TypingEnv *new_env = new TypingEnv(e);
        for (int i = 0; i < size; i++) {
            Node *expr = node->compound.exprs[i];
            annotate(expr, new_env);
        }

        Node *last = node->compound.exprs[size - 1];
        node->type_kind = last->type_kind;
    } else if (node->type == ND_NEW) {
        node->type_kind = node->new_expr.ty;
    } else {
        error("unknown node type");
    }
}

void Typing::equate(Node *node, TypingEnv *e) {
    if (node->type == ND_NUMBER) {
        Equation eq(node->type_kind, int_type, node);
        equations.push_back(eq);
    } else if (node->type == ND_STRING) {
        Equation eq(node->type_kind, string_type, node);
        equations.push_back(eq);
    } else if (node->type == ND_BIN) {
        equate(node->bin.lhs, e);
        equate(node->bin.rhs, e);
        Type *ty;
        Type *lhs_ty;
        Type *rhs_ty;
        switch (node->bin.op) {
        case OP_EQ:
        case OP_NOT_EQ:
        case OP_GREATER:
        case OP_GREATER_EQ:
        case OP_LESS:
        case OP_LESS_EQ: {
            ty = bool_type;
            lhs_ty = node->bin.lhs->type_kind;
            rhs_ty = node->bin.rhs->type_kind;
            break;
        }
        case OP_PTR_ASSIGN: {
            Type *ptr_ty = new Type(TY_PTR);
            ptr_ty->ptr_to = new Type(TY_VAR);
            ptr_ty->ptr_to->typevar = new_typevar();
            Equation ptr_eq(node->bin.lhs->type_kind, ptr_ty, node->bin.lhs);
            Equation deref_eq(node->bin.rhs->type_kind, ptr_ty->ptr_to,
                              node->bin.rhs);
            Equation node_eq(node->type_kind, node->bin.rhs->type_kind, node);
            equations.push_back(ptr_eq);
            equations.push_back(node_eq);
            equations.push_back(deref_eq);
            return;
        }
        case OP_SEMICOLON:
            return;
        case OP_ADDF:
        case OP_SUBF:
        case OP_MULF:
        case OP_DIVF:
        case OP_MODF:
            ty = float_type;
            lhs_ty = float_type;
            rhs_ty = float_type;
            break;
        default:
            ty = int_type;
            lhs_ty = int_type;
            rhs_ty = int_type;
            break;
        }
        Equation eq(node->type_kind, ty, node);
        Equation lhs_eq(node->bin.lhs->type_kind, lhs_ty, node->bin.lhs);
        Equation rhs_eq(node->bin.rhs->type_kind, rhs_ty, node->bin.rhs);
        Equation same_eq(lhs_ty, rhs_ty, node);

        equations.push_back(eq);
        equations.push_back(lhs_eq);
        equations.push_back(rhs_eq);
        equations.push_back(same_eq);
    } else if (node->type == ND_UNARY) {
        Type *ty;
        if (node->unary.op == OP_DEREF) {
            Type *ptr_ty = new Type(TY_PTR);
            ptr_ty->ptr_to = new Type(TY_VAR);
            ptr_ty->ptr_to->typevar = new_typevar();
            Equation eq(ptr_ty, node->unary.expr->type_kind, node->unary.expr);
            Equation node_eq(node->type_kind, ptr_ty->ptr_to, node);
            equations.push_back(eq);
            ty = ptr_ty->ptr_to;
        } else if (node->unary.op == OP_NOT) {
            Equation eq(node->unary.expr->type_kind, bool_type,
                        node->unary.expr);
            Equation node_eq(node->type_kind, bool_type, node);
            equations.push_back(eq);
            equations.push_back(node_eq);
        } else {
            error("unknown unary operator");
        }

        Equation eq(ty, node->type_kind, node);
        equations.push_back(eq);
    } else if (node->type == ND_IF) {
        equate(node->if_expr.cond, e);
        equate(node->if_expr.then_expr, e);
        equate(node->if_expr.else_expr, e);
        Equation cond(node->if_expr.cond->type_kind, bool_type,
                      node->if_expr.cond);
        Equation then_and_else(node->if_expr.then_expr->type_kind,
                               node->if_expr.else_expr->type_kind, node);
        Equation if_eq(node->type_kind, node->if_expr.then_expr->type_kind,
                       node);

        equations.push_back(cond);
        equations.push_back(then_and_else);
        equations.push_back(if_eq);
    } else if (node->type == ND_APP) {
        std::vector<Type *> arg_types;
        int size = node->app_expr.args.size();
        for (int i = 0; i < size; i++) {
            Node *arg = node->app_expr.args[i];
            equate(arg, e);
            arg_types.push_back(arg->type_kind);
        }

        if (e->get(node->app_expr.name) == NULL) {
            error("function not found: %s", node->app_expr.name.c_str());
        }
        Type *fun_type = e->get(node->app_expr.name);
        Type *app_type = new Type(TY_FUN);

        app_type->arg_types = arg_types;
        app_type->ret_type = node->type_kind;

        Equation eq(fun_type, app_type, node);
        equations.push_back(eq);
    } else if (node->type == ND_LET_IN) {
        equate(node->let_in.body, e);
        equate(node->let_in.next_expr, e);
    } else if (node->type == ND_LET_FUN) {
        Type *fun_type = new Type(TY_FUN);
        fun_type->ret_type = node->let_fun.body->type_kind;
        fun_type->arg_types = node->let_fun.arg_types;

        Equation eq(node->type_kind, fun_type, node);
        equations.push_back(eq);

        e->set(node->let_fun.name, fun_type);
        TypingEnv *new_env = new TypingEnv(e);
        int arg_len = node->let_fun.args.size();
        for (int i = 0; i < arg_len; i++) {
            new_env->set(node->let_fun.args[i], node->let_fun.arg_types[i]);
        }
        equate(node->let_fun.body, new_env);
    } else if (node->type == ND_LET_EXTERN) {
        Type *fun_type = new Type(TY_FUN);
        fun_type->ret_type = node->let_extern.ret_type;
        fun_type->arg_types = node->let_extern.arg_types;
        e->set(node->let_extern.name, fun_type);
        Equation eq(node->type_kind, fun_type, node);
        equations.push_back(eq);
    } else if (node->type == ND_COMPOUND) {
        TypingEnv *new_env = new TypingEnv(e);
        for (Node *expr : node->compound.exprs) {
            equate(expr, new_env);
        }
    }
}

void Typing::unify(Type *x, Type *y, Subst &subst) {
    if (x->kind != TY_VAR && x->kind != TY_FUN && x->kind == y->kind) {
        return;
    } else if (x->kind == TY_VAR && y->kind == TY_VAR &&
               x->typevar == y->typevar) {
        return;
    } else if (x->kind == TY_VAR) {
        unify_variable(x, y, subst);
        return;
    } else if (y->kind == TY_VAR) {
        unify_variable(y, x, subst);
        return;
    } else if (x->kind == TY_FUN && y->kind == TY_FUN) {
        if (x->arg_types.size() != y->arg_types.size()) {
            throw UNIFY_FAIL;
        }

        unify(x->ret_type, y->ret_type, subst);
        int arg_len = x->arg_types.size();
        for (int i = 0; i < arg_len; i++) {
            Type *lhs = x->arg_types[i];
            Type *rhs = y->arg_types[i];
            unify(lhs, rhs, subst);
        }

        return;
    } else if (x->kind == TY_PTR && y->kind == TY_PTR) {
        unify(x->ptr_to, y->ptr_to, subst);
    }

    // std::cout << "debug1" << std::endl;
    // x->print_type();
    // std::cout << std::endl;
    // y->print_type();
    // std::cout << std::endl;

    throw UNIFY_FAIL;
}

void Typing::unify_variable(Type *v, Type *x, Subst &subst) {
    if (subst.find(v->typevar) != subst.end()) {
        unify(subst[v->typevar], x, subst);
    } else if (x->kind == TY_VAR && subst.find(x->typevar) != subst.end()) {
        unify(v, subst[x->typevar], subst);
    } else if (occurs_check(v, x, subst)) {
        throw UNIFY_FAIL;
    } else {
        subst[v->typevar] = x;
    }
}

bool Typing::occurs_check(Type *v, Type *t, Subst &subst) {
    if (t->kind == TY_VAR && v->typevar == t->typevar) {
        return true;
    } else if (t->kind == TY_VAR && subst.find(t->typevar) != subst.end()) {
        return occurs_check(v, subst[t->typevar], subst);
    } else if (t->kind == TY_FUN) {
        for (Type *ty : t->arg_types) {
            if (occurs_check(v, ty, subst)) {
                return true;
            }
        }
    } else if (t->kind == TY_PTR) {
        if (occurs_check(v, t->ptr_to, subst)) {
            return true;
        }
    }

    return false;
}

Type *Typing::get_real_type(Type *v, Subst subst) {
    if (v->kind == TY_FUN) {
        int arg_len = v->arg_types.size();
        for (int i = 0; i < arg_len; i++) {
            v->arg_types[i] = get_real_type(v->arg_types[i], subst);
        }
        v->ret_type = get_real_type(v->ret_type, subst);
        return v;
    } else if (v->kind != TY_VAR) {
        if (v->kind == TY_PTR) {
            v->ptr_to = get_real_type(v->ptr_to, subst);
        }
        return v;
    }

    return get_real_type(subst[v->typevar], subst);
}

void Typing::set_type(Subst subst, Node *node) {
    node->type_kind = get_real_type(node->type_kind, subst);
    // if (node->type_kind->kind == TY_VAR) {
    //     node->type_kind = get_real_type(node->type_kind, subst);
    // }
    // if (node->type_kind->kind == TY_FUN) {
    //     int size = node->type_kind->arg_types.size();
    //     for (int i = 0; i < size; i++) {
    //         Type *ty = node->type_kind->arg_types[i];
    //         node->type_kind->arg_types[i] = get_real_type(ty, subst);
    //     }

    //     Type *ret_type = node->type_kind->ret_type;
    //     node->type_kind->ret_type = get_real_type(ret_type, subst);
    // }

    if (node->type == ND_BIN) {
        set_type(subst, node->bin.lhs);
        set_type(subst, node->bin.rhs);
    } else if (node->type == ND_UNARY) {
        set_type(subst, node->unary.expr);
    } else if (node->type == ND_IF) {
        set_type(subst, node->if_expr.cond);
        set_type(subst, node->if_expr.then_expr);
        set_type(subst, node->if_expr.else_expr);
    } else if (node->type == ND_LET_IN) {
        set_type(subst, node->let_in.body);
        set_type(subst, node->let_in.next_expr);
    } else if (node->type == ND_LET_FUN) {
        set_type(subst, node->let_fun.body);
    } else if (node->type == ND_APP) {
        int size = node->app_expr.args.size();
        for (int i = 0; i < size; i++) {
            set_type(subst, node->app_expr.args[i]);
        }
    } else if (node->type == ND_COMPOUND) {
        int size = node->compound.exprs.size();
        for (int i = 0; i < size; i++) {
            set_type(subst, node->compound.exprs[i]);
        }
    }
}

std::vector<Node *> Typing::infer() {
    TypingEnv *annotate_env = new TypingEnv(NULL);
    for (Node *node : nodes) {
        annotate(node, annotate_env);
    }

    TypingEnv *equate_env = new TypingEnv(NULL);
    for (Node *node : nodes) {
        equate(node, equate_env);
    }
    // print_equations();

    Subst subst;
    for (Equation eq : equations) {
        try {
            unify(eq.get_lhs(), eq.get_rhs(), subst);
        } catch (UnifyResult res) {
            error("unification failed");
        }
    }

    // std::string sep("--------------------");
    // std::cout << sep << std::endl;

    for (Node *node : nodes) {
        set_type(subst, node);
        // print_node_with_type(node);
    }

    return nodes;
}

void Typing::print_equations() {
    for (Equation eq : equations) {
        eq.get_lhs()->print_type();
        std::cout << " == ";
        eq.get_rhs()->print_type();
        std::cout << " :: ";
        eq.get_node()->print_node();
        std::flush(std::cout);
        std::cout << std::endl;
    }
}

void Typing::print_node_with_type(Node *node) {
    node->print_node();
    std::cout << " :: ";
    std::flush(std::cout);
    node->type_kind->print_type();
    std::cout << std::endl;

    if (node->type == ND_BIN) {
        print_node_with_type(node->bin.lhs);
        print_node_with_type(node->bin.rhs);
    } else if (node->type == ND_UNARY) {
        print_node_with_type(node->unary.expr);
    } else if (node->type == ND_IF) {
        print_node_with_type(node->if_expr.cond);
        print_node_with_type(node->if_expr.then_expr);
        print_node_with_type(node->if_expr.else_expr);
    } else if (node->type == ND_LET_IN) {
        print_node_with_type(node->let_in.body);
        print_node_with_type(node->let_in.next_expr);
    } else if (node->type == ND_LET_FUN) {
        print_node_with_type(node->let_fun.body);
    } else if (node->type == ND_APP) {
        int size = node->app_expr.args.size();
        for (int i = 0; i < size; i++) {
            print_node_with_type(node->app_expr.args[i]);
        }
    } else if (node->type == ND_COMPOUND) {
        int size = node->compound.exprs.size();
        for (int i = 0; i < size; i++) {
            print_node_with_type(node->compound.exprs[i]);
        }
    }
}

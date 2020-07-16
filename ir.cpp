#include "ir.hpp"
#include "parser.hpp"

IRInstr::IRInstr(IRInstrType type, Obj *operand)
    : type{type}, operand{operand} {}

IRFunc::IRFunc(std::vector<std::string> args, std::vector<Type *> arg_types,
               Type *ret_type, std::vector<IRInstr> code, std::string name)
    : args{args}, arg_types{arg_types}, ret_type{ret_type}, code{code},
      name{name} {}
IRFunc::IRFunc() {}

IR::IR(std::vector<Node *> nodes) {
    std::vector<std::string> dummy_arg;
    std::vector<Type *> dummy_types;
    std::vector<IRInstr> code;
    for (auto node : nodes) {
        gen_ir(node, code);
    }

    Obj *zero = new Obj(OBJ_INT);
    zero->number = 0;
    IRInstr push(IR_PUSH, zero);
    code.push_back(push);
    IRInstr ret_zero(IR_RET, NULL);
    code.push_back(ret_zero);
    Type *ret_type = new Type(TY_INT);
    IRFunc func(dummy_arg, dummy_types, ret_type, code, "main");
    func_map["main"] = func;
}

IRFunc IR::get_func(std::string name) { return func_map[name]; }

void IR::gen_ir(Node *node, std::vector<IRInstr> &code) {
    if (node->type == ND_NUMBER) {
        Obj *obj = new Obj(OBJ_INT);
        obj->number = node->number;
        IRInstr instr(IR_PUSH, obj);
        code.push_back(instr);
    } else if (node->type == ND_STRING) {
        Obj *obj = new Obj(OBJ_STRING);
        obj->str = node->str;
        IRInstr instr(IR_PUSH, obj);
        code.push_back(instr);
    } else if (node->type == ND_FLOAT) {
        Obj *obj = new Obj(OBJ_FLOAT);
        obj->float_number = node->float_number;
        IRInstr instr(IR_PUSH, obj);
        code.push_back(instr);
    } else if (node->type == ND_VAR) {
        Obj *obj = new Obj(OBJ_NAME);
        obj->name = node->ident;
        IRInstr instr(IR_LOAD, obj);
        code.push_back(instr);
    } else if (node->type == ND_BIN) {
        gen_ir(node->bin.lhs, code);
        gen_ir(node->bin.rhs, code);
        if (node->bin.op == OP_ADD) {
            IRInstr instr(IR_ADD, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_SUB) {
            IRInstr instr(IR_SUB, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_MUL) {
            IRInstr instr(IR_MUL, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_DIV) {
            IRInstr instr(IR_DIV, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_MOD) {
            IRInstr instr(IR_MOD, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_ADDF) {
            IRInstr instr(IR_ADDF, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_SUBF) {
            IRInstr instr(IR_SUBF, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_MULF) {
            IRInstr instr(IR_MULF, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_DIVF) {
            IRInstr instr(IR_DIVF, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_MODF) {
            IRInstr instr(IR_MODF, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_GREATER) {
            IRInstr instr(IR_GREATER, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_LESS) {
            IRInstr instr(IR_LESS, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_GREATER_EQ) {
            IRInstr instr(IR_GREATER_EQ, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_LESS_EQ) {
            IRInstr instr(IR_LESS_EQ, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_EQ) {
            IRInstr instr(IR_EQ, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_NOT_EQ) {
            IRInstr instr(IR_NOT_EQ, NULL);
            code.push_back(instr);
        } else if (node->bin.op == OP_PTR_ASSIGN) {
            IRInstr instr(IR_STORE_PTR, NULL);
            code.push_back(instr);
        } else {
            error("unknown binary operator");
        }
    } else if (node->type == ND_UNARY) {
        gen_ir(node->unary.expr, code);
        if (node->unary.op == OP_DEREF) {
            IRInstr instr(IR_LOAD_PTR, NULL);
            code.push_back(instr);
        } else if (node->unary.op == OP_NOT) {
            IRInstr instr(IR_NOT, NULL);
            code.push_back(instr);
        } else {
            error("unknown unary operator");
        }
    } else if (node->type == ND_IF) {
        gen_ir(node->if_expr.cond, code);
        // Obj *obj = new Obj(OBJ_INT);
        // obj->number = 0;
        // IRInstr push_instr(IR_PUSH, obj);
        // code.push_back(push_instr);
        // IRInstr instr(IR_NOT_EQ, NULL);
        // code.push_back(instr);

        std::vector<IRInstr> then_body;
        gen_ir(node->if_expr.then_expr, then_body);
        std::vector<IRInstr> else_body;
        gen_ir(node->if_expr.else_expr, else_body);

        Obj *then_obj = new Obj(OBJ_CODE);
        then_obj->code = then_body;
        Obj *else_obj = new Obj(OBJ_CODE);
        else_obj->code = else_body;
        IRInstr then_instr(IR_PUSH, then_obj);
        IRInstr else_instr(IR_PUSH, else_obj);
        code.push_back(then_instr);
        code.push_back(else_instr);

        IRInstr br_instr(IR_BR, NULL);
        code.push_back(br_instr);
    } else if (node->type == ND_LET_IN) {
        gen_ir(node->let_in.body, code);
        Obj *obj = new Obj(OBJ_NAME);
        obj->name = node->let_in.name;
        IRInstr instr(IR_STORE, obj);
        code.push_back(instr);
        gen_ir(node->let_in.next_expr, code);
    } else if (node->type == ND_LET_FUN) {
        std::vector<IRInstr> code;
        gen_ir(node->let_fun.body, code);
        IRInstr ret_val(IR_RET, NULL);
        code.push_back(ret_val);
        std::vector<Type *> arg_types = node->type_kind->arg_types;
        Type *ret_type = node->type_kind->ret_type;
        IRFunc new_func(node->let_fun.args, arg_types, ret_type, code,
                        node->let_fun.name);
        new_func.is_extern = false;

        func_map[node->let_fun.name] = new_func;
    } else if (node->type == ND_LET_EXTERN) {
        std::vector<IRInstr> dummy_body;
        std::vector<Type *> arg_types = node->type_kind->arg_types;
        Type *ret_type = node->type_kind->ret_type;
        IRFunc new_func(node->let_extern.args, arg_types, ret_type, dummy_body,
                        node->let_extern.name);
        new_func.is_extern = true;

        func_map[node->let_extern.name] = new_func;
    } else if (node->type == ND_APP) {
        Obj *name = new Obj(OBJ_NAME);
        name->name = node->app_expr.name;
        name->size = node->app_expr.args.size();
        for (auto arg : node->app_expr.args) {
            gen_ir(arg, code);
        }
        IRInstr call(IR_CALL, name);
        code.push_back(call);
    } else if (node->type == ND_COMPOUND) {
        for (auto expr : node->compound.exprs) {
            gen_ir(expr, code);
        }
    } else if (node->type == ND_NEW) {
        Obj *operand = new Obj(OBJ_TYPE);
        operand->ty = node->new_expr.ty;
        IRInstr instr(IR_ALLOC, operand);
        code.push_back(instr);
    }
}

void IRInstr::print_instr() {
    if (type == IR_ADD) {
        std::cout << "ADD";
    } else if (type == IR_SUB) {
        std::cout << "SUB";
    } else if (type == IR_MUL) {
        std::cout << "MUL";
    } else if (type == IR_DIV) {
        std::cout << "DIV";
    } else if (type == IR_MOD) {
        std::cout << "MOD";
    } else if (type == IR_PUSH) {
        std::cout << "PUSH ";
        operand->print_obj();
    } else if (type == IR_STORE) {
        std::cout << "STORE";
        operand->print_obj();
    } else if (type == IR_LOAD) {
        std::cout << "LOAD ";
        operand->print_obj();
    } else if (type == IR_CALL) {
        std::cout << "CALL";
        operand->print_obj();
    } else if (type == IR_GREATER) {
        std::cout << "GREATER";
    } else if (type == IR_LESS) {
        std::cout << "LESS";
    } else if (type == IR_GREATER_EQ) {
        std::cout << "GREATER_EQ";
    } else if (type == IR_LESS_EQ) {
        std::cout << "LESS_EQ";
    } else if (type == IR_EQ) {
        std::cout << "EQ ";
    } else if (type == IR_NOT_EQ) {
        std::cout << "NOT_EQ ";
    } else if (type == IR_RET) {
        std::cout << "RET";
    }
}

void IRFunc::print_ir_func() {
    std::cout << name << " ";
    for (auto arg : args) {
        std::cout << arg << " ";
    }
    std::cout << ":" << std::endl;
    for (auto instr : code) {
        std::cout << "  ";
        instr.print_instr();
        std::cout << std::endl;
    }
}

void IR::print_ir() {
    for (auto i : func_map) {
        i.second.print_ir_func();
    }
}

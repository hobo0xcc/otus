#include "vm.hpp"
#include "ir.hpp"

VM::VM(IR ir) : ir{ir}, flag{false} {}

void VM::run_instr(IRInstr instr, VMEnv *e) {
    if (instr.type == IR_ADD || instr.type == IR_SUB || instr.type == IR_MUL ||
        instr.type == IR_DIV || instr.type == IR_GREATER ||
        instr.type == IR_LESS || instr.type == IR_GREATER_EQ ||
        instr.type == IR_LESS_EQ || instr.type == IR_EQ ||
        instr.type == IR_NOT_EQ) {
        Obj *rhs = stack.top();
        stack.pop();
        Obj *lhs = stack.top();
        stack.pop();

        if (lhs->type != OBJ_INT || rhs->type != OBJ_INT) {
            error("object type must be integer");
        }
        Obj *result = new Obj(OBJ_INT);
        if (instr.type == IR_ADD) {
            result->number = lhs->number + rhs->number;
        } else if (instr.type == IR_SUB) {
            result->number = lhs->number - rhs->number;
        } else if (instr.type == IR_MUL) {
            result->number = lhs->number * rhs->number;
        } else if (instr.type == IR_DIV) {
            result->number = lhs->number / rhs->number;
        } else if (instr.type == IR_GREATER) {
            result->number = lhs->number > rhs->number;
        } else if (instr.type == IR_LESS) {
            result->number = lhs->number < rhs->number;
        } else if (instr.type == IR_GREATER_EQ) {
            result->number = lhs->number >= rhs->number;
        } else if (instr.type == IR_LESS_EQ) {
            result->number = lhs->number <= rhs->number;
        } else if (instr.type == IR_EQ) {
            result->number = lhs->number == rhs->number;
        } else if (instr.type == IR_NOT_EQ) {
            result->number = lhs->number != rhs->number;
        } else if (instr.type == IR_MOD) {
            result->number = lhs->number % rhs->number;
        }
        stack.push(result);
    } else if (instr.type == IR_PUSH) {
        stack.push(instr.operand);
    } else if (instr.type == IR_STORE) {
        if (instr.operand->type != OBJ_NAME) {
            error("operand must be name");
        }
        e->set(instr.operand->name, stack.top());
        stack.pop();
    } else if (instr.type == IR_LOAD) {
        if (instr.operand->type != OBJ_NAME) {
            error("operand must be name");
        }
        if (e->get(instr.operand->name) == NULL) {
            error("variable not found");
        }

        stack.push(e->get(instr.operand->name));
    } else if (instr.type == IR_CALL) {
        if (instr.operand->type != OBJ_NAME) {
            error("operand must be name");
        }
        if (ir.func_map.find(instr.operand->name) == ir.func_map.end()) {
            error("function not found");
        }

        IRFunc func = ir.get_func(instr.operand->name);
        VMEnv *new_env = new VMEnv(NULL);
        int arg_size = func.args.size();
        for (int i = 0; i < arg_size; i++) {
            Obj *arg = stack.top();
            stack.pop();
            new_env->set(func.args[arg_size - i - 1], arg);
        }
        Obj *obj = run_func(func, new_env);
        stack.push(obj);
    } else if (instr.type == IR_BR) {
        Obj *else_code = stack.top();
        stack.pop();
        Obj *then_code = stack.top();
        stack.pop();
        if (then_code->type != OBJ_CODE || else_code->type != OBJ_CODE) {
            error("object must be code");
        }

        VMEnv *new_env = new VMEnv(e);
        Obj *obj;
        Obj *cond = stack.top();
        stack.pop();
        if (cond->type != OBJ_INT) {
            error("condition must be integer");
        }
        if (cond->number) {
            obj = run(then_code->code, new_env);
        } else {
            obj = run(else_code->code, new_env);
        }
        stack.push(obj);
    } else if (instr.type == IR_RET) {
        return;
    } else {
        error("unknown instruction");
    }
}

Obj *VM::run(std::vector<IRInstr> code, VMEnv *e) {
    for (auto instr : code) {
        run_instr(instr, e);
    }

    Obj *obj = stack.top();
    stack.pop();

    return obj;
}

Obj *VM::run_func(IRFunc func, VMEnv *e) {
    Obj *obj = run(func.code, e);
    return obj;
}

Obj *VM::run_main() {
    if (ir.func_map.find("main") == ir.func_map.end()) {
        error("main function not found");
    }

    VMEnv *e = new VMEnv(NULL);

    return run_func(ir.get_func("main"), e);
}

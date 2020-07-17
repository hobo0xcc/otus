#pragma once

#include <map>

#include "error.hpp"
#include "parser.hpp"

typedef struct Obj Obj;

typedef enum IRInstrType {
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_ADDF,
    IR_SUBF,
    IR_MULF,
    IR_DIVF,
    IR_MODF,
    IR_NOT,
    IR_PUSH,
    IR_POP,
    IR_STORE,
    IR_LOAD,
    IR_STORE_PTR,
    IR_LOAD_PTR,
    IR_ALLOC,
    IR_CALL,
    IR_BR,
    IR_EQ,
    IR_NOT_EQ,
    IR_GREATER,
    IR_LESS,
    IR_GREATER_EQ,
    IR_LESS_EQ,
    IR_RET,
} IRInstrType;

class IRInstr {
  public:
    IRInstrType type;
    Obj *operand;

    IRInstr(IRInstrType type, Obj *operand);
    void print_instr();
};

class IRFunc {
  public:
    std::vector<IRInstr> code;
    std::vector<std::string> args;
    std::vector<Type *> arg_types;
    Type *ret_type;
    std::string name;
    bool is_extern;

    IRFunc(std::vector<std::string> args, std::vector<Type *> arg_types,
           Type *ret_type, std::vector<IRInstr> code, std::string name);
    IRFunc();
    void print_ir_func();
};

class IR {
  public:
    std::map<std::string, IRFunc> func_map;

    IR(std::vector<Node *> nodes);
    void gen_ir(Node *node, std::vector<IRInstr> &code);
    IRFunc get_func(std::string name);
    void print_ir();
};

typedef enum ObjType {
    OBJ_INT,
    OBJ_FLOAT,
    OBJ_NAME,
    OBJ_CODE,
    OBJ_STRING,
    OBJ_TYPE,
} ObjType;

struct Obj {
    ObjType type;
    struct {
        int number;
        double float_number;
        size_t size;
        std::string name;
        std::vector<IRInstr> code;
        std::string str;
        Type *ty;
    };

    Obj(ObjType type) : type{type} {};
    void print_obj() {
        if (type == OBJ_INT) {
            std::cout << number;
        } else if (type == OBJ_NAME) {
            std::cout << name;
        } else if (type == OBJ_CODE) {
            std::cout << "code: " << std::endl;
            for (auto instr : code) {
                instr.print_instr();
                std::cout << std::endl;
            }
        } else if (type == OBJ_STRING) {
            std::cout << "\"" << str << "\"";
        } else {
            std::cout << "unknown obj" << std::endl;
        }
    }
};

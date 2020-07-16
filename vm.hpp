#include "error.hpp"
#include "ir.hpp"
#include "parser.hpp"

#include <stack>

typedef struct VMEnv VMEnv;
struct VMEnv {
    std::map<std::string, Obj *> var_map;
    VMEnv *parent;

    VMEnv(VMEnv *parent) : parent{parent} {};
    Obj *get(std::string name) {
        if (var_map.find(name) != var_map.end()) {
            return var_map[name];
        } else {
            if (parent == NULL) {
                return NULL;
            } else {
                return parent->get(name);
            }
        }
    };

    void set(std::string name, Obj *obj) { var_map[name] = obj; };
};

class VM {
  private:
    bool flag;
    IR ir;
    std::stack<Obj *> stack;

  public:
    VM(IR ir);
    bool is_builtin_func(std::string name);
    Obj *run_builtin(std::string name);
    void run_instr(IRInstr instr, VMEnv *e);
    Obj *run(std::vector<IRInstr> code, VMEnv *e);
    Obj *run_func(IRFunc func, VMEnv *e);
    Obj *run_main();
};

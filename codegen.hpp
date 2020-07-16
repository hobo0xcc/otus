#include "ir.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/EHPersonalities.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include <memory>
#include <stack>

typedef struct CodegenEnv CodegenEnv;

struct CodegenEnv {
    std::map<std::string, llvm::Value *> var_map;
    CodegenEnv *parent;

    CodegenEnv(CodegenEnv *parent) : parent{parent} {};
    llvm::Value *get(std::string name) {
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
    void set(std::string name, llvm::Value *val) { var_map[name] = val; };
};

class Codegen {
  private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    IR ir;
    std::stack<llvm::Value *> stack;
    std::stack<std::vector<IRInstr>> code_stack;

  public:
    Codegen(IR ir);
    void gen_instr(IRInstr instr, CodegenEnv *e);
    llvm::Type *convert_type_to_llvm_type(Type *ty);
    void gen_function(IRFunc func);
    void gen_function_declare(IRFunc func);
    void gen_code(std::vector<IRInstr> code, CodegenEnv *e);
    void gc_alloc_init();
    void gc_collect_init();
    void gc_root_init();
    void gc_setup();
    void gen();
    void generate_object_file(std::string output);
    void print_code();
};

#include "codegen.hpp"
#include "error.hpp"
#include "ir.hpp"
#include "parser.hpp"

#include <atomic>
#include <cstdio>
#include <memory>
#include <ostream>
#include <system_error>

Codegen::Codegen(IR ir) : builder(context), ir{ir} {
    module = std::make_unique<llvm::Module>("jit", context);
}

// Generate LLVM IR Code from IR Code.
void Codegen::gen_instr(IRInstr instr, CodegenEnv *e) {
    if (instr.type == IR_ADD || instr.type == IR_SUB || instr.type == IR_MUL ||
        instr.type == IR_DIV || instr.type == IR_MOD || instr.type == IR_ADDF ||
        instr.type == IR_SUBF || instr.type == IR_MULF ||
        instr.type == IR_DIVF || instr.type == IR_MODF || instr.type == IR_EQ ||
        instr.type == IR_NOT_EQ || instr.type == IR_GREATER ||
        instr.type == IR_LESS || instr.type == IR_GREATER_EQ ||
        instr.type == IR_LESS_EQ || instr.type == IR_LOGAND || instr.type == IR_LOGOR || instr.type == IR_BITAND || instr.type == IR_BITXOR || instr.type == IR_BITOR) {
        llvm::Value *rhs = stack.top();
        stack.pop();
        llvm::Value *lhs = stack.top();
        stack.pop();

        // std::string name_lhs;
        // llvm::raw_string_ostream lhs_rso(name_lhs);
        // lhs->getType()->print(lhs_rso);
        // std::cout << lhs_rso.str()<< std::endl;
        // std::string name_rhs;
        // llvm::raw_string_ostream rhs_rso(name_rhs);
        // rhs->getType()->print(rhs_rso);
        // std::cout << rhs_rso.str() << std::endl;

        switch (instr.type) {
        case IR_ADD:
            stack.push(builder.CreateAdd(lhs, rhs, "addtmp"));
            break;
        case IR_SUB:
            stack.push(builder.CreateSub(lhs, rhs, "subtmp"));
            break;
        case IR_MUL:
            stack.push(builder.CreateMul(lhs, rhs, "multmp"));
            break;
        case IR_DIV:
            stack.push(builder.CreateExactSDiv(lhs, rhs, "divtmp"));
            break;
        case IR_MOD:
            stack.push(builder.CreateSRem(lhs, rhs, "remtmp"));
            break;
        case IR_ADDF:
            stack.push(builder.CreateFAdd(lhs, rhs, "addftmp"));
            break;
        case IR_SUBF:
            stack.push(builder.CreateFSub(lhs, rhs, "subftmp"));
            break;
        case IR_MULF:
            stack.push(builder.CreateFMul(lhs, rhs, "mulftmp"));
            break;
        case IR_DIVF:
            stack.push(builder.CreateFDiv(lhs, rhs, "divftmp"));
            break;
        case IR_MODF:
            stack.push(builder.CreateFRem(lhs, rhs, "remftmp"));
            break;
        case IR_EQ:
            stack.push(
                builder.CreateICmp(llvm::CmpInst::ICMP_EQ, lhs, rhs, "eqtmp"));
            break;
        case IR_NOT_EQ:
            stack.push(builder.CreateICmp(llvm::CmpInst::ICMP_NE, lhs, rhs,
                                          "noteqtmp"));
            break;
        case IR_GREATER:
            stack.push(
                builder.CreateICmp(llvm::CmpInst::ICMP_SGT, lhs, rhs, "gttmp"));
            break;
        case IR_LESS:
            stack.push(
                builder.CreateICmp(llvm::CmpInst::ICMP_SLT, lhs, rhs, "lttmp"));
            break;
        case IR_GREATER_EQ:
            stack.push(
                builder.CreateICmp(llvm::CmpInst::ICMP_SGE, lhs, rhs, "getmp"));
            break;
        case IR_LESS_EQ:
            stack.push(
                builder.CreateICmp(llvm::CmpInst::ICMP_SLE, lhs, rhs, "letmp"));
            break;
        case IR_LOGAND:
        case IR_BITAND:
            stack.push(builder.CreateAnd(lhs, rhs, "andtmp"));
            break;
        case IR_LOGOR:
        case IR_BITOR:
            stack.push(builder.CreateOr(lhs, rhs, "ortmp"));
            break;
        case IR_BITXOR:
            stack.push(builder.CreateXor(lhs, rhs, "xortmp"));
            break;
        default:
            error("unknown operator in codegen");
        }
    } else if (instr.type == IR_NOT) {
        llvm::Value *val = stack.top();
        stack.pop();
        stack.push(builder.CreateNeg(val, "negtmp"));
    } else if (instr.type == IR_PUSH) {
        if (instr.operand->type == OBJ_CODE) {
            code_stack.push(instr.operand->code);
        } else if (instr.operand->type == OBJ_INT) {
            stack.push(llvm::ConstantInt::get(
                context, llvm::APInt(32, instr.operand->number, true)));
        } else if (instr.operand->type == OBJ_FLOAT) {
            stack.push(llvm::ConstantFP::get(
                context, llvm::APFloat(instr.operand->float_number)));
        } else if (instr.operand->type == OBJ_BOOL) {
            stack.push(llvm::ConstantInt::get(context, llvm::APInt(8, instr.operand->bool_val)));
        } else if (instr.operand->type == OBJ_STRING) {
            llvm::Value *val =
                builder.CreateGlobalStringPtr(instr.operand->str);
            stack.push(val);
        } else {
            error("invalid operand");
        }
    } else if (instr.type == IR_POP) {
        stack.pop();
    } else if (instr.type == IR_STORE) {
        if (instr.operand->type != OBJ_NAME) {
            error("operand must be name");
        }

        std::string name = instr.operand->name;

        llvm::Value *val = stack.top();
        stack.pop();
        e->set(name, val);
    } else if (instr.type == IR_LOAD) {
        if (instr.operand->type != OBJ_NAME) {
            error("operand must be name");
        }

        std::string name = instr.operand->name;
        llvm::Value *val;
        if ((val = e->get(name)) == NULL) {
            error("undeclared variable: %s", name.c_str());
        }

        stack.push(val);
    } else if (instr.type == IR_STORE_PTR) {
        llvm::Value *rhs = stack.top();
        stack.pop();
        llvm::Value *lhs = stack.top();
        stack.pop();

        builder.CreateStore(rhs, lhs);
        // llvm::Value *val = builder.CreateGEP(lhs, 0);
        stack.push(rhs);
    } else if (instr.type == IR_LOAD_PTR) {
        llvm::Value *ptr = stack.top();
        stack.pop();

        llvm::Value *val = builder.CreateLoad(ptr);
        stack.push(val);
    } else if (instr.type == IR_ALLOC) {
        llvm::Type *orig_type =
            convert_type_to_llvm_type(instr.operand->ty->ptr_to);
        llvm::Function *alloc = module->getFunction("alloc");
        std::vector<llvm::Value *> alloc_arg;
        llvm::Value *ptr_size =
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                   orig_type->getPrimitiveSizeInBits() / 8);
        alloc_arg.push_back(ptr_size);
        llvm::Value *pp =
            builder.CreateAlloca(convert_type_to_llvm_type(instr.operand->ty));
        llvm::Value *val = builder.CreateCall(alloc, alloc_arg);
        llvm::Value *casted_pp = builder.CreateBitCast(
            pp, llvm::PointerType::getUnqual(val->getType()));
        builder.CreateStore(val, casted_pp);
        llvm::Function *callee = module->getFunction("llvm.gcroot");
        std::vector<llvm::Value *> argv;
        argv.push_back(casted_pp);
        argv.push_back(
            llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(context)));
        builder.CreateCall(callee, argv);
        llvm::Value *res = builder.CreateBitCast(
            val, convert_type_to_llvm_type(instr.operand->ty));
        stack.push(res);
    } else if (instr.type == IR_CALL) {
        if (instr.operand->type != OBJ_NAME) {
            error("operand must be name");
        }

        std::string name;
        if (ir.func_map[instr.operand->name].is_extern) {
            // name.push_back('_');
        }
        name.append(instr.operand->name);
        llvm::Function *callee = module->getFunction(name);
        if (!callee) {
            // There's nothing to do.
        }
        if (callee->arg_size() != instr.operand->size) {
            error("incorrect arguments passed");
        }
        CodegenEnv *new_env = new CodegenEnv(e);
        std::vector<llvm::Value *> argv;
        for (int i = 0; i < callee->arg_size(); i++) {
            argv.push_back(stack.top());
            stack.pop();
        }

        if (callee->getReturnType()->isVoidTy()) {
            builder.CreateCall(callee, argv);
            stack.push(nullptr);
        } else {
            stack.push(builder.CreateCall(callee, argv, "calltmp"));
            std::cout << name << std::endl;
            std::flush(std::cout);
        }
    } else if (instr.type == IR_BR) {
        std::vector<IRInstr> else_code = code_stack.top();
        code_stack.pop();
        std::vector<IRInstr> then_code = code_stack.top();
        code_stack.pop();
        llvm::Value *cond = stack.top();
        stack.pop();

        llvm::Function *function = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *then_bb =
            llvm::BasicBlock::Create(context, "then", function);
        llvm::BasicBlock *else_bb = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock *merge_bb =
            llvm::BasicBlock::Create(context, "ifcont");
        builder.CreateCondBr(cond, then_bb, else_bb);

        builder.SetInsertPoint(then_bb);
        CodegenEnv *then_env = new CodegenEnv(e);
        gen_code(then_code, then_env);
        llvm::Value *then_v = stack.top();
        stack.pop();
        builder.CreateBr(merge_bb);
        then_bb = builder.GetInsertBlock();

        function->getBasicBlockList().push_back(else_bb);
        builder.SetInsertPoint(else_bb);
        CodegenEnv *else_env = new CodegenEnv(e);
        gen_code(else_code, else_env);
        llvm::Value *else_v = stack.top();
        stack.pop();
        builder.CreateBr(merge_bb);
        else_bb = builder.GetInsertBlock();

        function->getBasicBlockList().push_back(merge_bb);
        builder.SetInsertPoint(merge_bb);
        if (then_v == nullptr || else_v == nullptr) {
            stack.push(nullptr);
            return;
        }
        llvm::PHINode *PN = builder.CreatePHI(then_v->getType(), 2, "iftmp");
        PN->addIncoming(then_v, then_bb);
        PN->addIncoming(else_v, else_bb);
        stack.push(PN);
    } else if (instr.type == IR_RET) {
        llvm::Value *val = stack.top();
        stack.pop();
        builder.CreateRet(val);
    } else {
        error("unknown IR instruction");
    }
}

llvm::Type *Codegen::convert_type_to_llvm_type(Type *ty) {
    if (ty->kind == TY_INT) {
        return llvm::Type::getInt32Ty(context);
    } else if (ty->kind == TY_STRING) {
        return llvm::Type::getInt8PtrTy(context);
    } else if (ty->kind == TY_BOOL) {
        return llvm::Type::getInt1Ty(context);
    } else if (ty->kind == TY_FLOAT) {
        return llvm::Type::getDoubleTy(context);
    } else if (ty->kind == TY_VOID) {
        return llvm::Type::getVoidTy(context);
    } else if (ty->kind == TY_FUN) {
        std::vector<llvm::Type *> arg_types;
        int arg_len = ty->arg_types.size();
        for (int i = 0; i < arg_len; i++) {
            arg_types.push_back(convert_type_to_llvm_type(ty->arg_types[i]));
        }
        llvm::Type *ret_type = convert_type_to_llvm_type(ty->ret_type);
        llvm::FunctionType *ft =
            llvm::FunctionType::get(ret_type, arg_types, false);
        return ft;
    } else if (ty->kind == TY_PTR) {
        llvm::Type *ptr_type =
            llvm::PointerType::get(convert_type_to_llvm_type(ty->ptr_to), 0);
        return ptr_type;
    } else {
        error("unknown type");
        return NULL;
    }
}

void Codegen::gen_function(IRFunc func) {
    // std::vector<llvm::Type *> arg_types(func.args.size(),
    //                                     llvm::Type::getInt32Ty(context));
    // llvm::FunctionType *ft = llvm::FunctionType::get(
    //     llvm::Type::getInt32Ty(context), arg_types, false);
    // llvm::Function *f = llvm::Function::Create(
    //     ft, llvm::Function::ExternalLinkage, func.name, module.get());
    if (func.is_extern) {
        return;
    }
    llvm::Function *f = module->getFunction(func.name);
    f->setGC("shadow-stack");

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", f);
    builder.SetInsertPoint(bb);
    CodegenEnv *e = new CodegenEnv(NULL);
    int index = 0;
    for (auto &arg : f->args()) {
        std::string name = func.args[index++];
        arg.setName(name);
        e->set(name, &arg);
    }

    gen_code(func.code, e);
    llvm::verifyFunction(*f);
}

void Codegen::gen_function_declare(IRFunc func) {
    std::vector<llvm::Type *> arg_types;
    for (int i = 0; i < func.arg_types.size(); i++) {
        Type *ty = func.arg_types[i];
        arg_types.push_back(convert_type_to_llvm_type(ty));
    }
    llvm::Type *ret_type = convert_type_to_llvm_type(func.ret_type);
    llvm::FunctionType *ft =
        llvm::FunctionType::get(ret_type, arg_types, false);
    std::string name;
    if (func.is_extern) {
        // name.push_back('_');
    }
    name.append(func.name);
    llvm::Function *f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, name, module.get());
    // llvm::EHPersonality pers = llvm::EHPersonality::GNU_CXX;
    // std::string pers_name = llvm::getEHPersonalityName(pers);
    // module->getOrInsertFunction(pers_name,
    // llvm::FunctionType::get(llvm::Type::getInt32Ty(context), true));
    // f->setPersonalityFn(module->getFunction(llvm::getEHPersonalityName(pers)));
}

void Codegen::gen_code(std::vector<IRInstr> code, CodegenEnv *e) {
    for (auto instr : code) {
        gen_instr(instr, e);
    }
}

void Codegen::gc_alloc_init() {
    std::vector<llvm::Type *> arg_types;
    llvm::Type *ty = llvm::Type::getInt64Ty(context);
    arg_types.push_back(ty);
    llvm::Type *ret_type = llvm::Type::getInt8PtrTy(context);
    llvm::FunctionType *ft =
        llvm::FunctionType::get(ret_type, arg_types, false);
    std::string name = "alloc";
    llvm::Function *f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, name, module.get());
}

void Codegen::gc_collect_init() {
    std::vector<llvm::Type *> arg_types;
    llvm::Type *ty = llvm::Type::getInt64Ty(context);
    llvm::Type *ret_type = llvm::Type::getVoidTy(context);
    llvm::FunctionType *ft =
        llvm::FunctionType::get(ret_type, arg_types, false);
    std::string name = "collect";
    llvm::Function *f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, name, module.get());
}

void Codegen::gc_root_init() {
    std::vector<llvm::Type *> arg_types;
    llvm::Type *ptrloc =
        llvm::PointerType::getUnqual(llvm::Type::getInt8PtrTy(context));
    llvm::Type *metadata = llvm::Type::getInt8PtrTy(context);
    arg_types.push_back(ptrloc);
    arg_types.push_back(metadata);
    llvm::Type *ret_type = llvm::Type::getVoidTy(context);
    llvm::FunctionType *ft =
        llvm::FunctionType::get(ret_type, arg_types, false);
    std::string name = "llvm.gcroot";
    llvm::Function *f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, name, module.get());
}

void Codegen::gc_setup() {
    gc_alloc_init();
    gc_collect_init();
    gc_root_init();
}

void Codegen::gen() {
    gc_setup();

    for (auto func : ir.func_map) {
        gen_function_declare(func.second);
    }
    for (auto func : ir.func_map) {
        gen_function(func.second);
    }
}

#include "llvm/CodeGen/CommandFlags.inc"

void Codegen::generate_object_file(std::string output) {
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    std::string error_str;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error_str);
    if (!target) {
        llvm::errs() << error_str;
        exit(1);
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt = InitTargetOptionsFromCodeGenFlags();

    auto rm = llvm::Optional<llvm::Reloc::Model>();
    TargetMachine *target_machine =
        target->createTargetMachine(target_triple, cpu, features, opt, rm);
    module->setDataLayout(target_machine->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest(output, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message();
        exit(1);
    }

    llvm::PassManagerBuilder *pm_builder =
        new (std::nothrow) llvm::PassManagerBuilder();

    TargetLibraryInfoImpl TLII(Triple(module->getTargetTriple()));
    llvm::legacy::PassManager pass;
    // pass.add(new TargetLibraryInfoWrapperPass(TLII));
    // pass.add(&TPC);
    // pass.add(MMIWP);
    // pass.add(llvm::createFreeMachineFunctionPass());
    auto file_type = llvm::CGFT_ObjectFile;
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type,
                                            true)) {
        llvm::errs() << "target_machine can't emit a file of this type";
        exit(1);
    }

    pass.run(*module);
    dest.flush();
}

void Codegen::print_code() { module->print(llvm::errs(), nullptr); }

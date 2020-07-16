#include <fstream>
#include <iostream>
#include <iterator>
#include <ostream>
#include <vector>

#include "codegen.hpp"
#include "error.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "typing.hpp"
#include "vm.hpp"

class Config {
  public:
    bool run_with_vm;
    std::string input_file;
    std::string output_file;

    Config() : run_with_vm{false}, input_file{}, output_file{} {}

    void print_usage() {
        std::cout << "Usage: otus [OPTIONS] [INPUT]" << std::endl;
        std::cout << "OPTIONS:" << std::endl;
        std::cout << "\t-o <output>\t\tSpecify output object file."
                  << std::endl;
    }

    void parse_argv(int argc, char **argv) {
        if (argc == 1) {
            print_usage();
            exit(1);
        }

        int cur = 1;
        for (; cur < argc; cur++) {
            std::string arg(argv[cur]);
            if (arg[0] == '-') {
                if (arg == "-o") {
                    cur++;
                    output_file = std::string(argv[cur]);
                } else if (arg == "-vm") {
                    run_with_vm = true;
                } else if (arg == "--help") {
                    print_usage();
                    std::exit(0);
                } else {
                    error("unknown option: %s", arg.c_str());
                }
            } else {
                input_file = std::string(argv[cur]);
            }
        }
    }
};

int main(int argc, char **argv) {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // llvm::PassRegistry *Registry = llvm::PassRegistry::getPassRegistry();
    // initializeCore(*Registry);
    // initializeCodeGen(*Registry);
    // initializeLoopStrengthReducePass(*Registry);
    // initializeLowerIntrinsicsPass(*Registry);

    llvm::linkAllBuiltinGCs();

    Config config;
    config.parse_argv(argc, argv);
    if (config.input_file.empty()) {
        error("input file unspecified");
    }
    std::ifstream file(config.input_file);
    std::string buf;
    if (file.is_open()) {
        buf = std::string((std::istreambuf_iterator<char>(file)),
                          (std::istreambuf_iterator<char>()));
    } else {
        error("could not open the file: %s", argv[1]);
    }

    // std::cout << buf << std::endl;
    Lexer lex(buf);
    std::vector<Token> tokens = lex.tokenize();
    Parser parser(tokens);
    std::vector<Node *> nodes = parser.parse_all();
    Typing typing(nodes);
    std::vector<Node *> typed_nodes = typing.infer();
    IR ir(nodes);
    // ir.print_ir();
    if (config.run_with_vm) {
        VM vm(ir);
        Obj *ret = vm.run_main();
        ret->print_obj();
    } else {
        Codegen codegen(ir);
        codegen.gen();
        codegen.print_code();
        codegen.generate_object_file(config.output_file);
    }
    // VM vm(ir);
    // Obj *obj = vm.run_main();
    // obj->print_obj();
    // std::cout << std::endl;

    return 0;
}

CXX:=g++
CXXFLAGS:=$(shell llvm-config --cxxflags --ldflags --system-libs --libs all)

.PHONY: otus
otus:
	$(CXX) -g $(CXXFLAGS) -o $@ main.cpp lexer.cpp parser.cpp typing.cpp ir.cpp vm.cpp codegen.cpp error.cpp
	$(CXX) -std=c++11 -g -c -o runtime.o runtime/gc.cpp

clean:
	rm -rf otus otus.dSYM

CXX:=g++
CXXFLAGS:=$(shell llvm-config --cxxflags --ldflags --system-libs --libs all)

lib.o: lib.cpp
	$(CXX) -c -o $@ $^

hello: hello.ot lib.o
	../otus $< -o $@.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o lib.o ../runtime.o

float: float.ot lib.o
	../otus $< -o $@.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o lib.o ../runtime.o

gc: gc.ot lib.o
	../otus $< -o $@.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o lib.o ../runtime.o

fib: fib.ot lib.o
	../otus $< -o $@.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o lib.o ../runtime.o

fizzbuzz: fizzbuzz.ot lib.o
	../otus $< -o $@.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o lib.o ../runtime.o

pointer: pointer.ot lib.o
	../otus $< -o $@.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o lib.o ../runtime.o

clean:
	rm -rf hello float gc fib fizzbuzz pointer lib.o *.o

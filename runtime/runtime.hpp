#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>

extern "C" {
    void *alloc(size_t size);
    void collect();
}

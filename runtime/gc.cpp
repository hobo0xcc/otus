#include "runtime.hpp"

#include "shadow_stack.hpp"

struct Obj {
    void *ptr;
    bool marked;
};

static class GC {
private:
    std::map<void *, Obj *> heap;
public:
    void mark_phase() {
        auto f = [&](void **Root, const void *Meta) {
            Obj *obj = heap[*Root];
            if (obj == nullptr) return;
            if (obj->marked == false) {
                std::cout << "marked: " << obj->ptr << std::endl;
                obj->marked = true;
            }
        };
        visitGCRoots(f);
    }

    void sweep_phase() {
        for (auto it = heap.begin(); it != heap.end(); it++) {
            Obj *obj = (*it).second;
            if (obj == nullptr) continue;
            std::cout << "sweep: " << obj->ptr << std::endl;
            if (obj->marked == false) {
                free(obj);
            } else {
                obj->marked = false;
            }
        }
    }

    void collect() {
        mark_phase();
        sweep_phase();
    }

    void *alloc(size_t size) {
        collect();
        void *ptr = std::malloc(size);
        if (ptr == NULL) {
            collect();
            ptr = std::malloc(size);
            if (ptr == NULL) {
                std::cerr << "memory exhausted" << std::endl;
                std::exit(1);
            }
        }

        std::cout << "allocated: " << ptr << std::endl;

        Obj *obj = new Obj;
        obj->ptr = ptr;
        obj->marked = false;
        heap[ptr] = obj;

        return ptr;
    }

    void free(Obj *obj) {
        std::cout << "freed: " << obj->ptr << std::endl;
        heap[obj->ptr] = nullptr;
        std::free(obj->ptr);
        delete obj;
    }
} gc;

extern "C" {
    void *alloc(size_t size) {
        return gc.alloc(size);
    }

    void collect() {
        gc.collect();
    }
}

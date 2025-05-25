#include "orbit.h"

void* cmalloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) crash("Tried to allocate ptr of size %d, failed with errno %s", size, strerror(errno));
    return ptr;
}

void* crealloc(void* ptr, size_t size) {
    void* nptr = realloc(ptr, size);
    if (nptr == NULL) crash("Tried to reallocate ptr to size %d, failed with errno %s", size, strerror(errno));
    return nptr;
}

void cfree(void* ptr) {
    free(ptr);
}
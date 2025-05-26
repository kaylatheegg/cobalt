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

void* ccharalloc(size_t size, u8 c) {
    u8* ptr = malloc(size);
    if (ptr == NULL) crash("Tried to charalloc ptr of size %d, failed with errno %s", size, strerror(errno));
    for (size_t i = 0; i < size; i++) {
        ptr[i] = c;
    }
    return ptr; 
}
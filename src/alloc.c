#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "cobalt.h"
#include "crash.h"

#include "common/type.h"

void* cmalloc(usize size) {
    void* ptr = malloc(size);
    if (ptr == NULL) crash("Tried to allocate ptr of size %d, failed with errno %s\n", size, strerror(errno));
    return ptr;
}

void* crealloc(void* ptr, usize size) {
    void* nptr = realloc(ptr, size);
    if (nptr == NULL) crash("Tried to reallocate ptr to size %d, failed with errno %s\n", size, strerror(errno));
    return nptr;
}

void cfree(void* ptr) {
    free(ptr);
}

void* ccharalloc(usize size, u8 c) {
    u8* ptr = malloc(size);
    if (ptr == NULL) crash("Tried to charalloc ptr of size %d, failed with errno %s\n", size, strerror(errno));
    memset(ptr, c, size);
    return ptr; 
}
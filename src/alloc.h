#pragma once
#define ALLOC_H

#include "common/type.h"

void* cmalloc(usize size);

void* crealloc(void* ptr, usize size);

void cfree(void* ptr);

void* ccharalloc(usize size, u8 c);
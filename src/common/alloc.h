#pragma once
#define ALLOC_H

void* cmalloc(size_t size);

void* crealloc(void* ptr, size_t size);

void cfree(void* ptr);
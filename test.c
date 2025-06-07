#define test(a) a + 1

#define meow(a, b, ...) a, b, __VA_ARGS__

meow

test(1)

meow(1, 2, 3)
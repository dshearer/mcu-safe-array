#define ARRAY_SIZE 10
Array<char, ARRAY_SIZE> a = {};
a.slice<ARRAY_SIZE + 1>();
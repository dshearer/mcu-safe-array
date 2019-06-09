#define ARRAY_SIZE 10

Array<char, ARRAY_SIZE> a1 = {0};
Array<char, ARRAY_SIZE + 1> a2 = {0};
a1.assign(a2.cslice());
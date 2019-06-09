#define ARRAY_SIZE 10
Array<char, ARRAY_SIZE> a1 = {};
Array<char, ARRAY_SIZE/2> a2 = {};
a1 << a2.cslice() << a2.cslice() << a2.cslice();
#define ARRAY_SIZE 10
Array<char, ARRAY_SIZE> a = {};
a.cslice<0, ARRAY_SIZE + 1>();
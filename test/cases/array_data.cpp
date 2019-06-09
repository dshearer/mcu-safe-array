#define ARRAY_SIZE 10
Array<char, ARRAY_SIZE> a = {};
a.data<ARRAY_SIZE+1>();
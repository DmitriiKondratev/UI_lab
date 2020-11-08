#ifndef TEST_H
#define TEST_H

#include <string>
#include <iostream>
#include <cassert>

bool allPassed = true;

template<class ...args>
void test( std::string const &testName, bool (*f)(args...), args... arg )
{
    auto res = f(arg...);
    if (!res) { allPassed = false; }
    std::cout << testName << ": " << (res ? "PASSED" : "FAILED") << std::endl;
}

template<class T>
void print(FILE* stream, T* collection) {
    assert(collection);

    if (!stream) { stream = stdout; }

    size_t dim = collection->getDim();
    fprintf(stream, "[");
    if (dim) {
        fprintf(stream, "%lf", collection->getCoord(0));
        for (size_t i = 1; i < dim; ++i) {
            fprintf(stream, " %lf", collection->getCoord(i));
        }
    }
    fprintf(stream, "]\n");
}

#endif /* TEST_H */

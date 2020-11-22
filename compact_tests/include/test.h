#ifndef TEST_H
#define TEST_H

#include <string>
#include <iostream>
#include <cassert>

bool allPassed = true;

template<class ...args>
void test( std::string const &testName, bool (*f)(args...), args... arg ) {
    auto res = f(arg...);
    if (!res) { allPassed = false; }
    std::cout << testName << ": " << (res ? "PASSED" : "FAILED") << std::endl;
}

template <class T>
bool isBad(T* entity) {
    return entity == nullptr;
}

#endif // TEST_H

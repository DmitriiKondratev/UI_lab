#include <iostream>
#include <cmath>
#include <array>
#include <cassert>

#include "include/test.h"
#include "include/ILogger.h"
#include "include/IVector.h"
#include "include/ISet.h"

#define CLIENT(n) ((void*) n)
#define CLIENT_KEY 47
#define TOLERANCE 1e-6
#define DIMENSION 2

using namespace std;

double
        coords_erase[]      = {1.0, 2.0},
        coords_insert_1[]   = {15.0, 20.0},
        coords_insert_2[]   = {15.0, 20.0, 25.0};

array<double, DIMENSION>
        setData1[]          = {{1.0, 2.0},
                               {3.0, 4.0},
                               {5.0, 6.0},
                               {7.0, 8.0},
                               {9.0, 10.0}},

        setData2[]          = {{0.0, 0.0},
                               {1.0, 2.0},
                               {2.0, 5.0},
                               {3.0, 6.0},
                               {5.0, 6.0},
                               {8.0, 8.0},
                               {9.0, 10.0}},

        etalonSum[]         = {{1.0, 2.0},
                               {3.0, 4.0},
                               {5.0, 6.0},
                               {7.0, 8.0},
                               {9.0, 10.0},
                               {0.0, 0.0},
                               {2.0, 5.0},
                               {3.0, 6.0},
                               {8.0, 8.0}},

        etalonIntersect[]   = {{1.0, 2.0},
                               {5.0, 6.0},
                               {9.0, 10.0}},

        etalonDiff[]        = {{3.0, 4.0},
                               {7.0, 8.0}},

        etalonSymDiff[]     = {{3.0, 4.0},
                               {7.0, 8.0},
                               {0.0, 0.0},
                               {2.0, 5.0},
                               {3.0, 6.0},
                               {8.0, 8.0}};

ILogger *pLogger = ILogger::createLogger(CLIENT(CLIENT_KEY));

static void print(FILE* stream, IVector* vec) {
    assert(vec);

    if (!stream) { stream = stdout; }

    size_t dim = vec->getDim();
    fprintf(stream, "[");
    if (dim) {
        fprintf(stream, "%lf", vec->getCoord(0));
        for (size_t i = 1; i < dim; ++i) {
            fprintf(stream, " %lf", vec->getCoord(i));
        }
    }
    fprintf(stream, "]\n");
}

static void print(FILE* stream, ISet* set) {
    assert(set);

    if (!stream) { stream = stdout; }

    size_t size = set->getSize();
    IVector* vec;

    fprintf(stream, "{\n");
    for (size_t i = 0; i < size; ++i) {
        fprintf(stream, "\t");
        if (set->get(vec, i) == RESULT_CODE::SUCCESS) {
            print(stream, vec);
            delete vec;
        } else {
            fprintf(stream, "[]\n");
        }
    }
    fprintf(stream, "     }\n");
}

static void printHead(ISet* s1, ISet* s2) {
    cout << "Tests for set library:" << endl;

    if (s1 && s2) {
        cout << "s1 = ";
        print(nullptr, s1);

        cout << "s2 = ";
        print(nullptr, s2);
    } else {
        cout << "s1 and s2 are bad" << endl;
    }
    cout << endl;
}

ISet* createSet(array<double, DIMENSION>* data, size_t size, ILogger* pLogger) {
    ISet* set = ISet::createSet(pLogger);

    if (set != nullptr) {
        IVector* vec;

        for (size_t i = 0; i < size; ++i) {
            vec = IVector::createVector(DIMENSION, data[i].data(), pLogger);
            if (vec == nullptr) {
                delete set;
                set = nullptr;
                break;
            }
            set->insert(vec, IVector::NORM::NORM_2, TOLERANCE);
        }
    }

    return set;
}

static bool checkVector(IVector* vec, double* data) {
    assert(vec && data);

    size_t dim = vec->getDim();
    for (size_t i = 0; i < dim; ++i) {
        if (abs(vec->getCoord(i) - data[i]) > TOLERANCE) {
            return false;
        }
    }
    return true;
}

static bool checkSet(ISet* set, array<double, DIMENSION>* data) {
    assert(set && data);

    IVector* vec;
    size_t size = set->getSize();
    for (size_t i = 0; i < size; ++i) {
        if (set->get(vec, i) != RESULT_CODE::SUCCESS) {
            return false;
        }

        if (!checkVector(vec, data[i].data())) {
            delete vec;
            return false;
        }
        delete vec;
    }
    return true;
}

template <class T>
bool isBadCollection(T* collection) {
    return collection == nullptr;
}

bool isEmpty(size_t size) {
    return size == 0;
}

static void testSum(ISet* s1, ISet* s2, ILogger* pLogger) {
    assert(s1 && s2);

    auto
            goodSum = ISet::add(s1, s2, IVector::NORM::NORM_2, TOLERANCE, pLogger),
            goodSum_s1 = ISet::add(s1, s1, IVector::NORM::NORM_2, TOLERANCE, pLogger),
            badSum_nan = ISet::add(s1, s2, IVector::NORM::NORM_2, NAN, static_cast<ILogger*>(nullptr)),
            badSum_nullptr_right = ISet::add(s1, nullptr, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr)),
            badSum_nullptr_left = ISet::add(nullptr, s2, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr));

    if (goodSum) {
        test("Sum of two sets", checkSet, goodSum, etalonSum);
        delete goodSum;
    }

    if (goodSum_s1) {
        test("Sum of equal sets", checkSet, goodSum_s1, setData1);
        delete goodSum_s1;
    }

    test("Sum with nan tolerance", isBadCollection<ISet>, badSum_nan);
    test("Sum of set and null", isBadCollection<ISet>, badSum_nullptr_right);
    test("Sum of null and set", isBadCollection<ISet>, badSum_nullptr_left);
}

static void testIntersect(ISet* s1, ISet* s2, ILogger* pLogger) {
    assert(s1 && s2);

    auto
            goodIntersect = ISet::sub(s1, s2, IVector::NORM::NORM_2, TOLERANCE, pLogger),
            goodIntersect_s1 = ISet::sub(s1, s1, IVector::NORM::NORM_2, TOLERANCE, pLogger),
            badIntersect_nan = ISet::sub(s1, s2, IVector::NORM::NORM_2, NAN, static_cast<ILogger*>(nullptr)),
            badIntersect_nullptr_right = ISet::sub(s1, nullptr, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr)),
            badIntersect_nullptr_left = ISet::sub(nullptr, s2, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr));

    if (goodIntersect) {
        test("Difference of two sets", checkSet, goodIntersect, etalonDiff);
        delete goodIntersect;
    }

    if (goodIntersect_s1) {
        test("Difference of equal sets", checkSet, goodIntersect_s1, setData1);
        delete goodIntersect_s1;
    }

    test("Difference with nan tolerance", isBadCollection<ISet>, badIntersect_nan);
    test("Difference of set and null", isBadCollection<ISet>, badIntersect_nullptr_right);
    test("Difference of null and set", isBadCollection<ISet>, badIntersect_nullptr_left);
}

static void testDiff(ISet* s1, ISet* s2, ILogger* pLogger) {
    assert(s1 && s2);

    auto
            goodDiff = ISet::sub(s1, s2, IVector::NORM::NORM_2, TOLERANCE, pLogger),
            goodDiff_empty = ISet::sub(s1, s1, IVector::NORM::NORM_2, TOLERANCE, pLogger),
            badDiff_nan = ISet::sub(s1, s2, IVector::NORM::NORM_2, NAN, static_cast<ILogger*>(nullptr)),
            badDiff_nullptr_right = ISet::sub(s1, nullptr, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr)),
            badDiff_nullptr_left = ISet::sub(nullptr, s2, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr));

    if (goodDiff) {
        test("Difference of two sets", checkSet, goodDiff, etalonDiff);
        delete goodDiff;
    }

    if (goodDiff_empty) {
        test("Difference of equal sets", isEmpty, goodDiff_empty->getSize());
        delete goodDiff_empty;
    }

    test("Difference with nan tolerance", isBadCollection<ISet>, badDiff_nan);
    test("Difference of set and null", isBadCollection<ISet>, badDiff_nullptr_right);
    test("Difference of null and set", isBadCollection<ISet>, badDiff_nullptr_left);
}

static void testSymDiff(ISet* s1, ISet* s2, ILogger* pLogger) {
    assert(s1 && s2);

    auto
            goodSymDiff = ISet::symSub(s1, s2, IVector::NORM::NORM_2, TOLERANCE, nullptr),
            goodSymDiff_empty = ISet::symSub(s1, s1, IVector::NORM::NORM_2, TOLERANCE, pLogger),
            badSymDiff_nan = ISet::symSub(s1, s2, IVector::NORM::NORM_2, NAN, static_cast<ILogger*>(nullptr)),
            badSymDiff_nullptr_right = ISet::symSub(s1, nullptr, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr)),
            badSymDiff_nullptr_left = ISet::symSub(nullptr, s2, IVector::NORM::NORM_2, TOLERANCE, static_cast<ILogger*>(nullptr));

    if (goodSymDiff) {
        test("Sym difference of two sets", checkSet, goodSymDiff, etalonSymDiff);
        delete goodSymDiff;
    }

    if (goodSymDiff_empty) {
        test("Sym difference of equal sets", isEmpty, goodSymDiff_empty->getSize());
        delete goodSymDiff_empty;
    }

    test("Sym difference with nan tolerance", isBadCollection<ISet>, badSymDiff_nan);
    test("Sym difference of set and null", isBadCollection<ISet>, badSymDiff_nullptr_right);
    test("Sym difference of null and set", isBadCollection<ISet>, badSymDiff_nullptr_left);
}

static bool isTrue(bool expression) {
    return expression == true;
}

static bool testClone(ISet* s) {
    assert(s);

    ISet* setClone = s->clone();
    bool res = false;

    if (setClone) {
        res = checkSet(setClone, setData1);
        test("Clone of set", isTrue, res);
        delete setClone;
    }
    return res;
}

static void testInsert(ILogger* pLogger) {
    ISet* s = ISet::createSet(nullptr);

    if (s && isEmpty(s->getSize())) {
        IVector* vec = IVector::createVector(DIMENSION, coords_insert_1, pLogger);

        if (vec) {
            auto
                rc_nan = s->insert(vec, IVector::NORM::NORM_2, NAN),
                rc_nullptr = s->insert(nullptr, IVector::NORM::NORM_2, TOLERANCE),
                rc_good = s->insert(vec, IVector::NORM::NORM_2, TOLERANCE);

            test("Insert with nan tolerance", isTrue, rc_nan != RESULT_CODE::SUCCESS);
            test("Insert nullptr", isTrue, rc_nullptr != RESULT_CODE::SUCCESS);
            test("Insert good vector", isTrue, rc_good == RESULT_CODE::SUCCESS);

            IVector* vecOtherDim = IVector::createVector(DIMENSION + 1, coords_insert_2, pLogger);
            if (vecOtherDim) {
                auto rc_bad = s->insert(vecOtherDim, IVector::NORM::NORM_2, TOLERANCE);
                test("Insert bad vector", isTrue, rc_bad != RESULT_CODE::SUCCESS);

                delete vecOtherDim;
            }
        }

        delete s;
    }
}

static void testErase(ISet* s, ILogger* pLogger) {
    assert(s && s->getSize() > 2);

    ISet* setClone = s->clone();

    if (setClone) {
        auto
            rc_bad_index = s->erase(s->getSize()),
            rc_good_index = s->erase(s->getSize() - 1);

        test("Erase elem with bad index", isTrue, rc_bad_index != RESULT_CODE::SUCCESS);
        test("Erase elem with good index", isTrue, rc_good_index == RESULT_CODE::SUCCESS);

        IVector* vec = IVector::createVector(DIMENSION, coords_erase, pLogger);

        if (vec) {
            auto
                rc_nan = s->erase(vec, IVector::NORM::NORM_2, NAN),
                rc_nullptr = s->erase(nullptr, IVector::NORM::NORM_2, TOLERANCE),
                rc_good = s->erase(vec, IVector::NORM::NORM_2, TOLERANCE);

            test("Erase with nan tolerance", isTrue, rc_nan != RESULT_CODE::SUCCESS);
            test("Erase nullptr", isTrue, rc_nullptr != RESULT_CODE::SUCCESS);
            test("Erase good vector", isTrue, rc_good == RESULT_CODE::SUCCESS);

            IVector* vecOtherDim = IVector::createVector(DIMENSION + 1, coords_insert_2, pLogger);
            if (vecOtherDim) {
                auto rc_bad = s->insert(vecOtherDim, IVector::NORM::NORM_2, TOLERANCE);
                test("Insert bad vector", isTrue, rc_bad != RESULT_CODE::SUCCESS);

                delete vecOtherDim;
            }
        }
        delete setClone;
    }
}

int main() {
    ISet
            *s1 = createSet(setData1, 5, nullptr),
            *s2 = createSet(setData2, 7, nullptr);

    printHead(s1, s2);

    if (s1 && s2 && checkSet(s1, setData1) && checkSet(s2, setData2)) {
        testSum(s1, s2, pLogger);
        testIntersect(s1, s2, pLogger);
        testDiff(s1, s2, pLogger);
        testSymDiff(s1, s2, pLogger);
        testInsert(pLogger);

        if (testClone(s1)) {
            testErase(s1, pLogger);
        }

        std::cout << endl << (allPassed ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
    }

    if (s1) { delete s1; }
    if (s2) { delete s2; }

    pLogger->destroyLogger(CLIENT(CLIENT_KEY));

    return 0;
}

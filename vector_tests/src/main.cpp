#include <iostream>
#include <cmath>
#include <array>
#include <cassert>

#include "include/test.h"
#include "include/ILogger.h"
#include "include/IVector.h"

#define CLIENT(n) ((void*) n)
#define CLIENT_KEY 47
#define TOLERANCE 1e-6
#define DIMENSION 4

using namespace std;

double
        coords1[]   = {1., 2., 3., 4.},
        coords2[]   = {5., 6., 7., 8.},
        coords_nan[]   = {5., NAN, 7., 8.},
        scaleParam  = 5;

double
        etalonSum[]     = {6., 8., 10., 12.},
        etalonDiff[]    = {-4., -4., -4., -4.},
        etalonMul_VD[]  = {5., 10., 15., 20.},
        etalonMul_VV    = 70.,
        etalonNorm_1    = 10.,
        etalonNorm_2    = sqrt(30.),
        etalonNorm_inf  = 4.;

ILogger *pLogger = ILogger::createLogger(CLIENT(CLIENT_KEY));

static void print(FILE* stream, IVector* vec) {
    if (!stream) { stream = stdout; }

    if (!vec) {
        fprintf(stream, "nullptr\n");
    } else {
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
}

static void printHead(IVector* v1, IVector* v2) {
    cout << "Tests for vector library:" << endl;

    if (v1 && v2) {
        cout << "v1 = ";
        print(nullptr, v1);

        cout << "v2 = ";
        print(nullptr, v2);
    } else {
        cout << "v1 and v2 are bad" << endl;
    }
    cout << endl;
}

static bool checkVector(IVector* vec, double* arr) {
    if (!vec || !arr) { return false; }

    size_t dim = vec->getDim();
    for (size_t i = 0; i < dim; ++i) {
        if (std::abs(vec->getCoord(i) - arr[i]) > TOLERANCE) {
            return false;
        }
    }

    return true;
}

static bool checkNum(double res, double num) {
    return std::abs(res - num) > TOLERANCE ? false : true;
}

static bool badCoord(IVector* vec) {
    if (!vec) { return false; }

    return std::isnan(vec->getCoord(DIMENSION)) || std::isinf(vec->getCoord(DIMENSION));
}

static void testSum(IVector* v1, IVector* v2, IVector* vecOtherDim, ILogger* pLogger) {
    assert(v1 && v2 && vecOtherDim);

    auto
            goodSum = IVector::add(v1, v2, pLogger),
            badSum_otherDim = IVector::add(v1, vecOtherDim, static_cast<ILogger*>(nullptr)),
            badSum_nullptr_right = IVector::add(v1, nullptr, static_cast<ILogger*>(nullptr)),
            badSum_nullptr_left = IVector::add(nullptr, v2, static_cast<ILogger*>(nullptr));

    if (goodSum) {
        test("Sum of compatible vec", checkVector, goodSum, etalonSum);
        // goodSum->~IVector();
    }

    test("Sum of incompatible vec", isBad<IVector>, badSum_otherDim);
    test("Sum of vec and null", isBad<IVector>, badSum_nullptr_right);
    test("Sum of null and vec", isBad<IVector>, badSum_nullptr_left);
}

static void testDiff(IVector* v1, IVector* v2, IVector* vecOtherDim, ILogger* pLogger) {
    assert(v1 && v2 && vecOtherDim);

    auto
            goodDiff = IVector::sub(v1, v2, pLogger),
            badDiff_otherDim = IVector::sub(v1, vecOtherDim, nullptr),
            badDiff_nullptr_right = IVector::sub(v1, nullptr, nullptr),
            badDiff_nullptr_left = IVector::sub(nullptr, v2, nullptr);

    if (goodDiff) {
        test("Diff of compatible vec", checkVector, goodDiff, etalonDiff);
        // delete goodDiff;
    }

    test("Diff of incompatible vec", isBad<IVector>, badDiff_otherDim);
    test("Diff of vec and null", isBad<IVector>, badDiff_nullptr_right);
    test("Diff of null and vec", isBad<IVector>, badDiff_nullptr_left);
}

static void testMul(IVector* v, ILogger* pLogger) {
    assert(v && !isnan(scaleParam));

    auto
            goodMul = IVector::mul(v, scaleParam, pLogger),
            badMul_nan = IVector::mul(v, NAN, nullptr),
            badMul_nullptr = IVector::mul(nullptr, scaleParam, nullptr);

    if (goodMul) {
        test("Product of vector by number", checkVector, goodMul, etalonMul_VD);
        // delete goodMul;
    }

    test("Product of vector by nan", isBad<IVector>, badMul_nan);
    test("Product of null by number", isBad<IVector>, badMul_nullptr);
}

static void testMul(IVector* v1, IVector* v2, IVector* vecOtherDim, ILogger* pLogger) {
    assert(v1 && v2 && vecOtherDim);

    auto
            goodMul = IVector::mul(v1, v2, pLogger),
            badMul_otherDim = IVector::mul(v1, vecOtherDim, nullptr),
            badMul_nullptr_right = IVector::mul(v1, nullptr, nullptr),
            badMul_nullptr_left = IVector::mul(nullptr, v2, nullptr);


    test("Dot product of compatible vectors", checkNum, goodMul, etalonMul_VV);
    test("Dot product of incompatible vectors", std::isnan, badMul_otherDim);
    test("Dot product of vector and null", std::isnan, badMul_nullptr_right);
    test("Dot product of null and vector", std::isnan, badMul_nullptr_left);
}

static void testNorm(IVector* v1) {
    assert(v1);

    test("Norm 1", checkNum, v1->norm(IVector::NORM::NORM_1), etalonNorm_1);
    test("Norm 2", checkNum, v1->norm(IVector::NORM::NORM_2), etalonNorm_2);
    test("Norm inf", checkNum, v1->norm(IVector::NORM::NORM_INF), etalonNorm_inf);
}

static void testAccessData(IVector* v1) {
    assert(v1);

    test("Available coord", checkNum, v1->getCoord(0), coords1[0]);
    test("Unavailable coord", badCoord, v1);
}

static bool vecEquals(IVector* pOperand1, IVector* pOperand2) {
    bool res = true;
    auto eq = IVector::equals(pOperand1, pOperand2, IVector::NORM::NORM_INF, TOLERANCE, &res, pLogger);

    return (eq == RESULT_CODE::SUCCESS) && res;
}

static bool vecNotEquals(IVector* pOperand1, IVector* pOperand2) {
    bool res = true;
    auto eq = IVector::equals(pOperand1, pOperand2, IVector::NORM::NORM_INF, TOLERANCE, &res, nullptr);

    return (eq == RESULT_CODE::SUCCESS) && !res;
}

void testEquals(IVector* v1, IVector* v2, IVector* otherVec) {
    assert(v1 && v2 && otherVec);

    test("Equals vectors", vecEquals, v1, v2);
    test("Unequals vectors", vecNotEquals, v1, otherVec);
}

int main() {
    IVector
            *vec3dim = IVector::createVector(3, coords1, pLogger),
            *v1 = IVector::createVector(4, coords1, pLogger),
            *v2 = IVector::createVector(4, coords2, pLogger),
            *v_nan = IVector::createVector(4, coords_nan, nullptr);


    printHead(v1, v2);

    if (v1) {
        if (v2 && vec3dim) {
            testSum(v1, v2, vec3dim, pLogger);
            testDiff(v1, v2, vec3dim, pLogger);
            testMul(v1, v2, vec3dim, pLogger);
        }

        testMul(v1, pLogger);
        testNorm(v1);
        testAccessData(v1);

        auto v3 = v1->clone();
        if (v3) {
            test("Clone", checkVector, v3, coords1);
            testEquals(v1, v3, v2);

            delete v3;
        }
    }

    print(nullptr, v_nan);
    test("Creation with nan data", isBad<IVector>, v_nan);

    std::cout << endl << (allPassed ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;

    if (v1) { delete v1; }
    if (v2) { delete v2; }
    if (vec3dim) { delete vec3dim; }
    if (v_nan) { delete vec3dim; }

    pLogger->destroyLogger(CLIENT(CLIENT_KEY));

    return 0;
}

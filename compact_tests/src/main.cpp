#include <iostream>
#include <cmath>
#include <array>
#include <cassert>

#include "include/test.h"
#include "include/ILogger.h"
#include "include/IVector.h"
#include "include/ICompact.h"

#define CLIENT(n) ((void*) n)
#define CLIENT_KEY 47
#define DIM 3

using namespace std;


static const double tolerance = 1e-6;

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

static void print(FILE* stream, ICompact* compact) {
    if (!stream) { stream = stdout; }

    if (!compact) {
        fprintf(stream, "nullptr\n");
    } else {
        fprintf(stream, "begin:\n\t");
        print(stream, compact->getBegin());
        fprintf(stream, "end:\n\t");
        print(stream, compact->getEnd());
    }
}

static void printHead() {
    cout << "Tests for compact library:" << endl << endl;
}

static bool isTrue(bool expression) {
    return expression == true;
}

template<size_t dim1, size_t dim2>
static ICompact * createCompact(array<double, dim1> const* begAr, array<double, dim2> const* endAr, ILogger *logger) {
    if (!begAr || !endAr) { return nullptr; }

    IVector *beg = IVector::createVector(dim1, const_cast<double *>(begAr->data()), logger);
    IVector *end = IVector::createVector(dim2, const_cast<double *>(endAr->data()), logger);

    auto compact = ICompact::createCompact(beg, end, logger);
    delete beg;
    delete end;

    return compact;
}

template<size_t dim>
static bool checkCompact(ICompact const* c, array<double, dim> const& begAr,
                  array<double, dim> const& endAr, ILogger *logger) {
    if (c == nullptr) { return false; }

    bool eq;
    IVector
            *beg = IVector::createVector(dim, const_cast<double*>(begAr.data()), logger),
            *end = IVector::createVector(dim, const_cast<double*>(endAr.data()), logger);

    if (!beg || !end) {
        delete beg;
        delete end;
        return false;
    }

    auto gotBeg = c->getBegin();
    auto rc = IVector::equals(gotBeg, beg, IVector::NORM::NORM_2, tolerance, &eq, logger);
    delete gotBeg;

    if (rc != RESULT_CODE::SUCCESS || !eq) {
        delete beg;
        delete end;
        return false;
    }

    auto gotEnd = c->getEnd();
    rc = IVector::equals(gotEnd, end, IVector::NORM::NORM_2, tolerance, &eq, logger);
    delete gotEnd;

    if (rc != RESULT_CODE::SUCCESS || !eq) {
        delete beg;
        delete end;
        return false;
    }

    delete beg;
    delete end;
    return true;
}


array<double, DIM> const
        beginData_1 = {0, 0, 0},
        endData_1   = {1, 1, 1},

        beginData_2 = {0.25, 0.25, 0.25},
        endData_2   = {1, 1, 1},

        beginData_3 = {0.75, 0.75, 0.75},
        endData_3   = {2, 2, 2},

        beginData_4 = {-0.5, -0.35, 0.2},
        endData_4   = {0.3, 1.3, 0.5},

        beginData_5 = {5, 5, 5},
        endData_5   = {6, 6, 6},

        beginData_6 = {0.4, 0, 0},
        endData_6   = {1, 1, 1.4},

        beginData_7 = {0, 0.5, 0},
        endData_7   = {1, 1.5, 1},

        nanData     = {1, NAN, 3},
        otherData   = {3, -1, 2},

        vecData_1   = {0.5, 0.5, 0.5},
        vecData_2   = {1.5, 1.5, 1.5};

array<double, DIM> const
        unifyData_l_12  = {0, 0, 0},
        unifyData_r_12  = {1, 1, 1},

        unifyData_l_17  = {0, 0, 0},
        unifyData_r_17  = {1, 1.5, 1},

        convexData_l    = {0, 0, 0},
        convexData_r    = {2, 2, 2},

        intersData_l_12 = {0.25, 0.25, 0.25},
        intersData_r_12 = {1, 1, 1},

        intersData_l_13 = {0.75, 0.75, 0.75},
        intersData_r_13 = {1, 1, 1},

        intersData_l_14 = {0, 0, 0.2},
        intersData_r_14 = {0.3, 1, 0.5};

array<double, DIM - 1> const
        otherDimData    = {2, 3};

static void checkBadCreation(ILogger* logger) {
    auto badCompact = createCompact<DIM, DIM>(&endData_1, &beginData_1, logger);
    test("Create bad compact (begin > end)", isBad<ICompact>, badCompact);
    delete badCompact;

    badCompact = createCompact<DIM, DIM - 1>(&beginData_1, &otherDimData, logger);
    test("Create bad compact (dim mismatch)", isBad<ICompact>, badCompact);
    delete badCompact;

    badCompact = createCompact<DIM, DIM>(&beginData_1, &nanData, logger);
    test("Create bad compact (NAN)", isBad<ICompact>, badCompact);
    delete badCompact;

    badCompact = createCompact<DIM, DIM>(nullptr, &endData_1, logger);
    test("Create bad compact (begin is null)", isBad<ICompact>, badCompact);
    delete badCompact;

    badCompact = createCompact<DIM, DIM>(&beginData_1, nullptr, logger);
    test("Create bad compact (end is null)", isBad<ICompact>, badCompact);
    delete badCompact;

    badCompact = createCompact<DIM, DIM>(&beginData_1, &otherData, logger);
    test("Create bad compact (begin !<= end)", isBad<ICompact>, badCompact);
    delete badCompact;
}

static void testClone(ICompact* c, ILogger* logger) {
    assert(c);

    ICompact* compactClone = c->clone();
    test("Clone of set", isTrue, checkCompact<DIM>(compactClone, beginData_1, endData_1, logger));
    delete compactClone;
}

static void testIsContains(ICompact* c, ILogger* logger) {
    assert(c);

    bool contains;
    auto vec = IVector::createVector(DIM, const_cast<double*>(vecData_1.data()), logger);
    auto rc = c->isContains(vec, contains);
    if (rc == RESULT_CODE::SUCCESS) { test("Contains (yes)", isTrue, contains); }
    delete vec;

    vec = IVector::createVector(DIM, const_cast<double*>(vecData_2.data()), logger);
    rc = c->isContains(vec, contains);
    if (rc == RESULT_CODE::SUCCESS) { test("Contains (no)", isTrue, !contains); }
    delete vec;
}

static void testUnify(ICompact* c1, ICompact* c2, ICompact* c3, ILogger* logger) {
    assert(c1 && c2 && c3);

    auto
            unify       = ICompact::add(c1, c2, logger),
            unify_eq    = ICompact::add(c1, c1, logger),
            badUnify_not_unify      = ICompact::add(c1, c3, nullptr),
            badUnify_nullptr_right  = ICompact::add(nullptr, c2, nullptr),
            badUnify_nullptr_left   = ICompact::add(c1, nullptr, nullptr);

    test("Unifying of two compacts", isTrue, checkCompact<DIM>(unify, unifyData_l_12, unifyData_r_12, logger));
    delete unify;
    test("Unifying of equal compacts", isTrue, checkCompact<DIM>(unify_eq, beginData_1, endData_1, logger));
    delete unify_eq;

    test("Impossible unifying of compacts", isBad<ICompact>, badUnify_not_unify);
    delete badUnify_not_unify;
    test("Unifying of compact and null", isBad<ICompact>, badUnify_nullptr_right);
    delete badUnify_nullptr_right;
    test("Unifying of null and compact", isBad<ICompact>, badUnify_nullptr_left);
    delete badUnify_nullptr_left;

    auto c = createCompact<DIM, DIM>(&beginData_5, &endData_5, logger);
    if (checkCompact<DIM>(c, beginData_5, endData_5, logger)) {
        auto unify_15 = ICompact::add(c1, c, nullptr);
        test("Impossible unifying of compacts (not connected)", isBad<ICompact>, unify_15);
        delete unify_15;
    }
    delete c;

    c = createCompact<DIM, DIM>(&beginData_6, &endData_6, logger);
    if (checkCompact<DIM>(c, beginData_6, endData_6, logger)) {
        auto unify_16 = ICompact::add(c1, c, nullptr);
        test("Impossible unifying of compacts (axis parallel but diff)", isBad<ICompact>, unify_16);
        delete unify_16;
    }
    delete c;

    c = createCompact<DIM, DIM>(&beginData_7, &endData_7, logger);
    if (checkCompact<DIM>(c, beginData_7, endData_7, logger)) {
        auto unify_17 = ICompact::add(c1, c, logger);
        test("Unifying of compacts (axis parallel but not diff)", isTrue, checkCompact<DIM>(unify_17, unifyData_l_17, unifyData_r_17, logger));
        delete unify_17;
    }
    delete c;
}

static void testIntersect(ICompact* c1, ICompact* c2, ICompact* c3, ILogger* logger) {
    assert(c1 && c2 && c3);

    auto
            inters_12 = ICompact::intersection(c1, c2, logger),
            inters_13 = ICompact::intersection(c1, c3, logger),
            inters_eq = ICompact::intersection(c1, c1, logger),
            badInters_nullptr_right = ICompact::intersection(nullptr, c2, nullptr),
            badInters_nullptr_left = ICompact::intersection(c1, nullptr, nullptr);

    test("Intersection of two compacts (1 & 2)", isTrue, checkCompact<DIM>(inters_12, intersData_l_12, intersData_r_12, logger));
    delete inters_12;
    test("Intersection of two compacts (1 & 3)", isTrue, checkCompact<DIM>(inters_13, intersData_l_13, intersData_r_13, logger));
    delete inters_13;
    test("Intersection of equal compacts", isTrue, checkCompact<DIM>(inters_eq, beginData_1, endData_1, logger));
    delete inters_eq;

    test("Intersection of compact and null", isBad<ICompact>, badInters_nullptr_right);
    delete badInters_nullptr_right;
    test("Intersection of null and compact", isBad<ICompact>, badInters_nullptr_left);
    delete badInters_nullptr_left;

    auto c = createCompact<DIM, DIM>(&beginData_4, &endData_4, logger);
    if (checkCompact<DIM>(c, beginData_4, endData_4, logger)) {
        auto inters_14 = ICompact::intersection(c1, c, logger);
        test("Intersection of two compacts (1 & 4)", isTrue, checkCompact<DIM>(inters_14, intersData_l_14, intersData_r_14, logger));
        delete inters_14;
    }
    delete c;

    c = createCompact<DIM, DIM>(&beginData_5, &endData_5, logger);
    if (checkCompact<DIM>(c, beginData_5, endData_5, logger)) {
        auto inters_no = ICompact::intersection(c1, c, nullptr);
        test("Impossible intersection of two compacts", isBad<ICompact>, inters_no);
        delete inters_no;
    }
    delete c;
}

static void testConvex(ICompact* c1, ICompact* c2, ILogger* logger) {
    assert(c1 && c2);

    auto convh = ICompact::makeConvex(c1, c2, logger);
    test("Convex hull of compacts", isTrue, checkCompact<DIM>(convh, convexData_l, convexData_r, logger));
    delete convh;
}

int main() {
    ILogger *logger = ILogger::createLogger(CLIENT(CLIENT_KEY));

    printHead();
    auto compact1 = createCompact<DIM, DIM>(&beginData_1, &endData_1, logger);
    if (checkCompact<DIM>(compact1, beginData_1, endData_1, logger)) {
        test("Check dimension", isTrue, compact1->getDim() == DIM);
        checkBadCreation(nullptr);
        testClone(compact1, logger);
        testIsContains(compact1, logger);

        auto compact2 = createCompact<DIM, DIM>(&beginData_2, &endData_2, logger);
        if (checkCompact<DIM>(compact2, beginData_2, endData_2, logger)) {

            auto compact3 = createCompact<DIM, DIM>(&beginData_3, &endData_3, logger);
            if (checkCompact<DIM>(compact3, beginData_3, endData_3, logger)) {
                testUnify(compact1, compact2, compact3, logger);
                testIntersect(compact1, compact2, compact3, logger);
                testConvex(compact1, compact3, logger);
            }
            delete compact3;
        }
        delete compact2;
    }
    delete compact1;

    std::cout << endl << (allPassed ? "ALL TESTS PASSED" : "TESTS FAILED") << endl;
    logger->destroyLogger(CLIENT(CLIENT_KEY));

    return 0;
}

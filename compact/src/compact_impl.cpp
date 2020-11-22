#include <new>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "include/IVector.h"
#include "include/ICompact.h"

namespace {
    static bool isLess(IVector const* l, IVector const* r) {
        size_t dim = l->getDim();

        if (dim != r->getDim()) { return false; }

        for (size_t i = 0; i < dim; ++i) {
            if (l->getCoord(i) > r->getCoord(i)) {
                return false;
            }
        }
        return true;
    }

    static IVector const* min(IVector const *l, IVector const *r) {
        return isLess(l, r) ? l : r;
    }

    static IVector const* max(IVector const *l, IVector const *r) {
        return isLess(l, r) ? r : l;
    }

    template<class T>
    static bool isValidData(T const* const begin, T const* const end) {
        if (begin == nullptr || end == nullptr) { return false; }
        if (begin->getDim() != end->getDim()) { return false; }
        return true;
    }

    class CompactImpl: public ICompact {
    private:
        // begin - left, end - right
        IVector *left, *right;
        size_t dim;
        ILogger *logger;

        CompactImpl(CompactImpl const& set) = delete;
        CompactImpl& operator=(CompactImpl const& set) = delete;

        bool isCorrectStep(IVector const* const step, bool reverse) {
            if (step == nullptr) { return false; }

            if (step->getDim() != dim) { return false; }

            for (size_t i = 0; i < dim; i++) {
                if (std::isnan(step->getCoord(i))) { return false; }

                auto coord = step->getCoord(i);
                if ((coord < 0 && !reverse)
                        || (coord > 0 && reverse)
                        || std::abs(coord) < tolerance) {
                    return false;
                }
            }
            return true;
        }

    public:
        constexpr static const double tolerance = 1e-6;

        CompactImpl(IVector *left, IVector *right, ILogger *logger):
            left(left->clone()), right(right->clone()),
            dim(left->getDim()), logger(logger) {}

        IVector* getBegin() const override { return left->clone(); }

        IVector* getEnd() const override { return right->clone(); }

        iterator* begin(IVector const* const step = nullptr) override {
            if (!isCorrectStep(step, false)) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::begin: incorrect step", RESULT_CODE::WRONG_ARGUMENT);
                }
                return nullptr;
            }

            auto it = new (std::nothrow) iterator(this, step, logger);
            if (it == nullptr) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::begin: no memory", RESULT_CODE::OUT_OF_MEMORY);
                }
            }
            return it;
        }

        iterator* end(IVector const* const step = nullptr) override {
            if (!isCorrectStep(step, true)) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::begin: incorrect step", RESULT_CODE::WRONG_ARGUMENT);
                }
                return nullptr;
            }

            auto it = new (std::nothrow) iterator(this, step, logger, true);
            if (it == nullptr) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::end: no memory", RESULT_CODE::OUT_OF_MEMORY);
                }
            }
            return it;
        }

        RESULT_CODE isContains(IVector const* const vec, bool& result) const override {
            if (vec == nullptr) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::isContains: null param", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            if (vec->getDim() != dim) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::isContains: dimension mismatch", RESULT_CODE::WRONG_DIM);
                }
                return RESULT_CODE::WRONG_DIM;
            }

            result = isLess(left, vec) && isLess(vec, right);
            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE isSubSet(ICompact const* const other, bool& result) const override {
            if (!isValidData<ICompact>(this, other)) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::isSubSet: inconsistent <other> param", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            bool is_contains;
            auto rc = isContains(other->getBegin(), is_contains);
            if (rc != RESULT_CODE::SUCCESS) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::isSubSet: bad this->getBegin()", RESULT_CODE::BAD_REFERENCE);
                }
                return rc;
            }

            if (is_contains) {
                rc = isContains(other->getEnd(), is_contains);
                if (rc != RESULT_CODE::SUCCESS) {
                    if (logger != nullptr) {
                        logger->log("in CompactImpl::isSubset: bad this->getEnd()", RESULT_CODE::BAD_REFERENCE);
                    }
                    return rc;
                }
                result = is_contains;
            }

            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE isIntersects(ICompact const* const other, bool& result) const override {
            if (!isValidData<ICompact>(this, other)) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::isIntersects: null param or dimension mismatch", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            auto
                    l = max(left, other->getBegin()),
                    r = min(right, other->getEnd());

            result = isLess(l, r);
            return RESULT_CODE::SUCCESS;
        }

        size_t getDim() const override { return dim; }

        ICompact* clone() const override {
            auto c = new (std::nothrow) CompactImpl(left, right, logger);
            if (c == nullptr) {
                if (logger != nullptr) {
                    logger->log("in CompactImpl::clone", RESULT_CODE::OUT_OF_MEMORY);
                }
            }
            return c;
        }

        ~CompactImpl() override {
            delete left;
            delete right;
        }

        class iterator : public ICompact::iterator {
            friend class CompactImpl;
        private:
            ILogger *logger;

            bool reverse;
            ICompact const *compact;
            IVector *current;
            IVector *dir;
            IVector const *step;

            iterator(const iterator& other) = delete;
            void operator=( const iterator& other) = delete;

            static bool checkUnique(IVector const* const dir, size_t dim, size_t idx) {
                auto coord = dir->getCoord(idx);
                if (coord < 0 || coord > dim - 1) { return false; }

                for (size_t i = 0; i < dim; i++) {
                    if (i == idx) { continue; }
                    if (std::abs(dir->getCoord(i) - coord) < tolerance) {
                        return false;
                    }
                }
                return true;
            }
        public:
            iterator(ICompact const *compact, IVector const *step, ILogger *logger, bool reverse = false):
                    logger(logger), reverse(reverse), compact(compact->clone()),
                    current(!reverse ? compact->getBegin()->clone() : compact->getEnd()->clone()) {
                const double stp = tolerance * 10;
                auto dim = compact->getDim();
                double *data = new double[dim];

                if (step == nullptr) {
                    for (size_t i = 0; i < dim; i++) {
                        data[i] = stp;
                    }
                }

                this->step = step->clone();

                for (size_t i = 0; i < dim; i++) { data[i] = i; }
                dir = IVector::createVector(compact->getDim(), data, logger);
                delete[] data;
            }

            // adds step to current value in iterator
            RESULT_CODE doStep() override {
                bool done = false;

                auto
                        begin = compact->getBegin(),
                        end = compact->getEnd();

                bool eq;
                auto rc = !reverse ? IVector::equals(end, current, IVector::NORM::NORM_2, tolerance, &eq, logger) :
                                     IVector::equals(begin, current, IVector::NORM::NORM_2, tolerance, &eq, logger);

                if (eq || rc != RESULT_CODE::SUCCESS) {
                    delete begin;
                    delete end;
                    return RESULT_CODE::OUT_OF_BOUNDS;
                }

                IVector *v = current->clone();

                auto dim = compact->getDim();

                for (size_t i = 0, idx = 0; i < dim && !done; i++) {
                    idx = static_cast<size_t>(std::round(dir->getCoord(i)));

                    if (!reverse) {
                        if (std::abs(v->getCoord(idx) - end->getCoord(idx)) < tolerance) {
                            v->setCoord(idx, begin->getCoord(idx));
                            continue;
                        }
                    } else {
                        if (std::abs(v->getCoord(idx) - begin->getCoord(idx)) < tolerance) {
                            v->setCoord(idx, end->getCoord(idx));
                            continue;
                        }
                    }

                    v->setCoord(idx, v->getCoord(idx) + step->getCoord(idx));

                    bool contains;
                    auto rc = compact->isContains(v, contains);

                    if (rc != RESULT_CODE::SUCCESS) {
                        if (logger != nullptr) {
                            logger->log("in CompactImpl::iterator::doStep: bad current or step vector", rc);
                        }
                        delete v;
                        return rc;
                    }

                    if (!contains) {
                        !reverse ? v->setCoord(idx, end->getCoord(idx)) : v->setCoord(idx, begin->getCoord(idx));
                    }

                    done = true;
                }

                if (done) {
                    for (size_t i = 0; i < dim; i++) {
                        current->setCoord(i, v->getCoord(i));
                    }
                }
                delete v;

                return RESULT_CODE::SUCCESS;
            }

            IVector* getPoint() const override { return current->clone(); }

            // change order of step
            RESULT_CODE setDirection(IVector const* const dir) override {
                if (dir->getDim() != compact->getDim()) {
                    if (logger != nullptr) {
                        logger->log("in CompactImpl::iterator::setDirection: dimension mismatch", RESULT_CODE::WRONG_DIM);
                    }
                    return RESULT_CODE::WRONG_DIM;
                }

                auto dim = dir ->getDim();

                for (size_t i = 0; i < dim; i++) {
                    if (!checkUnique(dir, dim, i)) {
                        if (logger != nullptr) {
                            logger->log("in CompactImpl::iterator::setDirection: direction with repeated coordinates", RESULT_CODE::WRONG_ARGUMENT);
                        }
                        return RESULT_CODE::WRONG_ARGUMENT;
                    }
                }

                dim = compact->getDim();
                for (size_t i = 0; i < dim; i++) {
                    auto coord = dir->getCoord(i);
                    if (std::abs(coord - std::round(coord)) > tolerance) {
                        if (logger != nullptr) {
                            logger->log("in CompactImpl::iteratorsetDirection: direction is integer vector mention order to pass compact", RESULT_CODE::WRONG_ARGUMENT);
                        }
                        return RESULT_CODE::WRONG_ARGUMENT;
                    }
                    this->dir->setCoord(i, dir->getCoord(i));
                }

                delete current;
                current = !reverse ? compact->getBegin() : compact->getEnd();

                return RESULT_CODE::SUCCESS;
            }

            ~iterator() override {
                delete dir;
                delete step;
                delete current;
                delete compact;
            }
        };
    };
}

ICompact::~ICompact() {}

ICompact* ICompact::createCompact(IVector const* const begin, IVector const* const end, ILogger* logger) {
    if (!isValidData<IVector>(begin, end)) {
        if (logger != nullptr) {
            logger->log("in ICompact::createCompact: null param or vector dimension mismatch", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    if (!isLess(begin, end)) {
        if (logger != nullptr) {
            logger->log("in ICompact::createCompact: bounds are not comparable", RESULT_CODE::WRONG_ARGUMENT);
        }
        return nullptr;
    }

    // begin < end!
    return new (std::nothrow) CompactImpl(const_cast<IVector *>(begin), const_cast<IVector *>(end), logger);
}

ICompact * ICompact::intersection(ICompact const* const left, ICompact const* const right, ILogger* logger) {
    if (!isValidData(left, right)) {
        if (logger != nullptr) {
            logger->log("in ICompact::intersection: null param or dimension mismatch", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    auto dim = left->getDim();

    bool inters;
    auto rc = left->isIntersects(right, inters);

    if (rc != RESULT_CODE::SUCCESS) {
        if (logger != nullptr) {
            logger->log("in ICompact::intersection", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    if (!inters) {
        if (logger != nullptr) {
            logger->log("in ICompact::intersecton: cannot intersect", RESULT_CODE::WRONG_ARGUMENT);
        }
        return nullptr;
    }

    auto *data = new (std::nothrow) double[dim]{0};
    if (data == nullptr) {
        if (logger != nullptr) {
            logger->log("in ICompact::intersection", RESULT_CODE::OUT_OF_MEMORY);
        }
        return nullptr;
    }

    auto
            l = IVector::createVector(dim, data, logger),
            r = IVector::createVector(dim, data, logger);

    delete[] data;

    auto
            lbeg = left->getBegin(),
            rbeg = right->getBegin(),
            lend = left->getEnd(),
            rend = right->getEnd();

    for (size_t i = 0; i < dim; i++) {
        l->setCoord(i, std::max(lbeg->getCoord(i), rbeg->getCoord(i)));
        r->setCoord(i, std::min(lend->getCoord(i), rend->getCoord(i)));
    }

    delete lbeg;
    delete lend;
    delete rbeg;
    delete rend;

    auto c = createCompact(l, r, logger);

    delete l;
    delete r;

    return c;
}

static bool compactIsInCompact(IVector const* const beg1, IVector const* const end1,
                               IVector const* const beg2, IVector const* const end2) {
    return isLess(beg1, beg2) && isLess(end2, end1);
};

static bool checkParallel(IVector const* const v , int &axisNo) {
    int nonZeroCount = 0;
    double norm = v->norm(IVector::NORM::NORM_INF);
    auto dim = v->getDim();

    for (size_t i = 0; i < dim && nonZeroCount < 2; ++i) {
        if (std::abs(v->getCoord(i) / norm) > CompactImpl::tolerance) {
            axisNo = i;
            nonZeroCount++;
        }
    }

    if (nonZeroCount > 1) { return false; }

    return true;
};

//union
ICompact* ICompact::add(ICompact const* const left, ICompact const* const right, ILogger*logger) {
    if (!isValidData(left, right)) {
        if (logger != nullptr) {
            logger->log("in ICompact::add: null param or dimension mismatch", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    auto
            lbeg = left->getBegin(),
            rbeg = right->getBegin(),
            lend = left->getEnd(),
            rend = right->getEnd();

    if (isLess(lend, rbeg) || isLess(rend, lbeg)) {
        delete lbeg;
        delete rbeg;
        delete lend;
        delete rend;

        if (logger != nullptr) {
            logger->log("in ICompact::add: cannot add", RESULT_CODE::WRONG_ARGUMENT);
        }
        return nullptr;
    }

    // right in left
    if (compactIsInCompact(lbeg, lend, rbeg, rend)) {
        delete lbeg;
        delete rbeg;
        delete lend;
        delete rend;

        return left->clone();
    }

    // left in right
    if (compactIsInCompact(rbeg, rend, lbeg, lend)) {
        delete lbeg;
        delete rbeg;
        delete lend;
        delete rend;

        return right->clone();
    }

    auto dbegin = IVector::sub(lbeg, rbeg, logger);
    if (dbegin == nullptr) {
        if (logger != nullptr) {
            logger->log("in ICompact::add: nonconsistent begin", RESULT_CODE::WRONG_ARGUMENT);
        }

        delete lbeg;
        delete rbeg;
        delete lend;
        delete rend;

        return nullptr;
    }

    int axisNoBeg, axisNoEnd;
    if (checkParallel(dbegin, axisNoBeg)) {
        auto dend = IVector::sub(lend, rend, logger);

        if (dend == nullptr) {
            delete dbegin;

            delete lbeg;
            delete rbeg;
            delete lend;
            delete rend;

            if (logger != nullptr) {
                logger->log("in ICompact::add: nonconsistent end", RESULT_CODE::WRONG_ARGUMENT);
            }

            return nullptr;
        }

        if (checkParallel(dend, axisNoEnd)) {
            if (axisNoBeg == axisNoEnd) {
                delete dbegin;
                delete dend;

                auto c = createCompact(min(lbeg, rbeg), max(lend, rend), logger);

                delete lbeg;
                delete rbeg;
                delete lend;
                delete rend;

                return c;
            }
        }
        delete dend;
    }
    delete dbegin;

    delete lbeg;
    delete rbeg;
    delete lend;
    delete rend;

    if (logger != nullptr) {
        logger->log("in ICompact::add: cannot create convex union. Try makeConvex instead", RESULT_CODE::WRONG_ARGUMENT);
    }
    return nullptr;
}

ICompact * ICompact::makeConvex(ICompact const* const left, ICompact const* const right, ILogger*logger) {
    if (!isValidData(left, right)) {
        if (logger != nullptr) {
            logger->log("in ICompact::makeConvex: null param or dimension mismatch", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    auto
            lbeg = left->getBegin(),
            rbeg = right->getBegin(),
            lend = left->getEnd(),
            rend = right->getEnd();

    auto c = createCompact(min(lbeg, rbeg), max(lend, rend), logger);

    delete lbeg;
    delete lend;
    delete rbeg;
    delete rend;

    return c;
}

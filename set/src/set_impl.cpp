#include <algorithm>
#include <memory>
#include <new>
#include <cmath>
#include <vector>

#include "include/ISet.h"

namespace {
    class SetImpl: public ISet {
    private:
        std::vector<IVector const*> elements;
        ILogger* pLogger;

        SetImpl(SetImpl const& set) = delete;
        SetImpl& operator=(SetImpl const& set) = delete;

    public:
        SetImpl(ILogger* pLogger) : pLogger(pLogger) {}

        ~SetImpl() override { clear(); }

        RESULT_CODE insert(const IVector* pVector,IVector::NORM norm, double tolerance) override {
            if (std::isnan(tolerance) || tolerance < 0) {
                if (pLogger != nullptr) {
                    pLogger->log("in SetImpl::insert: NAN tolerance", RESULT_CODE::NAN_VALUE);
                }
                return RESULT_CODE::NAN_VALUE;
            }

            if (pVector == nullptr) {
                if (pLogger != nullptr)
                    pLogger->log("in SetImpl::insert: null param", RESULT_CODE::BAD_REFERENCE);
                return RESULT_CODE::BAD_REFERENCE;
            }

            if (!elements.empty()) {
                if (pVector->getDim() != getDim()) {
                    if (pLogger != nullptr) {
                        pLogger->log("in SetImpl::insert", RESULT_CODE::WRONG_DIM);
                    }
                    return RESULT_CODE::WRONG_DIM;
                }
            }

            IVector *diff;
            for (auto elem: elements) {
                diff = IVector::sub(pVector, elem, pLogger);

                if (diff == nullptr) {
                    if (pLogger != nullptr) {
                        pLogger->log("in SetImpl::insert", RESULT_CODE::BAD_REFERENCE);
                    }
                    return RESULT_CODE::BAD_REFERENCE;
                }

                if (diff->norm(norm) < tolerance) {
                    delete diff;

                    if (pLogger != nullptr) {
                        pLogger->log("in SetImpl::insert", RESULT_CODE::MULTIPLE_DEFINITION);
                    }
                    return RESULT_CODE::MULTIPLE_DEFINITION;
                }
                delete diff;
            }

            auto cloneElem = pVector->clone();
            if (cloneElem == nullptr) {
                if (pLogger != nullptr) {
                    pLogger->log("in SetImpl::get: nullptr", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            elements.push_back(cloneElem);
            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE get(IVector*& pVector, size_t index) const override {
            if (index >= elements.size()) {
                if (pLogger != nullptr) {
                    pLogger->log("in SetImpl::get", RESULT_CODE::OUT_OF_BOUNDS);
                }
                return RESULT_CODE::OUT_OF_BOUNDS;
            }

            auto cloneElem = elements[index]->clone();
            if (cloneElem == nullptr) {
                if (pLogger != nullptr) {
                    pLogger->log("in SetImpl::get: nullptr", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            pVector = cloneElem;
            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE get(IVector*& pVector, IVector const* pSample, IVector::NORM norm, double tolerance) const override {
            if (std::isnan(tolerance) || tolerance < 0) {
                if (pLogger != nullptr) {
                    pLogger->log("in SetImpl::get: NAN tolerance", RESULT_CODE::NAN_VALUE);
                }
                return RESULT_CODE::NAN_VALUE;
            }

            if (pSample == nullptr) {
                if (pLogger != nullptr) {
                    pLogger->log("in SetImpl::get: null param", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            if (pSample->getDim() != getDim()) {
                if (pLogger != nullptr) {
                    pLogger->log("in SetImpl::get", RESULT_CODE::WRONG_DIM);
                }
                return RESULT_CODE::WRONG_DIM;
            }

            IVector *diff = nullptr;

            for (auto elem: elements) {
                diff = IVector::sub(pSample, elem, pLogger);
                if (diff == nullptr) {
                    if (pLogger != nullptr) {
                        pLogger->log("in SetImpl::get: nullptr", RESULT_CODE::BAD_REFERENCE);
                    }
                    return RESULT_CODE::BAD_REFERENCE;
                }

                if (diff->norm(norm) < tolerance) {
                    delete diff;

                    auto cloneElem = elem->clone();
                    if (cloneElem == nullptr) {
                        if (pLogger != nullptr) {
                            pLogger->log("in SetImpl::get: nullptr", RESULT_CODE::BAD_REFERENCE);
                        }
                        return RESULT_CODE::BAD_REFERENCE;
                    }

                    pVector = cloneElem;
                    return RESULT_CODE::SUCCESS;
                }
                delete diff;
            }

            if (pLogger != nullptr) {
                pLogger->log("in SetImpl::get", RESULT_CODE::NOT_FOUND);
            }
            return RESULT_CODE::NOT_FOUND;
        }

        //space dimension
        size_t getDim() const override {
            if (elements.empty()) { return 0; }
            return elements[0]->getDim();
        }

        //num elements in set
        size_t getSize() const override { return elements.size(); }

        void clear() override {
            for (auto &elem: elements) {
                delete elem;
            }
            elements.clear();
        }

        RESULT_CODE erase(size_t index) override {
            if (index >= elements.size()) {
                if (pLogger != nullptr) {
                    pLogger->log("ISet::erase", RESULT_CODE::OUT_OF_BOUNDS);
                }
                return RESULT_CODE::OUT_OF_BOUNDS;
            }

            auto elem = elements.begin() + index;
            if (*elem != nullptr) { delete *elem; }
            elements.erase(elem);
            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE erase(IVector const* pSample, IVector::NORM norm, double tolerance) override {
            if (std::isnan(tolerance) || tolerance < 0) {
                if (pLogger != nullptr) {
                    pLogger->log("In ISet::erase: nullptr", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            IVector *diff = nullptr;
            size_t i = 0;
            for (auto elem: elements) {
                diff = IVector::sub(pSample, elem, pLogger);
                if (diff == nullptr) { break; }
                if (diff->norm(norm) < tolerance) {
                    delete diff;
                    delete *(elements.begin() + i);
                    elements.erase(elements.begin() + i);
                    return RESULT_CODE::SUCCESS;
                }
                delete diff;
                i++;
            }
            if (pLogger != nullptr) {
                pLogger->log("In Set::erase", RESULT_CODE::NOT_FOUND);
            }
            return RESULT_CODE::NOT_FOUND;
        }

        ISet* clone() const override {
            SetImpl *set = new (std::nothrow) SetImpl(pLogger);
            if (set == nullptr) {
                if (pLogger != nullptr) {
                    pLogger->log("in ISet::clone: out of memory", RESULT_CODE::OUT_OF_MEMORY);
                }
                return nullptr;
            }

            auto n = elements.size();
            set->elements.resize(n);

            for (size_t i = 0; i < n; i++) {
                auto cloneElem = elements[i]->clone();

                if (cloneElem == nullptr) {
                    if (pLogger != nullptr) {
                        pLogger->log("in ISet::clone: out of memory", RESULT_CODE::OUT_OF_MEMORY);
                    }

                    set->elements.resize(i);
                    delete set;
                    return nullptr;
                }
                set->elements[i] = cloneElem;
            }
            return set;
        }
    };
}

ISet::~ISet() {}

ISet* ISet::createSet(ILogger* pLogger) {
    ISet* set = new (std::nothrow) SetImpl(pLogger);
    if (set == nullptr) {
        pLogger->log("in ISet::createSet", RESULT_CODE::OUT_OF_MEMORY);
    }
    return set;
}

ISet* ISet::add(ISet const* pOperand1, ISet const* pOperand2, IVector::NORM norm, double tolerance, ILogger* pLogger) {
    if (std::isnan(tolerance) || tolerance < 0) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::add: NAN tolerance", RESULT_CODE::NAN_VALUE);
        }
        return nullptr;
    }

    if (pOperand1 == nullptr || pOperand2 == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::add: operand null", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    if (pOperand1->getDim() != pOperand2->getDim()) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::add: dim mismatch", RESULT_CODE::WRONG_DIM);
        }
        return nullptr;
    }

    ISet *sum = pOperand1->clone();

    if (sum != nullptr) {
        IVector *elem2;

        for (size_t i = 0; i < pOperand2->getSize(); i++) {
            if (pOperand2->get(elem2, i) != RESULT_CODE::SUCCESS) {
                delete sum;
                sum = nullptr;
                break;
            }

            sum->insert(elem2, norm, tolerance);
            delete elem2;
        }
    }

    return sum;
}

ISet* ISet::intersect(ISet const* pOperand1, ISet const* pOperand2, IVector::NORM norm, double tolerance, ILogger* pLogger) {
    if (std::isnan(tolerance) || tolerance < 0) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::intersect: NAN tolerance", RESULT_CODE::NAN_VALUE);
        }
        return nullptr;
    }

    if (pOperand1 == nullptr || pOperand2 == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::intersect: null operand", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    if (pOperand1->getDim() != pOperand2->getDim()) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::intersect: dim mismatch", RESULT_CODE::WRONG_DIM);
        }
        return nullptr;
    }


    ISet *intersection = ISet::createSet(pLogger);

    if (intersection == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::intersect", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    IVector *elem, *elemFound;
    for (size_t i = 0; i < pOperand2->getSize(); i++) {
        if (pOperand2->get(elem, i) != RESULT_CODE::SUCCESS) {
            delete intersection;
            intersection = nullptr;
            break;
        }

        auto rc = pOperand1->get(elemFound, elem, norm, tolerance);
        if (rc == RESULT_CODE::SUCCESS) {
            intersection->insert(elem, norm, tolerance);
            delete elemFound;
        } else if (rc != RESULT_CODE::NOT_FOUND) {
            delete elem;
            delete intersection;
            intersection = nullptr;
            break;
        }
        delete elem;
    }

    return intersection;
}

ISet* ISet::sub(ISet const* pOperand1, ISet const* pOperand2, IVector::NORM norm, double tolerance, ILogger* pLogger) {
    if (std::isnan(tolerance) || tolerance < 0) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::sub: NAN tolerance", RESULT_CODE::NAN_VALUE);
        }
        return nullptr;
    }

    if (pOperand1 == nullptr || pOperand2 == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::sub: null operand", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    ISet *diff = ISet::createSet(pLogger);

    if (diff == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::sub", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    IVector *elem, *elemFound;
    for (size_t i = 0; i < pOperand1->getSize(); i++) {
        pOperand1->get(elem, i);
        if (pOperand1->get(elem, i) != RESULT_CODE::SUCCESS) {
            delete diff;
            diff = nullptr;
            break;
        }

        auto rc = pOperand2->get(elemFound, elem, norm, tolerance);
        if (rc == RESULT_CODE::NOT_FOUND) {
            diff->insert(elem, norm, tolerance);
        } else if (rc != RESULT_CODE::SUCCESS) {
            delete elem;
            delete diff;
            diff = nullptr;
            break;
        } else {
            delete elemFound;
        }
        delete elem;
    }

    return diff;
}

ISet* ISet::symSub(ISet const* pOperand1, ISet const* pOperand2, IVector::NORM norm, double tolerance, ILogger* pLogger) {
    if (std::isnan(tolerance) || tolerance < 0) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::symSub: NAN tolerance", RESULT_CODE::NAN_VALUE);
        }
        return nullptr;
    }

    ISet *unified = ISet::add(pOperand1, pOperand2, norm, tolerance, pLogger);
    if (unified == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::symSub", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    ISet *intersection = ISet::intersect(pOperand1, pOperand2, norm, tolerance, pLogger);
    if (intersection == nullptr) {
        delete unified;
        if (pLogger != nullptr) {
            pLogger->log("in ISet::symSub", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    auto symsub = ISet::sub(unified, intersection, norm, tolerance, pLogger);

    delete unified;
    delete intersection;

    if (symsub == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in ISet::symSub", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }
    return symsub;
}

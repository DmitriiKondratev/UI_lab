#include <memory>
#include <new>
#include <cmath>
#include <limits>

#include "include/IVector.h"

namespace {
    class VectorImpl: public IVector {
    private:
        size_t dim;
        double* pData;
        ILogger* pLogger;

        VectorImpl(VectorImpl const& vector) = delete;
        VectorImpl& operator=(VectorImpl const& vector) = delete;

    public:
        // pData should be guaranteed alive array for all runtime
        VectorImpl(size_t dim, double* pData, ILogger* pLogger) :
            dim(dim), pData(pData), pLogger(pLogger) {}

        ~VectorImpl() override {
            auto ptr = reinterpret_cast<char*>(pData) - sizeof(VectorImpl);
            operator delete[] (pData, ptr);
        }

        IVector* clone() const override {
            return IVector::createVector(dim, pData, pLogger);
        }

        double getCoord(size_t index) const override {
            if (index >= dim) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            return pData[index];
        }

        RESULT_CODE setCoord(size_t index, double value) override {
            if (index >= dim) {
                if (pLogger != nullptr) {
                    pLogger->log("in VectorImpl::setCoord: wrong index", RESULT_CODE::WRONG_DIM);
                }
                return RESULT_CODE::WRONG_DIM;
            }

            if (std::isnan(value)) {
                if (pLogger != nullptr) {
                    pLogger->log("in VectorImpl::setCoord: value is not a number", RESULT_CODE::NAN_VALUE);
                }
                return RESULT_CODE::NAN_VALUE;
            }

            pData[index] = value;
            return RESULT_CODE::SUCCESS;
        }

        double norm(NORM norm) const override {
            double vecNorm = 0;

            switch (norm) {
            case NORM::NORM_1:
                for (size_t i = 0; i < dim; i++) {
                    vecNorm += std::abs(pData[i]);
                }
                break;

            case NORM::NORM_2:
                for (size_t i = 0; i < dim; i++) {
                    vecNorm += pData[i] * pData[i];
                }
                vecNorm = sqrt(vecNorm);
                break;

            case NORM::NORM_INF:
                for (size_t i = 0; i < dim; i++) {
                    if (std::abs(pData[i]) > vecNorm) {
                        vecNorm = std::abs(pData[i]);
                    }
                }
                break;
            }
            return vecNorm;
        }

        size_t getDim() const override { return dim; }
    };
}

IVector::~IVector() {}

IVector* IVector::createVector(size_t dim, double* pData, ILogger* pLogger) {
    if (dim == 0) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::createVector: 0 dimension", RESULT_CODE::WRONG_DIM);
        }
        return nullptr;
    }

    if (pData == nullptr) {
       if (pLogger != nullptr) {
           pLogger->log("in IVector::createVector: null param", RESULT_CODE::BAD_REFERENCE);
       }
       return nullptr;
    }

    for (size_t i = 0; i < dim; ++i) {
        if (std::isnan(pData[i])) {
            if (pLogger != nullptr) {
                pLogger->log("in IVector::createVector: nan in data", RESULT_CODE::NAN_VALUE);
            }
            return nullptr;
        }
    }

    // placement new
    size_t shift = sizeof(VectorImpl);
    size_t size = shift + sizeof(double) * dim;
    char* buff = new (std::nothrow) char[size];

    if (buff == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::createVector: could not create buffer", RESULT_CODE::OUT_OF_MEMORY);
        }
        return nullptr;
    }

    auto vec = new (buff) VectorImpl(dim, reinterpret_cast<double*>(buff + shift), pLogger);
    memcpy(buff + shift, pData, sizeof(double) * dim);

    return vec;
}

IVector* IVector::add(IVector const* pOperand1, IVector const* pOperand2, ILogger* pLogger) {
    if (pOperand1 == nullptr || pOperand2 == nullptr) {
       if (pLogger != nullptr) {
           pLogger->log("in IVector::add: nullptr", RESULT_CODE::BAD_REFERENCE);
       }
       return nullptr;
    }

    if (pOperand1->getDim() != pOperand2->getDim()) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::add: unequal dimensions", RESULT_CODE::WRONG_DIM);
        }
        return nullptr;
    }

    IVector* res = pOperand1->clone();

    if (res == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::add: nullptr", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    size_t commonDim = pOperand1->getDim();

    for (size_t i = 0; i < commonDim; ++i) {
        if (res->setCoord(i, pOperand1->getCoord(i) + pOperand2->getCoord(i)) != RESULT_CODE::SUCCESS) {
            delete res;
            return nullptr;
        }
    }

    return res;
}

IVector* IVector::sub(IVector const* pOperand1, IVector const* pOperand2, ILogger* pLogger) {
    if (pOperand1 == nullptr || pOperand2 == nullptr) {
       if (pLogger != nullptr) {
           pLogger->log("in IVector::sub: nullptr", RESULT_CODE::BAD_REFERENCE);
       }
       return nullptr;
    }

    if (pOperand1->getDim() != pOperand2->getDim()) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::sub: unequal dimensions", RESULT_CODE::WRONG_DIM);
        }
        return nullptr;
    }

    IVector* res = pOperand1->clone();

    if (res == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::sub: nullptr", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    size_t commonDim = pOperand1->getDim();

    for (size_t i = 0; i < commonDim; ++i) {
        if (res->setCoord(i, pOperand1->getCoord(i) - pOperand2->getCoord(i)) != RESULT_CODE::SUCCESS) {
            delete res;
            return nullptr;
        }
    }

    return res;
}

IVector* IVector::mul(IVector const* pOperand1, double scaleParam, ILogger* pLogger) {
    if (pOperand1 == nullptr) {
       if (pLogger != nullptr) {
           pLogger->log("in IVector::mul: nullptr", RESULT_CODE::BAD_REFERENCE);
       }
       return nullptr;
    }

    if (std::isnan(scaleParam)) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::mul: scaleParam is not a number", RESULT_CODE::WRONG_DIM);
        }
        return nullptr;
    }

    IVector* res = pOperand1->clone();

    if (res == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::mul: nullptr", RESULT_CODE::BAD_REFERENCE);
        }
        return nullptr;
    }

    size_t commonDim = pOperand1->getDim();

    for (size_t i = 0; i < commonDim; ++i) {
        if (res->setCoord(i, pOperand1->getCoord(i) * scaleParam) != RESULT_CODE::SUCCESS) {
            delete res;
            return nullptr;
        }
    }

    return res;
}

double IVector::mul(IVector const* pOperand1, IVector const* pOperand2, ILogger* pLogger) {
    if (pOperand1 == nullptr || pOperand2 == nullptr) {
       if (pLogger != nullptr) {
           pLogger->log("in IVector::mul: nullptr", RESULT_CODE::BAD_REFERENCE);
       }
       return std::numeric_limits<double>::quiet_NaN();
    }

    if (pOperand1->getDim() != pOperand2->getDim()) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::mul: unequal dimensions", RESULT_CODE::WRONG_DIM);
        }
        return std::numeric_limits<double>::quiet_NaN();
    }

    double res = 0;

    size_t commonDim = pOperand1->getDim();

    for (size_t i = 0; i < commonDim; ++i) {
        res += pOperand1->getCoord(i) * pOperand2->getCoord(i);
    }

    return res;
}

RESULT_CODE IVector::equals(IVector const* pOperand1, IVector const* pOperand2, NORM norm, double tolerance, bool* result, ILogger* pLogger) {
    if (std::isnan(tolerance)) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::equals: tolerance is not a number", RESULT_CODE::NAN_VALUE);
        }
        return RESULT_CODE::NAN_VALUE;
    }

    if (pOperand1 == nullptr || pOperand2 == nullptr) {
       if (pLogger != nullptr) {
           pLogger->log("in IVector::equals: nullptr", RESULT_CODE::BAD_REFERENCE);
       }
       return RESULT_CODE::BAD_REFERENCE;
    }

    if (pOperand1->getDim() != pOperand2->getDim())
    {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::equals: unequal dimensions", RESULT_CODE::WRONG_DIM);
        }
        return RESULT_CODE::WRONG_DIM;
    }

    auto diff = sub(pOperand1, pOperand2, pLogger);

    if (diff == nullptr) {
        if (pLogger != nullptr) {
            pLogger->log("in IVector::equals: nullptr", RESULT_CODE::BAD_REFERENCE);
        }
        return RESULT_CODE::BAD_REFERENCE;
    }

    *result = diff->norm(norm) < tolerance;
    delete diff;

    return RESULT_CODE::SUCCESS;
}

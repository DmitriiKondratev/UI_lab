#include <new>
#include <cmath>

#include "include/IProblem.h"
#include "include/IBrocker.h"

#define DIM 2

namespace {
    class ProblemImpl : public IProblem {
    private:
        const size_t argsDim, paramsDim;
        IVector *params;
        ILogger *logger;
    public:
        ProblemImpl(): argsDim(DIM), paramsDim(DIM), params(nullptr) {
            logger = ILogger::createLogger(this);
        }

        ~ProblemImpl() override {
            delete params;
            if (logger != nullptr) {
                logger->destroyLogger(this);
            }
        }

        RESULT_CODE goalFunction(IVector const* args, IVector const* params, double& res) const override {
            if (args == nullptr || params == nullptr) {
                if (logger != nullptr) {
                    logger->log("in ProblemImpl::goalFunction: null params or args", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            if (args->getDim() != argsDim || params->getDim() != paramsDim) {
                if (logger != nullptr) {
                    logger->log("in ProblemImpl::goalFunction: wrong dimension of arg or param", RESULT_CODE::WRONG_DIM);
                }
                return RESULT_CODE::WRONG_DIM;
            }

            double x = args->getCoord(0);
            double y = args->getCoord(1);
            double a = params->getCoord(0);

            res = y - x * x + a;
            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE goalFunctionByArgs(IVector const*  args, double& res) const override {
            return goalFunction(args, params, res);
        }

        size_t getArgsDim() const override { return argsDim; }
        size_t getParamsDim() const override { return paramsDim; }

        RESULT_CODE setParams(IVector const* params) override {
            if (params == nullptr) {
                if (logger != nullptr) {
                    logger->log("in ProblemImpl::setParams: null params", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            auto dim = paramsDim;
            if (params->getDim() != dim) {
                if (logger != nullptr) {
                    logger->log("in ProblemImpl::setParams: wrong dimension of param", RESULT_CODE::WRONG_DIM);
                }
                return RESULT_CODE::WRONG_DIM;
            }

            if (this->params == nullptr) {
                this->params = params->clone();
                if (this->params == nullptr) {
                    if (logger != nullptr) {
                        logger->log("in ProblemImpl::setParams", RESULT_CODE::BAD_REFERENCE);
                    }
                    return RESULT_CODE::BAD_REFERENCE;
                }
            } else {
                for (size_t i = 0; i < dim; i++) {
                    this->params->setCoord(i, params->getCoord(i));
                }
            }

            return RESULT_CODE::SUCCESS;
        }

        bool isCompactValid(ICompact const* const &compact) const override {
            if (compact == nullptr) {
                if (logger != nullptr) {
                    logger->log("in ProblemImpl::isCompactValid: null compact", RESULT_CODE::BAD_REFERENCE);
                }
                return false;
            }

            if (compact->getDim() != argsDim) {
                if (logger != nullptr) {
                    logger->log("in ProblemImpl::isCompactValid: wrong dim", RESULT_CODE::WRONG_DIM);
                }
                return false;
            }

            auto
                    begin = compact->getBegin(),
                    end = compact->getEnd();

            bool res = false;
            if (begin && end) {
                double a = params->getCoord(0);

                double xBeg = begin->getCoord(0);
                double yBeg = begin->getCoord(1);

                double xEnd = end->getCoord(0);
                double yEnd = end->getCoord(1);

                double tmp = (yBeg - xBeg * xBeg + a) * (yEnd - xEnd * xEnd + a);
                res = tmp < 0 || std::abs(tmp) < 1e-6;
            }

            delete begin;
            delete end;

            return res;
        }
    };

    class BrockerImpl : public IBrocker {
    private:
        ProblemImpl *problem;

        BrockerImpl(const BrockerImpl& other) = delete;
        void operator=(const BrockerImpl& other) = delete;

    public:
        BrockerImpl(ProblemImpl* problem): problem(problem) {}

        Type getType() const override {
            return Type::PROBLEM;
        }

        void* getInterfaceImpl(Type type) const override {
            switch (type) {
            case Type::PROBLEM:
                return problem;
            default:
                return nullptr;
            }
        }

        void release() override {
            delete problem;
            problem = nullptr;
        }

        ~BrockerImpl() override {
            release();
        }
    };
}

extern "C" {
    LIBRARY_EXPORT IBrocker* getBrocker() {
        ProblemImpl* problem = new (std::nothrow) ProblemImpl();
        if (problem != nullptr) {
            IBrocker* br = new (std::nothrow) BrockerImpl(problem);
            if (br != nullptr) { return br; }
            delete problem;
        }
        return nullptr;
    }
}

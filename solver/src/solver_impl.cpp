#include <new>
#include <cmath>
#include <QStringList>

#include "include/ISolver.h"
#include "include/IBrocker.h"

namespace {
    class SolverImpl : public ISolver {
    private:
        IVector *solution;
        IVector *params;
        IVector *problemParams;
        ILogger *logger;
        IProblem *problem;
        ICompact *compact;

        SolverImpl(const SolverImpl& other) = delete;
        void operator=(const SolverImpl& other) = delete;

    public:
        SolverImpl(): solution(nullptr), params(nullptr),
            problemParams(nullptr), problem(nullptr),
            compact(nullptr) {
            logger = ILogger::createLogger(this);
        }

        ~SolverImpl() {
            delete solution;
            delete params;
            delete problemParams;
            delete compact;
            logger->destroyLogger(this);
        }

        RESULT_CODE setParams(IVector const* params) override {
            if (params == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setParams: null param", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }


            if (this->params != nullptr) { delete this->params; }

            this->params = params->clone();
            if (this->params == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setParams", RESULT_CODE::OUT_OF_MEMORY);
                }
                return RESULT_CODE::OUT_OF_MEMORY;
            }

            return RESULT_CODE::SUCCESS;
        }

        /* Grammar:
         *    dim = { INT }; step = { DOUBLE[DIM] }
         * Example:
         *    dim = 3; step = 0.01, 0.02, 0.03
         */
        RESULT_CODE setParams(QString& str) override {
            static struct Grammar {
                const QString   paramSeparator = ";";
                const QString   coordSeparator = ",";
                const QString   assign      = "=";
                const QString   dimension   = "dim";
                const QString   step        = "step";
                const QRegExp   strFlag     = QRegExp("\\s");
                const int       paramsCount = 2;
            } grammar;

            auto splitStr = str.split(grammar.paramSeparator, Qt::SplitBehaviorFlags::SkipEmptyParts);
            if (splitStr.size() != grammar.paramsCount) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setParams: Bad grammar of params string", RESULT_CODE::WRONG_ARGUMENT);
                }
                return RESULT_CODE::WRONG_ARGUMENT;
            }

            bool check;
            uint dim = 0;
            double *data = nullptr;
            for (auto str : splitStr) {
                auto args = str.split(grammar.assign, Qt::SplitBehaviorFlags::SkipEmptyParts);

                if (args.size() != 2) {
                    if (logger != nullptr) {
                        logger->log("in SolverImpl::setParams: bad param grammar (should be KEY = VALUE)", RESULT_CODE::WRONG_ARGUMENT);
                    }
                    return RESULT_CODE::WRONG_ARGUMENT;
                }

                if (args[0].remove(grammar.strFlag) == grammar.dimension) {
                    dim = args[1].toUInt(&check);
                    if (!check) {
                        if (logger != nullptr) {
                            logger->log("in SolverImpl::setParams: bad grammar (dimension should be uint)", RESULT_CODE::WRONG_ARGUMENT);
                        }
                        return RESULT_CODE::WRONG_ARGUMENT;
                    }
                } else if (args[0].remove(grammar.strFlag) == grammar.step) {
                    auto step = args[1].split(grammar.coordSeparator);

                    if (static_cast<uint>(step.size()) != dim) {
                        if (logger != nullptr) {
                            logger->log("in SolverImpl::setParams: bad grammar (step doesn't have required dimension)", RESULT_CODE::WRONG_ARGUMENT);
                        }
                        return RESULT_CODE::WRONG_ARGUMENT;
                    }

                    data = new (std::nothrow) double[dim];
                    if (data == nullptr) {
                        if (logger != nullptr) {
                            logger->log("in SolverImpl::setParams: not enough memory for data", RESULT_CODE::OUT_OF_MEMORY);
                        }
                        return RESULT_CODE::OUT_OF_MEMORY;
                    }

                    for (uint i = 0; i < dim; i++) {
                        data[i] = step[i].toDouble(&check);

                        if (!check) {
                            if (logger != nullptr) {
                                logger->log("in SolverImpl::setParams: bad grammar (coord should be double)", RESULT_CODE::WRONG_ARGUMENT);
                            }

                            delete[] data;
                            return RESULT_CODE::WRONG_ARGUMENT;
                        }
                    }
                } else {
                    if (logger != nullptr) {
                        logger->log("in SolverImpl::setParams: bad grammar (wrong param name)", RESULT_CODE::WRONG_ARGUMENT);
                    }
                    return RESULT_CODE::WRONG_ARGUMENT;
                }
            }

            IVector *vec = IVector::createVector(dim, data, logger);
            delete[] data;

            if (vec == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setParams: bad creation of vector", RESULT_CODE::WRONG_ARGUMENT);
                }
                return RESULT_CODE::WRONG_ARGUMENT;
            }

            params = vec;
            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE setProblem(IProblem *pProblem) override {
            if (pProblem == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setProblem: null param", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            problem = pProblem;
            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE setProblemParams(IVector const* params) override {
            if (params == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setProblemParams: null param", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            if (this->problemParams != nullptr) {
                delete this->problemParams;
            }

            this->problemParams = params->clone();
            if (this->problemParams == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setProblemParams", RESULT_CODE::OUT_OF_MEMORY);
                }
                return RESULT_CODE::OUT_OF_MEMORY;
            }

            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE setCompact(ICompact * pCompact) override {
            if (pCompact == nullptr) {
                if (logger != nullptr) {
                    logger->log("solver::setCompact: null param", RESULT_CODE::BAD_REFERENCE);
                }
                return RESULT_CODE::BAD_REFERENCE;
            }

            if (compact != nullptr) { delete compact; }

            compact = pCompact->clone();
            if (compact == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setCompact", RESULT_CODE::OUT_OF_MEMORY);
                }
                return RESULT_CODE::OUT_OF_MEMORY;
            }

            return RESULT_CODE::SUCCESS;
        }

        size_t getParamsDim() const override {
            return params == nullptr ? 0 : params->getDim();
        }

        RESULT_CODE solve() override {
            static const double tolerance = 1e-6;

            if (!problem->isCompactValid(compact)) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::solve: not valid compact", RESULT_CODE::WRONG_ARGUMENT);
                }
                return RESULT_CODE::WRONG_ARGUMENT;
            }

            if (params->getDim() != compact->getDim()) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::solver: params dimension should be equal to compact", RESULT_CODE::WRONG_DIM);
                }
                return RESULT_CODE::WRONG_DIM;
            }

            ICompact::iterator* it;
            if (params->getCoord(0) > 0) {
                it = compact->begin(params);
            } else {
                it= compact->end(params);
            }
            if (it == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::solve: not valid step", RESULT_CODE::WRONG_ARGUMENT);
                }
                return RESULT_CODE::WRONG_ARGUMENT;
            }

            if (problemParams != nullptr) {
                auto rc = problem->setParams(problemParams);
                if (rc != RESULT_CODE::SUCCESS) {
                    if (logger != nullptr) {
                        logger->log("in SolverImpl::solve: something wrong", rc);
                    }
                    return rc;
                }
            }

            auto dim = compact->getDim();
            double *data = new (std::nothrow) double[dim];

            if (data == nullptr) {
                delete it;
                if (logger != nullptr) {
                    logger->log("in SolverImpl::solve: no memory", RESULT_CODE::OUT_OF_MEMORY);
                }
                return RESULT_CODE::OUT_OF_MEMORY;
            }

            IVector *bestSolution = IVector::createVector(dim, data, logger);
            delete[] data;

            if (bestSolution == nullptr) {
                delete it;
                if (logger != nullptr) {
                    logger->log("in SolverImpl::solve: something bad with vector created", RESULT_CODE::WRONG_ARGUMENT);
                }
                return RESULT_CODE::WRONG_ARGUMENT;
            }

            const double startBestRes = 1e10;
            double bestRes = startBestRes;
            double curRes;

            auto check = RESULT_CODE::SUCCESS;
            do {
                auto rc = problem->goalFunctionByArgs(it->getPoint(), curRes);

                if (rc != RESULT_CODE::SUCCESS) {
                    if (logger != nullptr) {
                        delete bestSolution;
                        logger->log("in SolverImpl::solve: something wrong with goalFunctionByArgs", RESULT_CODE::WRONG_ARGUMENT);
                    }
                    return RESULT_CODE::WRONG_ARGUMENT;
                }

                if (curRes < bestRes) {
                    bestRes = curRes;
                    auto point = it->getPoint();
                    if (point == nullptr) {
                        delete it;
                        delete bestSolution;

                        if (logger != nullptr) {
                            logger->log("in SolverImpl::solve: something wrong with goalFunctionByArgs", RESULT_CODE::WRONG_ARGUMENT);
                        }
                        return RESULT_CODE::WRONG_ARGUMENT;
                    }

                    for (size_t i = 0; i < dim; i++) {
                        if (bestSolution->setCoord(i, point->getCoord(i)) != RESULT_CODE::SUCCESS) {
                            delete point;
                            delete it;
                            delete bestSolution;

                            if (logger != nullptr) {
                                logger->log("in SolverImpl::solve: something wrong with setCoord", RESULT_CODE::WRONG_ARGUMENT);
                            }
                            return RESULT_CODE::WRONG_ARGUMENT;
                        }
                    }
                    delete point;
                }
                check = it->doStep();
            } while (check == RESULT_CODE::SUCCESS);
            delete it;

            if (std::abs(startBestRes - bestRes) < tolerance) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::solve: solution not found", RESULT_CODE::NOT_FOUND);
                }
                return RESULT_CODE::NOT_FOUND;
            }

            if (solution != nullptr) { delete solution; }

            solution = bestSolution->clone();
            if (solution == nullptr) {
                delete bestSolution;
                if (logger != nullptr) {
                    logger->log("in SolverImpl::solve", RESULT_CODE::OUT_OF_MEMORY);
                }
                return RESULT_CODE::OUT_OF_MEMORY;
            }
            delete bestSolution;

            return RESULT_CODE::SUCCESS;
        }

        RESULT_CODE getSolution(IVector * &vec) const override {
            if (solution == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl: no solution yet", RESULT_CODE::NOT_FOUND);
                }
                return RESULT_CODE::NOT_FOUND;
            }

            vec = solution->clone();
            if (vec == nullptr) {
                if (logger != nullptr) {
                    logger->log("in SolverImpl::setCompact", RESULT_CODE::OUT_OF_MEMORY);
                }
                return RESULT_CODE::OUT_OF_MEMORY;
            }

            return RESULT_CODE::SUCCESS;
        }
    };

    class BrockerImpl : public IBrocker {
    private:
        SolverImpl *solver;

        BrockerImpl(const BrockerImpl& other) = delete;
        void operator=(const BrockerImpl& other) = delete;

    public:
        BrockerImpl(SolverImpl* solver): solver(solver) {}

        Type getType() const override {
            return Type::SOLVER;
        }

        void* getInterfaceImpl(Type type) const override {
            switch (type) {
            case Type::SOLVER:
                return solver;
            default:
                return nullptr;
            }
        }

        void release() override {
            delete solver;
            solver = nullptr;
        }

        ~BrockerImpl() override {
            release();
        }
    };
}

extern "C" {
    LIBRARY_EXPORT IBrocker* getBrocker() {
        SolverImpl* solver = new (std::nothrow) SolverImpl();
        if (solver != nullptr) {
            IBrocker* br = new (std::nothrow) BrockerImpl(solver);
            if (br != nullptr) { return br; }
            delete solver;
        }
        return nullptr;
    }
}

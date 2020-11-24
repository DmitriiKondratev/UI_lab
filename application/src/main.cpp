#include <iostream>

#include <QFileInfo>
#include <QLibrary>
#include <QString>

#include "include/ILogger.h"
#include "include/IBrocker.h"
#include "include/IProblem.h"
#include "include/ISolver.h"

// uncomment if you want to input data yourself
// #define USERS_LIBS

typedef IBrocker *(* get_brocker_func)();

template<typename InterfaceT, IBrocker::Type type>
InterfaceT * load(QFileInfo const& fileName, IBrocker*& brocker, ILogger* logger) {
    QLibrary lib(fileName.absoluteFilePath());

    if (!lib.load()) {
        std::cout << lib.errorString().toStdString() << "\n";
        if (logger != nullptr) {
            logger->log("Library was not found", RESULT_CODE::NOT_FOUND);
        }
        return nullptr;
    }

    auto getBrocker = reinterpret_cast<get_brocker_func>(lib.resolve("getBrocker"));

    if (getBrocker == nullptr) {
        if (logger != nullptr) {
            logger->log("Function getBrocker was not found", RESULT_CODE::NOT_FOUND);
        }
        return nullptr;
    }

    brocker = getBrocker();

    if (brocker->getType() != type) {
        if (logger != nullptr) {
            logger->log("Brocker cannot create required interface", RESULT_CODE::WRONG_ARGUMENT);
        }
        return nullptr;
    }

    return reinterpret_cast<InterfaceT *>(brocker->getInterfaceImpl(type));
}

static void print(FILE* stream, IVector const* vec) {
    if (stream == nullptr) { stream = stdout; }

    if (vec == nullptr) {
        fprintf(stream, "error");
    } else {
        fprintf(stream, "[");

        size_t dim = vec->getDim();
        if (dim) {
            fprintf(stream, "%lf", vec->getCoord(0));
            for (size_t i = 1; i < dim; ++i) {
                fprintf(stream, ", %lf", vec->getCoord(i));
            }
        }
        fprintf(stream, "]");
    }
    fprintf(stream, "\n");
}

int main(int argc, char *argv[]) {
    ILogger *logger = ILogger::createLogger(argv[0]);

    std::string path;
    #ifndef USERS_LIBS
        path = "../application/libs/problem";
    #else
        std::cout << "Input path to problem library: ";
        std::cin >> path;
    #endif

    IBrocker *problemBrocker;
    auto problem = load<IProblem, IBrocker::Type::PROBLEM>(QFileInfo(path.c_str()), problemBrocker, logger);

    if (problem == nullptr) {
        std::cout << "Problem was not loaded\n";
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    #ifndef USERS_LIBS
        path = "../application/libs/solver";
    #else
        std::cout << "Input path to solver library: ";
        std::cin >> path;
    #endif

    IBrocker *solverBrocker;
    auto solver = load<ISolver, IBrocker::Type::SOLVER>(QFileInfo(path.c_str()), solverBrocker, logger);

    if (solver == nullptr) {
        std::cout << "Solver was not loaded\n";
        problemBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }


    size_t dim = 0;
    #ifndef USERS_LIBS
        dim = 2;
    #else
        std::cout << "Input problem params dimension: ";
        std::cin >> dim;
    #endif


    double *data = new double[dim];
    if (data == nullptr) {
        std::cout << "Not enough memory for data array\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    #ifndef USERS_LIBS
        data[0] = 10;
        data[1] = 100;
    #else
        std::cout << "Input problem params vector, separated by space or enter: ";
        for (size_t i = 0; i < dim; i++) {
            std::cin >> data[i];
        }
    #endif

    IVector *vec = IVector::createVector(dim, data, logger);
    auto rc = problem->setParams(vec);
    delete[] data;
    delete vec;

    if (rc != RESULT_CODE::SUCCESS) {
        std::cout << "Something wrong\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    rc = solver->setProblem(problem);
    if (rc != RESULT_CODE::SUCCESS) {
        std::cout << "Something wrong\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    #ifndef USERS_LIBS
        dim = 2;
    #else
        std::cout << "Input solver params dimension: ";
        std::cin >> dim;
    #endif

    data = new double[dim];
    if (data == nullptr) {
        std::cout << "Not enough memory for data array\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    #ifndef USERS_LIBS
        data[0] = 0.01;
        data[1] = 0.02;
    #else
        std::cout << "Input solver params vector, separated by space or enter: ";
        for (size_t i = 0; i < dim; i++) {
            std::cin >> data[i];
        }
    #endif

    vec = IVector::createVector(dim, data, logger);
    rc = solver->setParams(vec);
    delete[] data;
    delete vec;

    if (rc != RESULT_CODE::SUCCESS) {
        std::cout << "Something wrong\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    #ifndef USERS_LIBS
        dim = 2;
    #else
        std::cout << "Input compact to be used in solver:\ndimension: ";
        std::cin >> dim;
    #endif

    data = new double[dim];
    if (data == nullptr) {
        std::cout << "Not enough memory for data array\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    #ifndef USERS_LIBS
        data[0] = 0;
        data[1] = 0;
    #else
        std::cout << "begin vector:\n";
        for (size_t i = 0; i < dim; i++) {
            std::cin >> data[i];
        }
    #endif
    auto beg = IVector::createVector(dim, data, logger);

    #ifndef USERS_LIBS
        data[0] = 5;
        data[1] = 4;
    #else
        std::cout << "end vector:\n";
        for (size_t i = 0; i < dim; i++) {
            std::cin >> data[i];
        }
    #endif
    auto end = IVector::createVector(dim, data, logger);
    delete[] data;

    auto compact = ICompact::createCompact(beg, end, logger);
    rc = solver->setCompact(compact);

    delete compact;
    delete beg;
    delete end;

    if (rc != RESULT_CODE::SUCCESS) {
        std::cout << "Something wrong\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    rc = solver->solve();
    if (rc != RESULT_CODE::SUCCESS) {
        std::cout << "Something wrong\n";
        problemBrocker->release();
        solverBrocker->release();
        if (logger != nullptr) {
            logger->destroyLogger(argv[0]);
        }
        return 0;
    }

    IVector *solution = nullptr;
    rc = solver->getSolution(solution);

    if (rc != RESULT_CODE::SUCCESS) {
        std::cout << "Soultion wasn't found\n";
    } else {
        std::cout << "Solution was found: ";
        print(nullptr, solution);
    }

    delete solution;
    problemBrocker->release();
    solverBrocker->release();
    if (logger != nullptr) {
        logger->destroyLogger(argv[0]);
    }

    return 0;
}

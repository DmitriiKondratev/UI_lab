#include <cassert>
#include <fstream>
#include <set>

#include "ILogger.h"

namespace {
    // singleton meyers
    class LoggerImpl: public ILogger {
    private:
        std::set<void*> clients;
        FILE* pLogStream = stderr;

        LoggerImpl(LoggerImpl const& vector) = delete;
        LoggerImpl& operator=(LoggerImpl const& vector) = delete;

        friend ILogger * ILogger::createLogger(void *pClient);

    protected:
        static LoggerImpl* instance() {
            static LoggerImpl logger;
            return &logger;
        }

        bool addClient(void* pClient) {
            assert(pClient);
            auto iter = clients.find(pClient);

            if (iter == clients.end()) {
                clients.insert(pClient);
                return true;
            }
            return false;
        }

        bool removeClient(void* pClient) {
            assert(pClient);
            auto iter = clients.find(pClient);

            if (iter != clients.end()) {
                clients.erase(pClient);
                return true;
            }
            return false;
        }

        LoggerImpl() = default;

        ~LoggerImpl() override {
            if (pLogStream != stderr) { fclose(pLogStream); }
        }

    public:
        void destroyLogger(void* pClient) override;
        void log(char const* pMsg, RESULT_CODE err) override;
        RESULT_CODE setLogFile(char const* pLogFile) override;
    };

    void LoggerImpl::destroyLogger(void* pClient) {
        if (pClient != nullptr) {
            LoggerImpl* logger = LoggerImpl::instance();
            logger->removeClient(pClient);
        }
    }

    void LoggerImpl::log(char const *pMsg, RESULT_CODE err) {
        assert(!clients.empty());

        char const *startMsg;
        switch (err) {
            case RESULT_CODE::SUCCESS:
                startMsg = "INFO: ";
                break;
            case RESULT_CODE::OUT_OF_MEMORY:
                startMsg = "ERROR (out of memory): ";
                break;
            case RESULT_CODE::BAD_REFERENCE:
                startMsg = "ERROR (bad reference): ";
                break;
            case RESULT_CODE::WRONG_DIM:
                startMsg = "ERROR (wrong dimension): ";
                break;
            case RESULT_CODE::DIVISION_BY_ZERO:
                startMsg = "ERROR (division by zero): ";
                break;
            case RESULT_CODE::NAN_VALUE:
                startMsg = "ERROR (not a number): ";
                break;
            case RESULT_CODE::FILE_ERROR:
                startMsg = "ERROR (file error): ";
                break;
            case RESULT_CODE::OUT_OF_BOUNDS:
                startMsg = "ERROR (out of bonds): ";
                break;
            case RESULT_CODE::NOT_FOUND:
                startMsg = "ERROR (not found): ";
                break;
            case RESULT_CODE::WRONG_ARGUMENT:
                startMsg = "ERROR (wrong argument): ";
                break;
            case RESULT_CODE::CALCULATION_ERROR:
                startMsg = "ERROR (calculation error): ";
                break;
            case RESULT_CODE::MULTIPLE_DEFINITION:
                startMsg = "ERROR (multiple definition): ";
                break;
        }
        fprintf(pLogStream, "%s %s\n", startMsg, pMsg);
    }

    RESULT_CODE LoggerImpl::setLogFile(char const* pLogFile) {
        assert(!clients.empty());
        assert(pLogStream);

        if (pLogStream != stderr) {
            fclose(pLogStream);
            pLogStream = stderr;
        }

        if (pLogFile) {
            pLogStream = fopen(pLogFile, "w");
            if (!pLogStream)  {
                return RESULT_CODE::FILE_ERROR;
            }
        }
        return RESULT_CODE::SUCCESS;
    }
}

ILogger::~ILogger() {}

ILogger* ILogger::createLogger(void *pClient) {
    if (pClient != nullptr) {
        LoggerImpl* logger = LoggerImpl::instance();
        if (logger->addClient(pClient)) { return logger; }
    }
    return nullptr;
}

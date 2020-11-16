TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    src/main.cpp

HEADERS += \
    include/library_global.h \
    include/test.h \
    include/RC.h \
    include/ILogger.h \
    include/IVector.h

LIBS += \
    -L$$PWD/libs/ -llogger \
    -L$$PWD/libs/ -lvector

DISTFILES += \
    libs/logger.dll \
    libs/vector.dll

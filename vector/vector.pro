CONFIG -= qt

TEMPLATE = lib

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/vector_impl.cpp

HEADERS += \
    include/library_global.h \
    include/RC.h \
    include/ILogger.h \
    include/IVector.h

LIBS += \
    -L$$PWD/libs/ -llogger

DISTFILES += \
    libs/logger.dll

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

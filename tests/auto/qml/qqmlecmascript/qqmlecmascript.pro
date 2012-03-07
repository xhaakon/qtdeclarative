CONFIG += testcase
TARGET = tst_qqmlecmascript
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlecmascript.cpp \
           testtypes.cpp \
           ../../shared/testhttpserver.cpp
HEADERS += testtypes.h \
           ../../shared/testhttpserver.h
INCLUDEPATH += ../../shared

include (../../shared/util.pri)

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

TESTDATA = data/*

CONFIG += parallel_test
QT += core-private gui-private v8-private qml-private network widgets testlib

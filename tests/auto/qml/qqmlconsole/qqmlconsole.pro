CONFIG += testcase
TARGET = tst_qqmlconsole
SOURCES += tst_qqmlconsole.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

CONFIG += parallel_test

QT += qml testlib

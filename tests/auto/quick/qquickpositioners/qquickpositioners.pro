CONFIG += testcase
TARGET = tst_qquickpositioners
SOURCES += tst_qquickpositioners.cpp

include (../shared/util.pri)
include (../../shared/util.pri)

macx:CONFIG -= app_bundle

TESTDATA = data/*

CONFIG += parallel_test
QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib

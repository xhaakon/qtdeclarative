CONFIG += testcase
TARGET = tst_qquickpincharea
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpincharea.cpp

include (../../shared/util.pri)

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private testlib

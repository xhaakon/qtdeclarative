CONFIG += testcase
TARGET = tst_qquicksmoothedanimation
macx:CONFIG -= app_bundle

SOURCES += tst_qquicksmoothedanimation.cpp

include (../../shared/util.pri)

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private v8-private qml-private quick-private testlib

CONFIG += testcase
TARGET = tst_qquickbinding
macx:CONFIG -= app_bundle

SOURCES += tst_qquickbinding.cpp

include (../../shared/util.pri)

TESTDATA = data/*

CONFIG += parallel_test

QT += core-private gui-private qml-private quick-private testlib

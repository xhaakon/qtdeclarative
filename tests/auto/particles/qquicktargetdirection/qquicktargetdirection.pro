CONFIG += testcase
TARGET = tst_qquicktargetdirection
SOURCES += tst_qquicktargetdirection.cpp
macx:CONFIG -= app_bundle

include (../../shared/util.pri)
TESTDATA = data/*

QT += core-private gui-private v8-private qml-private quick-private opengl-private testlib


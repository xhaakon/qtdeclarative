load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
QT += sql script webkit
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativesqldatabase.cpp

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test


DEFINES += MODELTEST

RELPWD = $$replace(PWD,$$_PRO_FILE_PWD_,.)

INCLUDEPATH += $$RELPWD
DEPENDPATH  += $$RELPWD

SOURCES += $$RELPWD/modeltest.cpp

HEADERS += $$RELPWD/modeltest.h
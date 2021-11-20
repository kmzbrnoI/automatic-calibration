QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = auto-calib
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
	lib/wsm/wsm.cpp \
	lib/xn/xn.cpp \
	lib/xn/xn-api.cpp \
	lib/xn/xn-receive.cpp \
	lib/xn/xn-send.cpp \
	lib/xn/xn-hist.cpp \
	lib/xn/xn-win-com-discover.cpp \
	src/main.cpp \
	src/main-window.cpp \
	src/settings.cpp \
	src/power-map.cpp \
	src/power-graph-window.cpp \
	src/speed-map.cpp \
	src/calib-step.cpp \
	src/calib-man.cpp \
	src/calib-overview.cpp \
	src/calib-range.cpp

HEADERS += \
	lib/q-str-exception.h \
	lib/wsm/q-str-exception.h \
	lib/wsm/wsm.h \
	lib/xn/xn.h \
	lib/xn/xn-loco-addr.h \
	lib/xn/xn-commands.h \
	lib/xn/xn-win-com-discover.h \
	lib/xn/q-str-exception.h \
	src/main-window.h \
	src/settings.h \
	src/power-map.h \
	src/power-graph-window.h \
	src/speed-map.h \
	src/calib-step.h \
	src/calib-man.h \
	src/calib-overview.h \
	src/calib-range.h

FORMS += \
	form/main-window.ui \
	form/power-graph-window.ui

CONFIG += c++14
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
UI_DIR = src

QT += serialport
QT += charts

RESOURCES += auto-calib.qrc
win32:RC_ICONS += icon/app-icon.ico

win32 {
	LIBS += -lsetupapi
}
win64 {
	LIBS += -lsetupapi
}

VERSION_MAJOR = 1
VERSION_MINOR = 4

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
	"VERSION_MINOR=$$VERSION_MINOR"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}

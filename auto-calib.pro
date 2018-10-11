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
	main.cpp \
	main-window.cpp \
	lib/wsm/wsm.cpp \
	lib/xn/xn.cpp \
	settings.cpp \
	power-map.cpp \
	power-graph-window.cpp \
	speed-map.cpp \
	calib-step.cpp

HEADERS += \
	main-window.h \
	lib/q-str-exception.h \
	lib/wsm/wsm.h \
	lib/xn/xn.h \
	lib/xn/xn-typedefs.h \
	settings.h \
	power-map.h \
	power-graph-window.h \
	speed-map.h \
	calib-step.h

FORMS += \
	main-window.ui \
	power-graph-window.ui

CONFIG += c++14
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

QT += serialport
QT += charts

VERSION_MAJOR = 0
VERSION_MINOR = 1

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
	"VERSION_MINOR=$$VERSION_MINOR"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}

#dirs
UI_DIR = ./gui
MOC_DIR = ./moc
OBJECTS_DIR = ./obj
RCC_DIR = ./res
# compiler flags
QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -std=c++11
# prefix
targetprefix = $$(TARGET_PREFIX)
message("Using target prefix \"$${targetprefix}\".")
# print install root
message("Using install root \"$$(INSTALL_ROOT)\".")
# target
CONFIG(debug, debug|release) {
    TARGET = $${targetprefix}$${projectname}d
} else {
    TARGET = $${targetprefix}$${projectname}
}
# add defines
DEFINES += "'PROJECT_NAME=\"$${projectname}\"'"
DEFINES += "'APP_NAME=\"$${appname}\"'"
DEFINES += "'APP_AUTHOR=\"$${appauthor}\"'"
DEFINES += "'APP_URL=\"$${appurl}\"'"
DEFINES += "'APP_VERSION=\"$${VERSION}\"'"
# variables to check target architecture
win32-g++:QMAKE_TARGET.arch = $$QMAKE_HOST.arch
win32-g++-32:QMAKE_TARGET.arch = x86
win32-g++-64:QMAKE_TARGET.arch = x86_64
linux-g++:QMAKE_TARGET.arch = $$QMAKE_HOST.arch
linux-g++-32:QMAKE_TARGET.arch = x86
linux-g++-64:QMAKE_TARGET.arch = x86_64
# configuration
mobile {
    DEFINES += CONFIG_MOBILE
} else:desktop {
    DEFINES += CONFIG_DESKTOP
} else:android {
    CONFIG += mobile
    DEFINES += CONFIG_MOBILE
} else {
    CONFIG += desktop
    DEFINES += CONFIG_DESKTOP
}
no-gui {
    QT -= gui
    DEFINES += GUI_NONE
    guiqtquick || guiqtwidgets {
        error("Can not use no-gui with guiqtquick or guiqtwidgets.")
    } else {
        message("Configured for no GUI support.")
    }
} else {
    QT += gui
    mobile {
        CONFIG += guiqtquick
    }
    desktop {
        CONFIG += guiqtwidgets
    }
}
guiqtquick {
    message("Configured for Qt Quick GUI support.")
    greaterThan(QT_MAJOR_VERSION, 4): QT += quick
    CONFIG(debug, debug|release) {
        CONFIG += qml_debug
    }
    DEFINES += GUI_QTQUICK
}
guiqtwidgets {
    message("Configured for Qt widgets GUI support.")
    greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
    DEFINES += GUI_QTWIDGETS
    DEFINES += MODEL_UNDO_SUPPORT
}
# Windows stuff: configuration for cross compliation with mingw-w64
win32 {
    QMAKE_TARGET_PRODUCT = "$${appname}"
    QMAKE_TARGET_COPYRIGHT = "by $${appauthor}"
}
mingw-w64-manualstrip-dll {
    QMAKE_POST_LINK=$${CROSS_COMPILE}strip --strip-unneeded ./release/$(TARGET); \
        $${CROSS_COMPILE}strip --strip-unneeded ./release/lib$(TARGET).a
}
mingw-w64-manualstrip-exe {
    QMAKE_POST_LINK=$${CROSS_COMPILE}strip --strip-unneeded ./release/$(TARGET)
}
mingw-w64-noversion {
    TARGET_EXT = ".dll"
    TARGET_VERSION_EXT = ""
    CONFIG += skip_target_version_ext
}

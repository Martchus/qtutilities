# meta data
projectname = qtutilities
appname = "Qt Utilities"
appauthor = Martchus
QMAKE_TARGET_DESCRIPTION = "Common Qt related C++ classes and routines used by my applications such as dialogs, widgets and models."
VERSION = 3.1.1

# include ../../common.pri when building as part of a subdirs project; otherwise include general.pri
!include(../../common.pri) {
    !include(./general.pri) {
        error("Couldn't find the common.pri or the general.pri file!")
    }
}

# basic configuration: shared library
TEMPLATE = lib
QT += core gui
CONFIG += shared

# enable platform specific capslock detection (for password dialog)
CONFIG(noplatformspecificcapslockdetection, noplatformspecificcapslockdetection|platformspecificcapslockdetection) {
    DEFINES -= PLATFORM_SPECIFIC_CAPSLOCK_DETECTION
} else {
    DEFINES += PLATFORM_SPECIFIC_CAPSLOCK_DETECTION
}

# add project files
HEADERS += \
    resources/resources.h \
    models/checklistmodel.h \
    resources/qtconfigarguments.h \
    misc/dialogutils.h \
    misc/desktoputils.h \
    misc/xmlparsermacros.h

SOURCES += resources/resources.cpp \
    models/checklistmodel.cpp \
    resources/qtconfigarguments.cpp \
    misc/dialogutils.cpp \
    misc/desktoputils.cpp

contains(DEFINES, GUI_QTWIDGETS) {
    HEADERS += \
        aboutdialog/aboutdialog.h \
        enterpassworddialog/enterpassworddialog.h \
        settingsdialog/optioncategorymodel.h \
        settingsdialog/settingsdialog.h \
        settingsdialog/optioncategory.h \
        settingsdialog/optionpage.h \
        settingsdialog/optioncategoryfiltermodel.h \
        widgets/clearlineedit.h \
        widgets/iconbutton.h \
        widgets/buttonoverlay.h \
        widgets/clearcombobox.h \
        widgets/clearspinbox.h \
        widgets/clearplaintextedit.h

    SOURCES += \
        aboutdialog/aboutdialog.cpp \
        enterpassworddialog/enterpassworddialog.cpp \
        settingsdialog/optioncategorymodel.cpp \
        settingsdialog/settingsdialog.cpp \
        settingsdialog/optionpage.cpp \
        settingsdialog/optioncategory.cpp \
        settingsdialog/optioncategoryfiltermodel.cpp \
        widgets/clearlineedit.cpp \
        widgets/iconbutton.cpp \
        widgets/buttonoverlay.cpp \
        widgets/clearcombobox.cpp \
        widgets/clearspinbox.cpp \
        widgets/clearplaintextedit.cpp

    FORMS += \
        aboutdialog/aboutdialog.ui \
        enterpassworddialog/enterpassworddialog.ui \
        settingsdialog/settingsdialog.ui
}

RESOURCES += \
    resources/qtutilsicons.qrc

OTHER_FILES += \
    README.md \
    LICENSE \
    CMakeLists.txt \
    resources/config.h.in \
    resources/windows.rc.in


# add libs
CONFIG(debug, debug|release) {
    LIBS += -lc++utilitiesd
} else {
    LIBS += -lc++utilities
}
contains(DEFINES, PLATFORM_SPECIFIC_CAPSLOCK_DETECTION) {
    x11 {
        LIBS += -lX11
    }
}

# installs
mingw-w64-install {
    target.path = $$(INSTALL_ROOT)
    target.extra = install -m755 -D $${OUT_PWD}/release/lib$(TARGET).a $$(INSTALL_ROOT)/lib/lib$(TARGET).a
    INSTALLS += target
    dlltarget.path = $$(INSTALL_ROOT)
    dlltarget.extra = install -m755 -D $${OUT_PWD}/release/$(TARGET) $$(INSTALL_ROOT)/bin/$(TARGET)
    INSTALLS += dlltarget
} else {
    target.path = $$(INSTALL_ROOT)/lib
    INSTALLS += target
}
for(dir, $$list(aboutdialog enterpassworddialog models resources settingsdialog widgets misc)) {
    eval(inc_$${dir} = $${dir})
    inc_$${dir}.path = $$(INSTALL_ROOT)/include/$$projectname/$${dir}
    inc_$${dir}.files = $${dir}/*.h
    INSTALLS += inc_$${dir}
}

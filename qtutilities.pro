projectname = qtutilities

# include ../../common.pri when building as part of a subdirs project; otherwise include general.pri
!include(../../common.pri) {
    !include(./general.pri) {
        error("Couldn't find the common.pri or the general.pri file!")
    }
}

QT += core gui

CONFIG(noplatformspecificcapslockdetection, noplatformspecificcapslockdetection|platformspecificcapslockdetection) {
    DEFINES -= PLATFORM_SPECIFIC_CAPSLOCK_DETECTION
} else {
    DEFINES += PLATFORM_SPECIFIC_CAPSLOCK_DETECTION
}

win32 {
    CONFIG += dll
}

contains(DEFINES, PLATFORM_SPECIFIC_CAPSLOCK_DETECTION) {
    x11 {
        LIBS += -lX11
    }
}

SOURCES += resources/resources.cpp \
        models/checklistmodel.cpp \
    resources/qtconfigarguments.cpp

contains(DEFINES, GUI_QTWIDGETS) {
    SOURCES += aboutdialog/aboutdialog.cpp \
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

    FORMS += aboutdialog/aboutdialog.ui \
        enterpassworddialog/enterpassworddialog.ui \
        settingsdialog/settingsdialog.ui
}

HEADERS += resources/resources.h \
        models/checklistmodel.h \
    resources/qtconfigarguments.h

contains(DEFINES, GUI_QTWIDGETS) {
    HEADERS += aboutdialog/aboutdialog.h \
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
}

OTHER_FILES += \
    pkgbuild/default/PKGBUILD \
    pkgbuild/mingw-w64/PKGBUILD

# libs and includepath
CONFIG(debug, debug|release) {
    LIBS += -L../../ -lc++utilitiesd
} else {
    LIBS += -L../../ -lc++utilities
}
INCLUDEPATH += ../

RESOURCES += resources/qtutilsicons.qrc

# installs
target.path = $$(INSTALL_ROOT)/lib
INSTALLS += target
for(dir, $$list(aboutdialog enterpassworddialog models resources settingsdialog widgets)) {
    eval(inc_$${dir} = $${dir})
    inc_$${dir}.path = $$(INSTALL_ROOT)/include/$$projectname/$${dir}
    inc_$${dir}.files = $${dir}/*.h
    INSTALLS += inc_$${dir}
}


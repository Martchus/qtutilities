cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# prevent multiple inclusion
if (DEFINED QT_UTILITIES_ICONS)
    return()
endif ()
set(QT_UTILITIES_ICONS ON)

# icons needed by context menus (menus of Qt itself use non-symbolic icons)
set(QT_ICONS_CONTEXT_MENU
    edit-copy
    edit-clear
    edit-cut
    edit-delete
    edit-paste
    edit-redo
    edit-undo
    edit-select
    edit-select-all
    edit-copy-symbolic
    edit-clear-symbolic
    edit-cut-symbolic
    edit-delete-symbolic
    edit-paste-symbolic
    edit-redo-symbolic
    edit-undo-symbolic
    edit-select-symbolic
    edit-select-all-symbolic)

# icons needed by settings and paletteeditor
set(QT_ICONS_SETTINGS
    dialog-cancel-symbolic
    dialog-ok-apply-symbolic
    dialog-ok-symbolic
    edit-clear-symbolic
    preferences-desktop-icons
    preferences-desktop-locale
    system-search)

# icons needed by setup tools / updater
set(QT_ICONS_SETUP_TOOLS info-symbolic install-symbolic system-software-update-symbolic view-refresh-symbolic)

# icons needed by recentmanager
set(QT_ICONS_RECENTMANAGER document-open-recent-symbolic edit-clear-symbolic)

# icons needed by enterpassworddialog
set(QT_ICONS_PASSWORD_DIALOG go-next-symbolic window-close-symbolic)

# icons needed by various widgets
set(QT_ICONS_WIDGETS document-open-symbolic edit-clear-symbolic system-file-manager-symbolic system-run-symbolic)

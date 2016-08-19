#ifndef MISC_UTILS_IMPORT_PLUGIN_H
#define MISC_UTILS_IMPORT_PLUGIN_H

#ifdef QT_STATIC
# if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
#  include <QtPlugin>
#  ifdef PLATFORM_WINDOWS
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#  endif
#  ifdef SVG_SUPPORT
Q_IMPORT_PLUGIN(QSvgPlugin)
#  endif
# endif
#endif

#endif // MISC_UTILS_IMPORT_PLUGIN_H

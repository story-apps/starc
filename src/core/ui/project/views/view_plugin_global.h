#pragma once

#include <QtCore/qglobal.h>

#if defined(VIEW_PLUGIN)
#  define VIEW_PLUGIN_EXPORT Q_DECL_EXPORT
#else
#  define VIEW_PLUGIN_EXPORT Q_DECL_IMPORT
#endif

#pragma once

#include <QtCore/qglobal.h>

#if defined(CORE_PLUGIN)
#  define CORE_PLUGIN_EXPORT Q_DECL_EXPORT
#else
#  define CORE_PLUGIN_EXPORT Q_DECL_IMPORT
#endif

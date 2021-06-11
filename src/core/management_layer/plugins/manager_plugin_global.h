#pragma once

#include <QtCore/qglobal.h>

#if defined(MANAGER_PLUGIN)
#define MANAGER_PLUGIN_EXPORT Q_DECL_EXPORT
#else
#define MANAGER_PLUGIN_EXPORT Q_DECL_IMPORT
#endif

#pragma once

#include <QtCore/qglobal.h>

#if defined(CORE_LIBRARY)
#define CORE_LIBRARY_EXPORT Q_DECL_EXPORT
#else
#define CORE_LIBRARY_EXPORT Q_DECL_IMPORT
#endif

#pragma once

#include <QtGlobal>

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
#include "qt6/abstract_scroll_area_p.h"
#else
#include "qt5/abstract_scroll_area_p.h"
#endif

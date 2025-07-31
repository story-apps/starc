#pragma once

#include <QtGlobal>

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
#include "qt6/page_text_edit_scroll_bar.h"
#else
#include "qt5/page_text_edit_scroll_bar.h"
#endif

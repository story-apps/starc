#pragma once

#include <corelib_global.h>

class SpellCheckTextEdit;


class CORE_LIBRARY_EXPORT UiHelper
{
public:
    /**
     * @brief Настроить проверку орфографии для редактора текста
     */
    static void initSpellingFor(SpellCheckTextEdit* _edit);
    static void initSpellingFor(const QVector<SpellCheckTextEdit*>& _edits);
};

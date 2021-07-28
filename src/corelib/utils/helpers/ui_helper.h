#pragma once

#include <qnamespace.h>

#include <corelib_global.h>

class QScrollArea;
class QWidget;
class SpellCheckTextEdit;


class CORE_LIBRARY_EXPORT UiHelper
{
public:
    /**
     * @brief Настроить проверку орфографии для редактора текста
     */
    static void initSpellingFor(SpellCheckTextEdit* _edit);
    static void initSpellingFor(const QVector<SpellCheckTextEdit*>& _edits);

    /**
     * @brief Задать политику фокусирования для виджета и всех его детей
     */
    static void setFocusPolicyRecursively(QWidget* _widget, Qt::FocusPolicy _policy);

    /**
     * @brief Создать виджет скролареи с настроенной палитрой и скролбаром
     */
    static QScrollArea* createScrollArea(QWidget* _parent);
};

#pragma once

#include <qnamespace.h>

#include <corelib_global.h>

class BaseTextEdit;
class Button;
class QAbstractScrollArea;
class QHBoxLayout;
class QScrollArea;
class QVBoxLayout;
class QWidget;
class SpellCheckTextEdit;


class CORE_LIBRARY_EXPORT UiHelper
{
public:
    /**
     * @brief Роли кнопок
     */
    enum ButtonRole {
        DialogDefault,
        DialogAccept,
        DialogCritical,
    };
    /**
     * @brief Настроить цвет кнопки в зависимости от её роли
     */
    static void initColorsFor(Button* _button, ButtonRole _role);

    /**
     * @brief Создать и подготовить лейаут
     */
    static QHBoxLayout* makeHBoxLayout();
    static QVBoxLayout* makeVBoxLayout();

    /**
     * @brief Настроить проверку орфографии для редактора текста
     */
    static void initSpellingFor(SpellCheckTextEdit* _edit);
    static void initSpellingFor(const QVector<SpellCheckTextEdit*>& _edits);

    /**
     * @brief Настроить опции для редактора текста
     */
    static void initOptionsFor(BaseTextEdit* _edit);
    static void initOptionsFor(const QVector<BaseTextEdit*>& _edits);

    /**
     * @brief Задать политику фокусирования для виджета и всех его детей
     */
    static void setFocusPolicyRecursively(QWidget* _widget, Qt::FocusPolicy _policy);

    /**
     * @brief Создать виджет скролареи с настроенной палитрой и скролбаром
     */
    static QScrollArea* createScrollArea(QWidget* _parent, bool _withGridLayout = false);
    static QScrollArea* createScrollAreaWithGridLayout(QWidget* _parent);

    /**
     * @brief Настроить скроллинг для заданного виджета в заданных направлениях
     */
    static void setupScrolling(QAbstractScrollArea* _scrollArea,
                               bool _addHorizontalScrollBar = false);

    /**
     * @brief Показать тултип в текущем положении мышки
     */
    static void showToolTip(const QString& _text);
};

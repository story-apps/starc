#pragma once

#include <corelib/ui/widgets/text_edit/base/base_text_edit.h>

#include <QObject>

class Widget;

namespace Ui {
class SearchToolbar;
}

namespace BusinessLayer {

enum class TextParagraphType;

class CORE_LIBRARY_EXPORT SearchManager : public QObject
{
    Q_OBJECT

public:
    SearchManager(QWidget* _parent, BaseTextEdit* _textEdit);
    ~SearchManager() override;

    /**
     * @brief Вставить текст в поле поиска и выделить его
     */
    void activateSearhToolbar();

    /**
     * @brief Задать список типов блоков текста, в которых возможен поиск
     */
    void setSearchInBlockTypes(const QVector<QPair<QString, TextParagraphType>>& _blockTypes);

    /**
     * @brief Настроить режим редактирования
     */
    void setReadOnly(bool _readOnly);

    /**
     * @brief Получить панель поиска
     */
    Widget* toolbar() const;

signals:
    /**
     * @brief Запрос на скрытие панели поиска
     */
    void hideToolbarRequested();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

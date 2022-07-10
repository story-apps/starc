#pragma once

#include <QObject>

class Widget;

namespace Ui {
class SimpleTextEdit;
}

namespace BusinessLayer {

class SimpleTextSearchManager : public QObject
{
    Q_OBJECT

public:
    SimpleTextSearchManager(QWidget* _parent, Ui::SimpleTextEdit* _textEdit);
    ~SimpleTextSearchManager() override;

    /**
     * @brief Получить панель поиска
     */
    Widget* toolbar() const;

    /**
     * @brief Настроить режим редактирования
     */
    void setReadOnly(bool _readOnly);

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

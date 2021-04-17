#pragma once

#include <QObject>

class Widget;

namespace Ui {
class TextEdit;
}

namespace BusinessLayer {

class TextSearchManager : public QObject
{
    Q_OBJECT

public:
    TextSearchManager(QWidget* _parent, Ui::TextEdit* _textEdit);
    ~TextSearchManager() override;

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

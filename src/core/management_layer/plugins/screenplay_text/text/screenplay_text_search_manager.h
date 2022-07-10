#pragma once

#include <QObject>

class Widget;

namespace Ui {
class ScreenplayTextEdit;
}

namespace BusinessLayer {

class ScreenplayTextSearchManager : public QObject
{
    Q_OBJECT

public:
    ScreenplayTextSearchManager(QWidget* _parent, Ui::ScreenplayTextEdit* _textEdit);
    ~ScreenplayTextSearchManager() override;

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

#pragma once

#include <QObject>

class Widget;

namespace Ui {
class StageplayTextEdit;
}

namespace BusinessLayer {

class StageplayTextSearchManager : public QObject
{
    Q_OBJECT

public:
    StageplayTextSearchManager(QWidget* _parent, Ui::StageplayTextEdit* _textEdit);
    ~StageplayTextSearchManager() override;

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

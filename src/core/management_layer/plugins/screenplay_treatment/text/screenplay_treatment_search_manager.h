#pragma once

#include <QObject>

class Widget;

namespace Ui {
class ScreenplayTreatmentEdit;
}

namespace BusinessLayer {

class ScreenplayTreatmentSearchManager : public QObject
{
    Q_OBJECT

public:
    ScreenplayTreatmentSearchManager(QWidget* _parent, Ui::ScreenplayTreatmentEdit* _textEdit);
    ~ScreenplayTreatmentSearchManager() override;

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

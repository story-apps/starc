#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

class BeatNameWidget : public Widget
{
    Q_OBJECT

public:
    explicit BeatNameWidget(QWidget* _parent = nullptr);
    ~BeatNameWidget() override;

    /**
     * @brief Задать текст бита
     */
    void setBeatName(const QString& _name);
    void clearBeatName();

signals:
    /**
     * @brief Пользователь кликнул на кнопке вставки текста бита
     */
    void pasteBeatNamePressed(const QString& _name);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

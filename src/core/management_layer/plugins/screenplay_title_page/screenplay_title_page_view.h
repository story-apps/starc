#pragma once

#include <ui/widgets/widget/widget.h>

namespace BusinessLayer {
class TextModel;
}

namespace Ui {

/**
 * @brief Представление редактора титульной страницы сценария
 */
class ScreenplayTitlePageView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTitlePageView(QWidget* _parent = nullptr);
    ~ScreenplayTitlePageView() override;

    /**
     * @brief Настроить редактор в соответствии с параметрами заданными в настройках
     */
    void reconfigure(const QStringList& _changedSettingsKeys);

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadViewSettings();
    void saveViewSettings();

    /**
     * @brief Установить модель текста
     */
    void setModel(BusinessLayer::TextModel* _model);

    /**
     * @brief Позиция курсора
     */
    int cursorPosition() const;
    void setCursorPosition(int _position);

protected:
    /**
     * @brief Переопределяем для корректировки положения тулбара действий над проектами
     */
    void resizeEvent(QResizeEvent* _event) override;

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

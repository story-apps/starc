#pragma once

#include <ui/widgets/search_tool_bar/search_toolbar.h>


namespace Ui {

/**
 * @brief Виджет панели поиска с попапом для выбора типа текста
 */
class CORE_LIBRARY_EXPORT SearchInTypeToolbar : public Ui::SearchToolbar
{
    Q_OBJECT
public:
    explicit SearchInTypeToolbar(QWidget* _parent);
    ~SearchInTypeToolbar() override;

    /**
     * @brief В каком блоке искать
     */
    int searchInType() const;

protected:
    /**
     * @brief Переопределяем для более красивой работы с выпадающими списками
     */
    bool canAnimateHoverOut() const override;

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

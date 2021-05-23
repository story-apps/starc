#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>

class QAbstractItemModel;


namespace Ui
{

/**
 * @brief Панель инструментов редактора текста
 */
class ScreenplayTitlePageEditToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ScreenplayTitlePageEditToolbar(QWidget* _parent = nullptr);
    ~ScreenplayTitlePageEditToolbar() override;

    /**
     * @brief Задать модель выпадающего списка типов абзацев
     */
    void setParagraphTypesModel(QAbstractItemModel* _model);

    /**
     * @brief Задать название типа текущего параграфа
     */
    void setCurrentParagraphType(const QModelIndex& _index);

signals:
    void undoPressed();
    void redoPressed();
    void paragraphTypeChanged(const QModelIndex& _index);

protected:
    /**
     * @brief Скрываем попап, когда фокус ушёл из виджета
     */
    void focusOutEvent(QFocusEvent* _event) override;

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

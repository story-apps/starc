#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Панель инструментов редактора текста
 */
class TitlePageEditToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit TitlePageEditToolbar(QWidget* _parent = nullptr);
    ~TitlePageEditToolbar() override;

    /**
     * @brief Задать текущий шрифт у курсора
     */
    void setCurrentFont(const QFont& _font);

signals:
    void undoPressed();
    void redoPressed();
    void fontChanged(const QFont& _font);
    void restoreTitlePagePressed();

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

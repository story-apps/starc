#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Панель быстрого форматирования сценария
 */
class ComicBookTextFastFormatWidget : public Widget
{
    Q_OBJECT

public:
    explicit ComicBookTextFastFormatWidget(QWidget* _parent = nullptr);
    ~ComicBookTextFastFormatWidget() override;

    /**
     * @brief Задать модель списка типов абзацев
     */
    void setParagraphTypesModel(QAbstractItemModel* _model);

    /**
     * @brief Задать название типа текущего параграфа
     */
    void setCurrentParagraphType(const QModelIndex& _index);

signals:
    /**
     * @brief Пользователь хочет сменить тип текущего абзаца
     */
    void paragraphTypeChanged(const QModelIndex& _index);

protected:
    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

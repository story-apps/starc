#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Панель инструментов редактора текста
 */
class SimpleTextEditToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit SimpleTextEditToolbar(QWidget* _parent = nullptr);
    ~SimpleTextEditToolbar() override;

    /**
     * @brief Настроить режим редактирования
     */
    void setReadOnly(bool _readOnly);

    /**
     * @brief Задать модель выпадающего списка типов абзацев
     */
    void setParagraphTypesModel(QAbstractItemModel* _model);

    /**
     * @brief Задать название типа текущего параграфа
     */
    void setCurrentParagraphType(const QModelIndex& _index);

    /**
     * @brief Включена ли панель быстрого форматирования
     */
    bool isFastFormatPanelVisible() const;
    void setFastFormatPanelVisible(bool _visible);

    /**
     * @brief Иконка поиска
     */
    QString searchIcon() const;

    /**
     * @brief Позиция иконки поиска
     */
    QPointF searchIconPosition() const;

    /**
     * @brief Включён ли режим рецензирования
     */
    bool isCommentsModeEnabled() const;
    void setCommentsModeEnabled(bool _enabled);

    /**
     * @brief Включён ли режим ИИ помощника
     */
    bool isAiAssistantEnabled() const;
    void setAiAssistantEnabled(bool _enabled);

    /**
     * @brief Включена ли опция изолирования элементов на экране
     */
    bool isItemIsolationEnabled() const;
    void setItemIsolationEnabled(bool _enabled);

    /**
     * @brief Задать список дополнительных опций
     */
    void setOptions(const QVector<QAction*>& _options);

signals:
    void undoPressed();
    void redoPressed();
    void paragraphTypeChanged(const QModelIndex& _index);
    void fastFormatPanelVisibleChanged(bool _visible);
    void searchPressed();
    void commentsModeEnabledChanged(bool _enabled);
    void aiAssistantEnabledChanged(bool _enabled);
    void itemIsolationEnabledChanged(bool _enabled);

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

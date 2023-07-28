#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

class CreateDraftDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateDraftDialog(QWidget* _parent = nullptr);
    ~CreateDraftDialog() override;

    /**
     * @brief Задать список версий из которых можно создать новую
     */
    void setVersions(const QStringList& _versions, int _selectVersionIndex);

    /**
     * @brief Редактировать версию с заданными параметрами
     */
    void edit(const QString& _name, const QColor& _color, bool _readOnly);

signals:
    /**
     * @brief Пользователь нажал кнопку создания новой версии/сохранения редактируемой
     */
    void savePressed(const QString& _versionName, const QColor& _color, int _sourceVersionIndex,
                     bool _readOnly);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

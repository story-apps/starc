#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

/**
 * @brief Диалог создания/изменения команд
 */
class TeamDialog : public AbstractDialog
{
    Q_OBJECT

public:
    /**
     * @brief Способ отображения диалога
     */
    enum class DialogType {
        CreateNew,
        Edit,
    };

public:
    explicit TeamDialog(QWidget* _parent = nullptr);
    ~TeamDialog() override;

    /**
     * @brief Задать тип диалога, по умолчанию тип равен CreateNew
     */
    void setDialogType(DialogType _type);

    /**
     * @brief Имя
     */
    QString teamName() const;
    void setTeamName(const QString& _name);
    void setTeamNameError(const QString& _error);

    /**
     * @brief Краткое описание
     */
    QString teamDescription() const;
    void setteamDescription(const QString& _description);

    /**
     * @brief Фотка
     */
    QPixmap teamAvatar() const;
    void setTeamAvatar(const QPixmap& _photo);

signals:
    /**
     * @brief Пользователь нажал кнопку сохранения
     */
    void savePressed();

protected:
    /**
     * @brief При именении размера фотки, корректируем высоту
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

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

#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

/**
 * @brief Диалог отправки сообщения об ошибке в работе приложения
 */
class CrashReportDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CrashReportDialog(QWidget* _parent = nullptr);
    ~CrashReportDialog() override;

    /**
     * @brief Частота падений
     */
    QString frequency() const;

    /**
     * @brief На каких проектах падает
     */
    QString crashSource() const;

    /**
     * @brief Подробности о падении
     */
    QString crashDetails() const;

    /**
     * @brief Контактный имейл
     */
    QString contactEmail() const;
    void setContactEmail(const QString& _email);

signals:
    /**
     * @brief Пользователь нажал кнопку отправки отчёта об ошибке
     */
    void sendReportPressed();

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

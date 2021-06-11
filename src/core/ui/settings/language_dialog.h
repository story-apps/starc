#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

#include <QLocale>


namespace Ui {

class LanguageDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit LanguageDialog(QWidget* _parent = nullptr);
    ~LanguageDialog() override;

    /**
     * @brief Задать текущий язык приложения
     */
    void setCurrentLanguage(QLocale::Language _language);

signals:
    /**
     * @brief Пользователь изменил язык
     */
    void languageChanged(QLocale::Language _language);

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

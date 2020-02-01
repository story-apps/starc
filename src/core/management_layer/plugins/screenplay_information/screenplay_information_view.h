#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

class ScreenplayInformationView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayInformationView(QWidget* _parent = nullptr);
    ~ScreenplayInformationView() override;

    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);

    void setLogline(const QString& _logline);
    Q_SIGNAL void loglineChanged(const QString& _logline);

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

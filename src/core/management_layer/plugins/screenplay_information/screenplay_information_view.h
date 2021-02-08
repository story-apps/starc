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

    void setTagline(const QString& _tagline);
    Q_SIGNAL void taglineChanged(const QString& _logline);

    void setLogline(const QString& _logline);
    Q_SIGNAL void loglineChanged(const QString& _logline);

    void setTitlePageVisible(bool _visible);
    Q_SIGNAL void titlePageVisibleChanged(bool _visible);

    void setSynopsisVisible(bool _visible);
    Q_SIGNAL void synopsisVisibleChanged(bool _visible);

    void setTreatmentVisible(bool _visible);
    Q_SIGNAL void treatmentVisibleChanged(bool _visible);

    void setScreenplayTextVisible(bool _visible);
    Q_SIGNAL void screenplayTextVisibleChanged(bool _visible);

    void setScreenplayStatisticsVisible(bool _visible);
    Q_SIGNAL void screenplayStatisticsVisibleChanged(bool _visible);

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

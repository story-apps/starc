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

    void setHeader(const QString& _header);
    Q_SIGNAL void headerChanged(const QString& _header);

    void setFooter(const QString& _footer);
    Q_SIGNAL void footerChanged(const QString& _footer);

    void setScenesNumbersPrefix(const QString& _prefix);
    Q_SIGNAL void scenesNumbersPrefixChanged(const QString& _prefix);

    void setScenesNumbersingStartAt(int _startNumber);
    Q_SIGNAL void scenesNumberingStartAtChanged(int _startNumber);

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

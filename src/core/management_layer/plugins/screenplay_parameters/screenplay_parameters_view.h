#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

class ScreenplayParametersView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayParametersView(QWidget* _parent = nullptr);
    ~ScreenplayParametersView() override;

    void setHeader(const QString& _header);
    Q_SIGNAL void headerChanged(const QString& _header);

    void setPrintHeaderOnTitlePage(bool _print);
    Q_SIGNAL void printHeaderOnTitlePageChanged(bool _print);

    void setFooter(const QString& _footer);
    Q_SIGNAL void footerChanged(const QString& _footer);

    void setPrintFooterOnTitlePage(bool _print);
    Q_SIGNAL void printFooterOnTitlePageChanged(bool _print);

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

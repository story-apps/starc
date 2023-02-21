#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/widgets/widget/widget.h>


namespace Ui {


/**
 * @brief Представление информации о пьесе
 */
class NovelInformationView : public Widget, public IDocumentView
{
    Q_OBJECT

public:
    explicit NovelInformationView(QWidget* _parent = nullptr);
    ~NovelInformationView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    /** @{ */
    QWidget* asQWidget() override;
    void setEditingMode(ManagementLayer::DocumentEditingMode _mode) override;
    /** @} */

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

    void setOutlineVisible(bool _visible);
    Q_SIGNAL void outlineVisibleChanged(bool _visible);

    void setNovelTextVisible(bool _visible);
    Q_SIGNAL void novelTextVisibleChanged(bool _visible);

    void setNovelStatisticsVisible(bool _visible);
    Q_SIGNAL void novelStatisticsVisibleChanged(bool _visible);

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

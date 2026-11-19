#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/widgets/widget/widget.h>


namespace BusinessLayer {
struct ChronometerOptions;
}

namespace Ui {

class ProjectParametersView : public Widget, public IDocumentView
{
    Q_OBJECT

public:
    explicit ProjectParametersView(QWidget* _parent = nullptr);
    ~ProjectParametersView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    /** @{ */
    QWidget* asQWidget() override;
    void setEditingMode(ManagementLayer::DocumentEditingMode _mode) override;
    /** @} */

    void setOverrideScreenplaySettings(bool _override);
    Q_SIGNAL void overrideScreenplaySettingsChanged(bool _override);

    void setScreenplayEditorTemplate(const QString& _templateId);
    Q_SIGNAL void screenplayEditorTemplateChanged(const QString& _templateId);

    void setScreenplayEditorShowSceneNumbers(bool _show);
    Q_SIGNAL void screenplayEditorShowSceneNumbersChanged(bool _show);

    void setScreenplayEditorShowSceneNumbersOnLeft(bool _show);
    Q_SIGNAL void screenplayEditorShowSceneNumbersOnLeftChanged(bool _show);

    void setScreenplayEditorShowSceneNumbersOnRight(bool _show);
    Q_SIGNAL void screenplayEditorShowSceneNumbersOnRightChanged(bool _show);

    void setScreenplayEditorShowDialoguesNumbers(bool _show);
    Q_SIGNAL void screenplayEditorShowDialoguesNumbersChanged(bool _show);

    void setScreenplayEditorCorrectSceneCharacters(bool _correct);
    Q_SIGNAL void screenplayEditorCorrectSceneCharactersChanged(bool _correct);

    void setScreenplayEditorCorrectTextOnPageBreaks(bool _correct);
    Q_SIGNAL void screenplayEditorCorrectTextOnPageBreaksChanged(bool _correct);

    void setScreenplayDurationOptions(const BusinessLayer::ChronometerOptions& _options);
    Q_SIGNAL void screenplayDurationOptionsChanged(
        const BusinessLayer::ChronometerOptions& _options);

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

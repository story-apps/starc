#include "screenplay_text_structure_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/completer/completer_text_edit.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_edit/spell_check/spell_checker.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>

#include <QScrollArea>
#include <QStringListModel>
#include <QVBoxLayout>

#include <QDebug>
namespace Ui
{

class ScreenplayTextStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    CompleterTextEdit* screenplayText = nullptr;
    ScalableWrapper* scalableWrapper = nullptr;
};

ScreenplayTextStructureView::Implementation::Implementation(QWidget* _parent)
    : screenplayText(new CompleterTextEdit(_parent)),
      scalableWrapper(new ScalableWrapper(screenplayText, _parent))
{
    screenplayText->setVerticalScrollBar(new ScrollBar);
    screenplayText->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->setVerticalScrollBar(new ScrollBar);
    scalableWrapper->setHorizontalScrollBar(new ScrollBar);
    scalableWrapper->initScrollBarsSyncing();

    screenplayText->setFrameShape(QFrame::NoFrame);
    screenplayText->setUsePageMode(true);
    screenplayText->setPageFormat(QPageSize::A6);
    screenplayText->setPageMargins({30, 20, 10, 20});
    screenplayText->setPageNumbersAlignment(Qt::AlignBottom | Qt::AlignRight);
    screenplayText->setHeader("Header text");
    screenplayText->setFooter("Footer text");
    screenplayText->setCursorWidth(4);
    screenplayText->setUseSpellChecker(true);
    screenplayText->setSpellCheckLanguage(SpellCheckerLanguage::EnglishUS);

    connect(screenplayText, &CompleterTextEdit::textChanged, [this] {
        if (screenplayText->toPlainText().isEmpty()) {
            return;
        }

        static QStringListModel* model = new QStringListModel({"АНТОН", "АДМИРАЛ", "АДМИНИСТРАТОР", "АПОСТОЛ", "АДЛЕН", "АМИР"}, screenplayText);
        screenplayText->complete(model, screenplayText->toPlainText());
    });
}


// ****


ScreenplayTextStructureView::ScreenplayTextStructureView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->scalableWrapper);
    setLayout(layout);


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTextStructureView::~ScreenplayTextStructureView() = default;

void ScreenplayTextStructureView::updateTranslations()
{

}

void ScreenplayTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->screenplayText->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().background());
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onBackground());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().secondary());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onSecondary());
    d->scalableWrapper->setPalette(palette);
    d->screenplayText->setPalette(palette);
    d->screenplayText->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->screenplayText->completer()->setBackgroundColor(Ui::DesignSystem::color().background());
}

} // namespace Ui

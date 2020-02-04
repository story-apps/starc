#include "text_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/text_edit/spell_check/spell_checker.h>
#include <ui/widgets/text_edit/completer/completer_text_edit.h>
#include <ui/widgets/text_edit/completer/completer.h>

#include <QGridLayout>
#include <QScrollArea>
#include <QStringListModel>


namespace Ui
{

class TextView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* screenplayInfo = nullptr;
    QGridLayout* screenplayInfoLayout = nullptr;
    TextField* documentName = nullptr;
    CompleterTextEdit* documentText = nullptr;
};

TextView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      screenplayInfo(new Card(_parent)),
      screenplayInfoLayout(new QGridLayout),
      documentName(new TextField(screenplayInfo)),
      documentText(new CompleterTextEdit(screenplayInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setVerticalScrollBar(new ScrollBar);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    screenplayInfoLayout->setContentsMargins({});
    screenplayInfoLayout->setSpacing(0);
    screenplayInfoLayout->setRowMinimumHeight(0, 1); // добавляем пустую строку сверху
    screenplayInfoLayout->addWidget(documentName, 1, 0);
//    screenplayInfoLayout->addWidget(documentText, 2, 0);
    screenplayInfoLayout->setRowMinimumHeight(3, 1); // добавляем пустую строку внизу
    screenplayInfoLayout->setColumnStretch(0, 1);
    screenplayInfo->setLayoutReimpl(screenplayInfoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(screenplayInfo);
    layout->addWidget(documentText, 1);
    contentWidget->setLayout(layout);

    documentText->setFrameShape(QFrame::NoFrame);
    documentText->setUsePageMode(true);
    documentText->setPageFormat(QPageSize::A6);
    documentText->setPageMargins({10, 10, 10, 10});
    documentText->setPageNumbersAlignment(Qt::AlignBottom | Qt::AlignRight);
    documentText->setHeader("Header text");
    documentText->setFooter("Footer text");
    documentText->setCursorWidth(4);
    documentText->setUseSpellChecker(true);
    documentText->setSpellCheckLanguage(SpellCheckerLanguage::EnglishUS);

    connect(documentText, &CompleterTextEdit::textChanged, [this] {
        static QStringListModel* model = new QStringListModel({"АНТОН", "АДМИРАЛ", "АДМИНИСТРАТОР", "АПОСТОЛ", "АДЛЕН", "АМИР"}, documentText);
        documentText->complete(model, documentText->toPlainText());
    });
}


// ****


TextView::TextView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->documentName, &TextField::textChanged, this, [this] {
        emit nameChanged(d->documentName->text());
    });
    connect(d->documentText, &PageTextEdit::textChanged, this, [this] {
        emit textChanged(d->documentText->toHtml());
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

TextView::~TextView() = default;

void TextView::setName(const QString& _name)
{
    d->documentName->setText(_name);
}

void TextView::setText(const QString& _text)
{
    if (_text == d->documentText->toHtml()) {
        return;
    }

    d->documentText->setHtml(_text);
}

void TextView::updateTranslations()
{
    d->documentName->setLabel(tr("Screenplay name"));
//    d->documentText->setLabel(tr("Header"));
}

void TextView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().topContentMargin(),
                          Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24())
                .toMargins());

    d->screenplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->documentName }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().background());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->screenplayInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->screenplayInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->screenplayInfoLayout->setRowMinimumHeight(3, static_cast<int>(Ui::DesignSystem::layout().px16()));


    d->documentText->setPageSpacing(Ui::DesignSystem::layout().px24());
    QPalette palette;
    palette.setColor(QPalette::Base, Ui::DesignSystem::color().background());
    palette.setColor(QPalette::Window, Ui::DesignSystem::color().surface());
    palette.setColor(QPalette::Text, Ui::DesignSystem::color().onBackground());
    palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().secondary());
    palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onSecondary());
    d->documentText->setPalette(palette);
    d->documentText->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentText->completer()->setBackgroundColor(Ui::DesignSystem::color().background());
}

} // namespace Ui

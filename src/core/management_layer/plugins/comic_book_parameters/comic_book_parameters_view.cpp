#include "comic_book_parameters_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui {

class ComicBookParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* comicBookInfo = nullptr;
    QGridLayout* comicBookInfoLayout = nullptr;
    TextField* comicBookHeader = nullptr;
    CheckBox* comicBookPrintHeaderOnTitlePage = nullptr;
    TextField* comicBookFooter = nullptr;
    CheckBox* comicBookPrintFooterOnTitlePage = nullptr;
};

ComicBookParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , comicBookInfo(new Card(_parent))
    , comicBookInfoLayout(new QGridLayout)
    , comicBookHeader(new TextField(comicBookInfo))
    , comicBookPrintHeaderOnTitlePage(new CheckBox(comicBookInfo))
    , comicBookFooter(new TextField(comicBookInfo))
    , comicBookPrintFooterOnTitlePage(new CheckBox(comicBookInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    comicBookInfoLayout->setContentsMargins({});
    comicBookInfoLayout->setSpacing(0);
    int row = 0;
    comicBookInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    comicBookInfoLayout->addWidget(comicBookHeader, row++, 0);
    comicBookInfoLayout->addWidget(comicBookPrintHeaderOnTitlePage, row++, 0);
    comicBookInfoLayout->addWidget(comicBookFooter, row++, 0);
    comicBookInfoLayout->addWidget(comicBookPrintFooterOnTitlePage, row++, 0);
    comicBookInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    comicBookInfoLayout->setColumnStretch(0, 1);
    comicBookInfo->setLayoutReimpl(comicBookInfoLayout);

    comicBookHeader->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    comicBookFooter->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(comicBookInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ComicBookParametersView::ComicBookParametersView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->comicBookHeader, &TextField::textChanged, this,
            [this] { emit headerChanged(d->comicBookHeader->text()); });
    connect(d->comicBookPrintHeaderOnTitlePage, &CheckBox::checkedChanged, this,
            &ComicBookParametersView::printHeaderOnTitlePageChanged);
    connect(d->comicBookFooter, &TextField::textChanged, this,
            [this] { emit footerChanged(d->comicBookFooter->text()); });
    connect(d->comicBookPrintFooterOnTitlePage, &CheckBox::checkedChanged, this,
            &ComicBookParametersView::printFooterOnTitlePageChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ComicBookParametersView::~ComicBookParametersView() = default;


void ComicBookParametersView::setHeader(const QString& _header)
{
    if (d->comicBookHeader->text() == _header) {
        return;
    }

    d->comicBookHeader->setText(_header);
}

void ComicBookParametersView::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->comicBookPrintHeaderOnTitlePage->isChecked() == _print) {
        return;
    }

    d->comicBookPrintHeaderOnTitlePage->setChecked(_print);
}

void ComicBookParametersView::setFooter(const QString& _footer)
{
    if (d->comicBookFooter->text() == _footer) {
        return;
    }

    d->comicBookFooter->setText(_footer);
}

void ComicBookParametersView::setPrintFooterOnTitlePage(bool _print)
{
    if (d->comicBookPrintFooterOnTitlePage->isChecked() == _print) {
        return;
    }

    d->comicBookPrintFooterOnTitlePage->setChecked(_print);
}

void ComicBookParametersView::updateTranslations()
{
    d->comicBookHeader->setLabel(tr("Header"));
    d->comicBookPrintHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->comicBookFooter->setLabel(tr("Footer"));
    d->comicBookPrintFooterOnTitlePage->setText(tr("Print footer on title page"));
}

void ComicBookParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->comicBookInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->comicBookHeader, d->comicBookFooter }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox :
         { d->comicBookPrintHeaderOnTitlePage, d->comicBookPrintFooterOnTitlePage }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->comicBookInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->comicBookInfoLayout->setRowMinimumHeight(
        0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->comicBookInfoLayout->setRowMinimumHeight(
        7, static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui

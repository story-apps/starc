#include "comic_book_information_view.h"

#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/logline_generator/logline_generator_dialog.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui {

class ComicBookInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* comicBookInfo = nullptr;
    QGridLayout* comicBookInfoLayout = nullptr;
    TextField* comicBookName = nullptr;
    TextField* comicBookTagline = nullptr;
    TextField* comicBookLogline = nullptr;
    CheckBox* titlePageVisiblity = nullptr;
    CheckBox* synopsisVisiblity = nullptr;
    CheckBox* comicBookTextVisiblity = nullptr;
    CheckBox* comicBookStatisticsVisiblity = nullptr;
};

ComicBookInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , comicBookInfo(new Card(_parent))
    , comicBookInfoLayout(new QGridLayout)
    , comicBookName(new TextField(comicBookInfo))
    , comicBookTagline(new TextField(comicBookInfo))
    , comicBookLogline(new TextField(comicBookInfo))
    , titlePageVisiblity(new CheckBox(comicBookInfo))
    , synopsisVisiblity(new CheckBox(comicBookInfo))
    , comicBookTextVisiblity(new CheckBox(comicBookInfo))
    , comicBookStatisticsVisiblity(new CheckBox(comicBookInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    comicBookName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    UiHelper::initSpellingFor({ comicBookTagline, comicBookLogline });
    comicBookLogline->setEnterMakesNewLine(true);
    comicBookLogline->setTrailingIcon(u8"\U000F1353");

    comicBookInfoLayout->setContentsMargins({});
    comicBookInfoLayout->setSpacing(0);
    int row = 0;
    comicBookInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    comicBookInfoLayout->addWidget(comicBookName, row++, 0);
    comicBookInfoLayout->addWidget(comicBookTagline, row++, 0);
    comicBookInfoLayout->addWidget(comicBookLogline, row++, 0);
    comicBookInfoLayout->addWidget(titlePageVisiblity, row++, 0);
    comicBookInfoLayout->addWidget(synopsisVisiblity, row++, 0);
    comicBookInfoLayout->addWidget(comicBookTextVisiblity, row++, 0);
    comicBookInfoLayout->addWidget(comicBookStatisticsVisiblity, row++, 0);
    comicBookInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    comicBookInfoLayout->setColumnStretch(0, 1);
    comicBookInfo->setContentLayout(comicBookInfoLayout);

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


ComicBookInformationView::ComicBookInformationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->comicBookName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->comicBookName->text()); });
    connect(d->comicBookTagline, &TextField::textChanged, this,
            [this] { emit taglineChanged(d->comicBookTagline->text()); });
    connect(d->comicBookLogline, &TextField::textChanged, this,
            [this] { emit loglineChanged(d->comicBookLogline->text()); });
    connect(d->comicBookLogline, &TextField::trailingIconPressed, this, [this] {
        auto dialog = new LoglineGeneratorDialog(topLevelWidget());
        connect(dialog, &LoglineGeneratorDialog::donePressed, this, [this, dialog] {
            d->comicBookLogline->setText(dialog->logline());
            dialog->hideDialog();
        });
        connect(dialog, &LoglineGeneratorDialog::disappeared, dialog,
                &LoglineGeneratorDialog::deleteLater);

        dialog->showDialog();
    });
    connect(d->titlePageVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::titlePageVisibleChanged);
    connect(d->synopsisVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::synopsisVisibleChanged);
    connect(d->comicBookTextVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::comicBookTextVisibleChanged);
    connect(d->comicBookStatisticsVisiblity, &CheckBox::checkedChanged, this,
            &ComicBookInformationView::comicBookStatisticsVisibleChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ComicBookInformationView::~ComicBookInformationView() = default;

QWidget* ComicBookInformationView::asQWidget()
{
    return this;
}

void ComicBookInformationView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->comicBookName->setReadOnly(readOnly);
    d->comicBookTagline->setReadOnly(readOnly);
    d->comicBookLogline->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->titlePageVisiblity->setEnabled(enabled);
    d->synopsisVisiblity->setEnabled(enabled);
    d->comicBookTextVisiblity->setEnabled(enabled);
    d->comicBookStatisticsVisiblity->setEnabled(enabled);
}

void ComicBookInformationView::setName(const QString& _name)
{
    if (d->comicBookName->text() == _name) {
        return;
    }

    d->comicBookName->setText(_name);
}

void ComicBookInformationView::setTagline(const QString& _tagline)
{
    if (d->comicBookTagline->text() == _tagline) {
        return;
    }

    d->comicBookTagline->setText(_tagline);
}

void ComicBookInformationView::setLogline(const QString& _logline)
{
    if (d->comicBookLogline->text() == _logline) {
        return;
    }

    d->comicBookLogline->setText(_logline);
}

void ComicBookInformationView::setTitlePageVisible(bool _visible)
{
    d->titlePageVisiblity->setChecked(_visible);
}

void ComicBookInformationView::setSynopsisVisible(bool _visible)
{
    d->synopsisVisiblity->setChecked(_visible);
}

void ComicBookInformationView::setComicBookTextVisible(bool _visible)
{
    d->comicBookTextVisiblity->setChecked(_visible);
}

void ComicBookInformationView::setComicBookStatisticsVisible(bool _visible)
{
    d->comicBookStatisticsVisiblity->setChecked(_visible);
}

void ComicBookInformationView::updateTranslations()
{
    d->comicBookName->setLabel(tr("Comic book name"));
    d->comicBookTagline->setLabel(tr("Tagline"));
    d->comicBookLogline->setLabel(tr("Logline"));
    d->comicBookLogline->setTrailingIconToolTip(tr("Generate logline"));
    d->titlePageVisiblity->setText(tr("Title page"));
    d->synopsisVisiblity->setText(tr("Synopsis"));
    d->comicBookTextVisiblity->setText(tr("Script"));
    d->comicBookStatisticsVisiblity->setText(tr("Statistics"));
}

void ComicBookInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->comicBookInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->comicBookName, d->comicBookTagline, d->comicBookLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : { d->titlePageVisiblity, d->synopsisVisiblity, d->comicBookTextVisiblity,
                           d->comicBookStatisticsVisiblity }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->comicBookInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->comicBookInfoLayout->setRowMinimumHeight(
        0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->comicBookInfoLayout->setRowMinimumHeight(
        d->comicBookInfoLayout->rowCount() - 1,
        static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui

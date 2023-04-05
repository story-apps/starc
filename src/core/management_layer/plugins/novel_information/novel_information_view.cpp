#include "novel_information_view.h"

#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/logline_generator/logline_generator_dialog.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QScrollArea>


namespace Ui {

class NovelInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* novelInfo = nullptr;
    QGridLayout* novelInfoLayout = nullptr;
    TextField* novelName = nullptr;
    TextField* novelTagline = nullptr;
    TextField* novelLogline = nullptr;
    CheckBox* titlePageVisiblity = nullptr;
    CheckBox* synopsisVisiblity = nullptr;
    CheckBox* outlineVisiblity = nullptr;
    CheckBox* novelTextVisiblity = nullptr;
    CheckBox* novelStatisticsVisiblity = nullptr;
};

NovelInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , novelInfo(new Card(_parent))
    , novelInfoLayout(new QGridLayout)
    , novelName(new TextField(novelInfo))
    , novelTagline(new TextField(novelInfo))
    , novelLogline(new TextField(novelInfo))
    , titlePageVisiblity(new CheckBox(novelInfo))
    , synopsisVisiblity(new CheckBox(novelInfo))
    , outlineVisiblity(new CheckBox(novelInfo))
    , novelTextVisiblity(new CheckBox(novelInfo))
    , novelStatisticsVisiblity(new CheckBox(novelInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    novelName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    UiHelper::initSpellingFor({ novelTagline, novelLogline });
    novelLogline->setEnterMakesNewLine(true);
    novelLogline->setTrailingIcon(u8"\U000F1353");

    novelInfoLayout->setContentsMargins({});
    novelInfoLayout->setSpacing(0);
    int row = 0;
    novelInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    novelInfoLayout->addWidget(novelName, row++, 0);
    novelInfoLayout->addWidget(novelTagline, row++, 0);
    novelInfoLayout->addWidget(novelLogline, row++, 0);
    novelInfoLayout->addWidget(titlePageVisiblity, row++, 0);
    novelInfoLayout->addWidget(synopsisVisiblity, row++, 0);
    novelInfoLayout->addWidget(outlineVisiblity, row++, 0);
    novelInfoLayout->addWidget(novelTextVisiblity, row++, 0);
    novelInfoLayout->addWidget(novelStatisticsVisiblity, row++, 0);
    novelInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    novelInfoLayout->setColumnStretch(0, 1);
    novelInfo->setContentLayout(novelInfoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(novelInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


NovelInformationView::NovelInformationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->novelName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->novelName->text()); });
    connect(d->novelTagline, &TextField::textChanged, this,
            [this] { emit taglineChanged(d->novelTagline->text()); });
    connect(d->novelLogline, &TextField::textChanged, this, [this] {
        //
        // Покажем статистику по количеству знаков
        //
        const auto wordsCount = TextHelper::wordsCount(d->novelLogline->text());
        QString loglineInfo;
        if (wordsCount > 0) {
            if (wordsCount <= 25) {
                loglineInfo = tr("Perfect logline length");
            } else if (wordsCount <= 30) {
                loglineInfo = tr("Good logline length");
            } else {
                loglineInfo = tr("Recommended logline length is 30 words");
            }
            loglineInfo.append(QString(" (%1)").arg(tr("%n word(s)", 0, wordsCount)));
        }
        d->novelLogline->setHelper(loglineInfo);

        //
        // Уведомим клиентов об изменении
        //
        emit loglineChanged(d->novelLogline->text());
    });
    connect(d->novelLogline, &TextField::trailingIconPressed, this, [this] {
        auto dialog = new LoglineGeneratorDialog(topLevelWidget());
        connect(dialog, &LoglineGeneratorDialog::donePressed, this, [this, dialog] {
            d->novelLogline->setText(dialog->logline());
            dialog->hideDialog();
        });
        connect(dialog, &LoglineGeneratorDialog::disappeared, dialog,
                &LoglineGeneratorDialog::deleteLater);

        dialog->showDialog();
    });
    connect(d->titlePageVisiblity, &CheckBox::checkedChanged, this,
            &NovelInformationView::titlePageVisibleChanged);
    connect(d->synopsisVisiblity, &CheckBox::checkedChanged, this,
            &NovelInformationView::synopsisVisibleChanged);
    connect(d->outlineVisiblity, &CheckBox::checkedChanged, this,
            &NovelInformationView::outlineVisibleChanged);
    connect(d->novelTextVisiblity, &CheckBox::checkedChanged, this,
            &NovelInformationView::novelTextVisibleChanged);
    connect(d->novelStatisticsVisiblity, &CheckBox::checkedChanged, this,
            &NovelInformationView::novelStatisticsVisibleChanged);
}

NovelInformationView::~NovelInformationView() = default;

QWidget* NovelInformationView::asQWidget()
{
    return this;
}

void NovelInformationView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->novelName->setReadOnly(readOnly);
    d->novelTagline->setReadOnly(readOnly);
    d->novelLogline->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->titlePageVisiblity->setEnabled(enabled);
    d->synopsisVisiblity->setEnabled(enabled);
    d->outlineVisiblity->setEnabled(enabled);
    d->novelTextVisiblity->setEnabled(enabled);
    d->novelStatisticsVisiblity->setEnabled(enabled);
}

void NovelInformationView::setName(const QString& _name)
{
    if (d->novelName->text() == _name) {
        return;
    }

    d->novelName->setText(_name);
}

void NovelInformationView::setTagline(const QString& _tagline)
{
    if (d->novelTagline->text() == _tagline) {
        return;
    }

    d->novelTagline->setText(_tagline);
}

void NovelInformationView::setLogline(const QString& _logline)
{
    if (d->novelLogline->text() == _logline) {
        return;
    }

    d->novelLogline->setText(_logline);
}

void NovelInformationView::setTitlePageVisible(bool _visible)
{
    d->titlePageVisiblity->setChecked(_visible);
}

void NovelInformationView::setSynopsisVisible(bool _visible)
{
    d->synopsisVisiblity->setChecked(_visible);
}

void NovelInformationView::setOutlineVisible(bool _visible)
{
    d->outlineVisiblity->setChecked(_visible);
}

void NovelInformationView::setNovelTextVisible(bool _visible)
{
    d->novelTextVisiblity->setChecked(_visible);
}

void NovelInformationView::setNovelStatisticsVisible(bool _visible)
{
    d->novelStatisticsVisiblity->setChecked(_visible);
}

void NovelInformationView::updateTranslations()
{
    d->novelName->setLabel(tr("Novel name"));
    d->novelTagline->setLabel(tr("Tagline"));
    d->novelLogline->setLabel(tr("Logline"));
    d->novelLogline->setTrailingIconToolTip(tr("Generate logline"));
    d->titlePageVisiblity->setText(tr("Title page"));
    d->synopsisVisiblity->setText(tr("Synopsis"));
    d->outlineVisiblity->setText(tr("Outline"));
    d->novelTextVisiblity->setText(tr("Novel"));
    d->novelStatisticsVisiblity->setText(tr("Statistics"));
}

void NovelInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());

    d->novelInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->novelName, d->novelTagline, d->novelLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : {
             d->titlePageVisiblity,
             d->synopsisVisiblity,
             d->outlineVisiblity,
             d->novelTextVisiblity,
             d->novelStatisticsVisiblity,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->novelInfoLayout->setVerticalSpacing(Ui::DesignSystem::compactLayout().px16());
    d->novelInfoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->novelInfoLayout->setRowMinimumHeight(
        d->novelInfoLayout->rowCount() - 1,
        static_cast<int>(Ui::DesignSystem::compactLayout().px24()));
}

} // namespace Ui

#include "stageplay_information_view.h"

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

class StageplayInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* stageplayInfo = nullptr;
    QGridLayout* stageplayInfoLayout = nullptr;
    TextField* stageplayName = nullptr;
    TextField* stageplayTagline = nullptr;
    TextField* stageplayLogline = nullptr;
    CheckBox* titlePageVisiblity = nullptr;
    CheckBox* synopsisVisiblity = nullptr;
    CheckBox* stageplayTextVisiblity = nullptr;
    CheckBox* stageplayStatisticsVisiblity = nullptr;
};

StageplayInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , stageplayInfo(new Card(_parent))
    , stageplayInfoLayout(new QGridLayout)
    , stageplayName(new TextField(stageplayInfo))
    , stageplayTagline(new TextField(stageplayInfo))
    , stageplayLogline(new TextField(stageplayInfo))
    , titlePageVisiblity(new CheckBox(stageplayInfo))
    , synopsisVisiblity(new CheckBox(stageplayInfo))
    , stageplayTextVisiblity(new CheckBox(stageplayInfo))
    , stageplayStatisticsVisiblity(new CheckBox(stageplayInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    stageplayName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    UiHelper::initSpellingFor({ stageplayTagline, stageplayLogline });
    stageplayLogline->setEnterMakesNewLine(true);
    stageplayLogline->setTrailingIcon(u8"\U000F1353");

    stageplayInfoLayout->setContentsMargins({});
    stageplayInfoLayout->setSpacing(0);
    int row = 0;
    stageplayInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    stageplayInfoLayout->addWidget(stageplayName, row++, 0);
    stageplayInfoLayout->addWidget(stageplayTagline, row++, 0);
    stageplayInfoLayout->addWidget(stageplayLogline, row++, 0);
    stageplayInfoLayout->addWidget(titlePageVisiblity, row++, 0);
    stageplayInfoLayout->addWidget(synopsisVisiblity, row++, 0);
    stageplayInfoLayout->addWidget(stageplayTextVisiblity, row++, 0);
    stageplayInfoLayout->addWidget(stageplayStatisticsVisiblity, row++, 0);
    stageplayInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    stageplayInfoLayout->setColumnStretch(0, 1);
    stageplayInfo->setContentLayout(stageplayInfoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(stageplayInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


StageplayInformationView::StageplayInformationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->stageplayName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->stageplayName->text()); });
    connect(d->stageplayTagline, &TextField::textChanged, this,
            [this] { emit taglineChanged(d->stageplayTagline->text()); });
    connect(d->stageplayLogline, &TextField::textChanged, this, [this] {
        //
        // Покажем статистику по количеству знаков
        //
        const auto wordsCount = TextHelper::wordsCount(d->stageplayLogline->text());
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
        d->stageplayLogline->setHelper(loglineInfo);

        //
        // Уведомим клиентов об изменении
        //
        emit loglineChanged(d->stageplayLogline->text());
    });
    connect(d->stageplayLogline, &TextField::trailingIconPressed, this, [this] {
        auto dialog = new LoglineGeneratorDialog(topLevelWidget());
        connect(dialog, &LoglineGeneratorDialog::donePressed, this, [this, dialog] {
            d->stageplayLogline->setText(dialog->logline());
            dialog->hideDialog();
        });
        connect(dialog, &LoglineGeneratorDialog::disappeared, dialog,
                &LoglineGeneratorDialog::deleteLater);

        dialog->showDialog();
    });
    connect(d->titlePageVisiblity, &CheckBox::checkedChanged, this,
            &StageplayInformationView::titlePageVisibleChanged);
    connect(d->synopsisVisiblity, &CheckBox::checkedChanged, this,
            &StageplayInformationView::synopsisVisibleChanged);
    connect(d->stageplayTextVisiblity, &CheckBox::checkedChanged, this,
            &StageplayInformationView::stageplayTextVisibleChanged);
    connect(d->stageplayStatisticsVisiblity, &CheckBox::checkedChanged, this,
            &StageplayInformationView::stageplayStatisticsVisibleChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

StageplayInformationView::~StageplayInformationView() = default;

QWidget* StageplayInformationView::asQWidget()
{
    return this;
}

void StageplayInformationView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->stageplayName->setReadOnly(readOnly);
    d->stageplayTagline->setReadOnly(readOnly);
    d->stageplayLogline->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->titlePageVisiblity->setEnabled(enabled);
    d->synopsisVisiblity->setEnabled(enabled);
    d->stageplayTextVisiblity->setEnabled(enabled);
    d->stageplayStatisticsVisiblity->setEnabled(enabled);
}

void StageplayInformationView::setName(const QString& _name)
{
    if (d->stageplayName->text() == _name) {
        return;
    }

    d->stageplayName->setText(_name);
}

void StageplayInformationView::setTagline(const QString& _tagline)
{
    if (d->stageplayTagline->text() == _tagline) {
        return;
    }

    d->stageplayTagline->setText(_tagline);
}

void StageplayInformationView::setLogline(const QString& _logline)
{
    if (d->stageplayLogline->text() == _logline) {
        return;
    }

    d->stageplayLogline->setText(_logline);
}

void StageplayInformationView::setTitlePageVisible(bool _visible)
{
    d->titlePageVisiblity->setChecked(_visible);
}

void StageplayInformationView::setSynopsisVisible(bool _visible)
{
    d->synopsisVisiblity->setChecked(_visible);
}

void StageplayInformationView::setStageplayTextVisible(bool _visible)
{
    d->stageplayTextVisiblity->setChecked(_visible);
}

void StageplayInformationView::setStageplayStatisticsVisible(bool _visible)
{
    d->stageplayStatisticsVisiblity->setChecked(_visible);
}

void StageplayInformationView::updateTranslations()
{
    d->stageplayName->setLabel(tr("Stageplay name"));
    d->stageplayTagline->setLabel(tr("Tagline"));
    d->stageplayLogline->setLabel(tr("Logline"));
    d->stageplayLogline->setTrailingIconToolTip(tr("Generate logline"));
    d->titlePageVisiblity->setText(tr("Title page"));
    d->synopsisVisiblity->setText(tr("Synopsis"));
    d->stageplayTextVisiblity->setText(tr("Stageplay"));
    d->stageplayStatisticsVisiblity->setText(tr("Statistics"));
}

void StageplayInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->stageplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->stageplayName, d->stageplayTagline, d->stageplayLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : { d->titlePageVisiblity, d->synopsisVisiblity, d->stageplayTextVisiblity,
                           d->stageplayStatisticsVisiblity }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->stageplayInfoLayout->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->stageplayInfoLayout->setRowMinimumHeight(
        0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->stageplayInfoLayout->setRowMinimumHeight(
        d->stageplayInfoLayout->rowCount() - 1,
        static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui

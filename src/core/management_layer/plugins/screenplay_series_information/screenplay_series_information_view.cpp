#include "screenplay_series_information_view.h"

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

class ScreenplaySeriesInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* seriesInfo = nullptr;
    QGridLayout* infoLayout = nullptr;
    TextField* seriesName = nullptr;
    TextField* seriesTagline = nullptr;
    TextField* seriesLogline = nullptr;
    CheckBox* titlePageVisiblity = nullptr;
    CheckBox* synopsisVisiblity = nullptr;
    CheckBox* treatmentVisiblity = nullptr;
    CheckBox* screenplayVisiblity = nullptr;
    CheckBox* statisticsVisiblity = nullptr;
};

ScreenplaySeriesInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , seriesInfo(new Card(_parent))
    , infoLayout(new QGridLayout)
    , seriesName(new TextField(seriesInfo))
    , seriesTagline(new TextField(seriesInfo))
    , seriesLogline(new TextField(seriesInfo))
    , titlePageVisiblity(new CheckBox(seriesInfo))
    , synopsisVisiblity(new CheckBox(seriesInfo))
    , treatmentVisiblity(new CheckBox(seriesInfo))
    , screenplayVisiblity(new CheckBox(seriesInfo))
    , statisticsVisiblity(new CheckBox(seriesInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    seriesName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    seriesLogline->setEnterMakesNewLine(true);
    seriesLogline->setTrailingIcon(u8"\U000F1353");
    UiHelper::initSpellingFor({
        seriesTagline,
        seriesLogline,
    });
    UiHelper::initOptionsFor({
        seriesName,
        seriesTagline,
        seriesLogline,
    });

    infoLayout->setContentsMargins({});
    infoLayout->setSpacing(0);
    int row = 0;
    infoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    infoLayout->addWidget(seriesName, row++, 0);
    infoLayout->addWidget(seriesTagline, row++, 0);
    infoLayout->addWidget(seriesLogline, row++, 0);
    infoLayout->addWidget(titlePageVisiblity, row++, 0);
    infoLayout->addWidget(synopsisVisiblity, row++, 0);
    infoLayout->addWidget(treatmentVisiblity, row++, 0);
    infoLayout->addWidget(screenplayVisiblity, row++, 0);
    infoLayout->addWidget(statisticsVisiblity, row++, 0);
    infoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    infoLayout->setColumnStretch(0, 1);
    seriesInfo->setContentLayout(infoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(seriesInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);

    for (auto widget : {
             titlePageVisiblity,
             synopsisVisiblity,
             treatmentVisiblity,
             screenplayVisiblity,
         }) {
        widget->hide();
    }
}


// ****


ScreenplaySeriesInformationView::ScreenplaySeriesInformationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->seriesName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->seriesName->text()); });
    connect(d->seriesTagline, &TextField::textChanged, this,
            [this] { emit taglineChanged(d->seriesTagline->text()); });
    connect(d->seriesLogline, &TextField::textChanged, this, [this] {
        //
        // Покажем статистику по количеству знаков
        //
        const auto wordsCount = TextHelper::wordsCount(d->seriesLogline->text());
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
        d->seriesLogline->setHelper(loglineInfo);

        //
        // Уведомим клиентов об изменении
        //
        emit loglineChanged(d->seriesLogline->text());
    });
    connect(d->seriesLogline, &TextField::trailingIconPressed, this, [this] {
        auto dialog = new LoglineGeneratorDialog(topLevelWidget());
        connect(dialog, &LoglineGeneratorDialog::donePressed, this, [this, dialog] {
            d->seriesLogline->setText(dialog->logline());
            dialog->hideDialog();
        });
        connect(dialog, &LoglineGeneratorDialog::disappeared, dialog,
                &LoglineGeneratorDialog::deleteLater);

        dialog->showDialog();
    });
    connect(d->titlePageVisiblity, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesInformationView::titlePageVisibleChanged);
    connect(d->synopsisVisiblity, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesInformationView::synopsisVisibleChanged);
    connect(d->treatmentVisiblity, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesInformationView::treatmentVisibleChanged);
    connect(d->screenplayVisiblity, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesInformationView::screenplayVisibleChanged);
    connect(d->statisticsVisiblity, &CheckBox::checkedChanged, this,
            &ScreenplaySeriesInformationView::statisticsVisibleChanged);
}

ScreenplaySeriesInformationView::~ScreenplaySeriesInformationView() = default;

QWidget* ScreenplaySeriesInformationView::asQWidget()
{
    return this;
}

void ScreenplaySeriesInformationView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->seriesName->setReadOnly(readOnly);
    d->seriesTagline->setReadOnly(readOnly);
    d->seriesLogline->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->titlePageVisiblity->setEnabled(enabled);
    d->synopsisVisiblity->setEnabled(enabled);
    d->treatmentVisiblity->setEnabled(enabled);
    d->screenplayVisiblity->setEnabled(enabled);
    d->statisticsVisiblity->setEnabled(enabled);
}

void ScreenplaySeriesInformationView::setName(const QString& _name)
{
    if (d->seriesName->text() == _name) {
        return;
    }

    d->seriesName->setText(_name);
}

void ScreenplaySeriesInformationView::setTagline(const QString& _tagline)
{
    if (d->seriesTagline->text() == _tagline) {
        return;
    }

    d->seriesTagline->setText(_tagline);
}

void ScreenplaySeriesInformationView::setLogline(const QString& _logline)
{
    if (d->seriesLogline->text() == _logline) {
        return;
    }

    d->seriesLogline->setText(_logline);
}

void ScreenplaySeriesInformationView::setTitlePageVisible(bool _visible)
{
    d->titlePageVisiblity->setChecked(_visible);
}

void ScreenplaySeriesInformationView::setSynopsisVisible(bool _visible)
{
    d->synopsisVisiblity->setChecked(_visible);
}

void ScreenplaySeriesInformationView::setTreatmentVisible(bool _visible)
{
    d->treatmentVisiblity->setChecked(_visible);
}

void ScreenplaySeriesInformationView::setScreenplayVisible(bool _visible)
{
    d->screenplayVisiblity->setChecked(_visible);
}

void ScreenplaySeriesInformationView::setStatisticsVisible(bool _visible)
{
    d->statisticsVisiblity->setChecked(_visible);
}

void ScreenplaySeriesInformationView::updateTranslations()
{
    d->seriesName->setLabel(tr("Series name"));
    d->seriesTagline->setLabel(tr("Tagline"));
    d->seriesLogline->setLabel(tr("Logline"));
    d->seriesLogline->setTrailingIconToolTip(tr("Generate logline"));
    d->titlePageVisiblity->setText(tr("Title page"));
    d->synopsisVisiblity->setText(tr("Synopsis"));
    d->treatmentVisiblity->setText(tr("Treatment"));
    d->screenplayVisiblity->setText(tr("Screenplay"));
    d->statisticsVisiblity->setText(tr("Statistics"));
}

void ScreenplaySeriesInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());

    d->seriesInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->seriesName, d->seriesTagline, d->seriesLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : { d->titlePageVisiblity, d->synopsisVisiblity, d->treatmentVisiblity,
                           d->screenplayVisiblity, d->statisticsVisiblity }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->infoLayout->setVerticalSpacing(Ui::DesignSystem::compactLayout().px16());
    d->infoLayout->setRowMinimumHeight(0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->infoLayout->setRowMinimumHeight(d->infoLayout->rowCount() - 1,
                                       static_cast<int>(Ui::DesignSystem::compactLayout().px24()));
}

} // namespace Ui

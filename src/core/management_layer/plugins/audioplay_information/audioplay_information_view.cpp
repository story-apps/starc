#include "audioplay_information_view.h"

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

class AudioplayInformationView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* audioplayInfo = nullptr;
    QGridLayout* audioplayInfoLayout = nullptr;
    TextField* audioplayName = nullptr;
    TextField* audioplayTagline = nullptr;
    TextField* audioplayLogline = nullptr;
    CheckBox* titlePageVisiblity = nullptr;
    CheckBox* synopsisVisiblity = nullptr;
    CheckBox* audioplayTextVisiblity = nullptr;
    CheckBox* audioplayStatisticsVisiblity = nullptr;
};

AudioplayInformationView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , audioplayInfo(new Card(_parent))
    , audioplayInfoLayout(new QGridLayout)
    , audioplayName(new TextField(audioplayInfo))
    , audioplayTagline(new TextField(audioplayInfo))
    , audioplayLogline(new TextField(audioplayInfo))
    , titlePageVisiblity(new CheckBox(audioplayInfo))
    , synopsisVisiblity(new CheckBox(audioplayInfo))
    , audioplayTextVisiblity(new CheckBox(audioplayInfo))
    , audioplayStatisticsVisiblity(new CheckBox(audioplayInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    audioplayName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    UiHelper::initSpellingFor({ audioplayTagline, audioplayLogline });
    audioplayLogline->setEnterMakesNewLine(true);
    audioplayLogline->setTrailingIcon(u8"\U000F1353");

    audioplayInfoLayout->setContentsMargins({});
    audioplayInfoLayout->setSpacing(0);
    int row = 0;
    audioplayInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку сверху
    audioplayInfoLayout->addWidget(audioplayName, row++, 0);
    audioplayInfoLayout->addWidget(audioplayTagline, row++, 0);
    audioplayInfoLayout->addWidget(audioplayLogline, row++, 0);
    audioplayInfoLayout->addWidget(titlePageVisiblity, row++, 0);
    audioplayInfoLayout->addWidget(synopsisVisiblity, row++, 0);
    audioplayInfoLayout->addWidget(audioplayTextVisiblity, row++, 0);
    audioplayInfoLayout->addWidget(audioplayStatisticsVisiblity, row++, 0);
    audioplayInfoLayout->setRowMinimumHeight(row++, 1); // добавляем пустую строку внизу
    audioplayInfo->setContentLayout(audioplayInfoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(audioplayInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


AudioplayInformationView::AudioplayInformationView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->audioplayName, &TextField::textChanged, this,
            [this] { emit nameChanged(d->audioplayName->text()); });
    connect(d->audioplayTagline, &TextField::textChanged, this,
            [this] { emit taglineChanged(d->audioplayTagline->text()); });
    connect(d->audioplayLogline, &TextField::textChanged, this, [this] {
        //
        // Покажем статистику по количеству знаков
        //
        const auto wordsCount = TextHelper::wordsCount(d->audioplayLogline->text());
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
        d->audioplayLogline->setHelper(loglineInfo);

        //
        // Уведомим клиентов об изменении
        //
        emit loglineChanged(d->audioplayLogline->text());
    });
    connect(d->audioplayLogline, &TextField::trailingIconPressed, this, [this] {
        auto dialog = new LoglineGeneratorDialog(topLevelWidget());
        connect(dialog, &LoglineGeneratorDialog::donePressed, this, [this, dialog] {
            d->audioplayLogline->setText(dialog->logline());
            dialog->hideDialog();
        });
        connect(dialog, &LoglineGeneratorDialog::disappeared, dialog,
                &LoglineGeneratorDialog::deleteLater);

        dialog->showDialog();
    });
    connect(d->titlePageVisiblity, &CheckBox::checkedChanged, this,
            &AudioplayInformationView::titlePageVisibleChanged);
    connect(d->synopsisVisiblity, &CheckBox::checkedChanged, this,
            &AudioplayInformationView::synopsisVisibleChanged);
    connect(d->audioplayTextVisiblity, &CheckBox::checkedChanged, this,
            &AudioplayInformationView::audioplayTextVisibleChanged);
    connect(d->audioplayStatisticsVisiblity, &CheckBox::checkedChanged, this,
            &AudioplayInformationView::audioplayStatisticsVisibleChanged);
}

AudioplayInformationView::~AudioplayInformationView() = default;

QWidget* AudioplayInformationView::asQWidget()
{
    return this;
}

void AudioplayInformationView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->audioplayName->setReadOnly(readOnly);
    d->audioplayTagline->setReadOnly(readOnly);
    d->audioplayLogline->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->titlePageVisiblity->setEnabled(enabled);
    d->synopsisVisiblity->setEnabled(enabled);
    d->audioplayTextVisiblity->setEnabled(enabled);
    d->audioplayStatisticsVisiblity->setEnabled(enabled);
}

void AudioplayInformationView::setName(const QString& _name)
{
    if (d->audioplayName->text() == _name) {
        return;
    }

    d->audioplayName->setText(_name);
}

void AudioplayInformationView::setTagline(const QString& _tagline)
{
    if (d->audioplayTagline->text() == _tagline) {
        return;
    }

    d->audioplayTagline->setText(_tagline);
}

void AudioplayInformationView::setLogline(const QString& _logline)
{
    if (d->audioplayLogline->text() == _logline) {
        return;
    }

    d->audioplayLogline->setText(_logline);
}

void AudioplayInformationView::setTitlePageVisible(bool _visible)
{
    d->titlePageVisiblity->setChecked(_visible);
}

void AudioplayInformationView::setSynopsisVisible(bool _visible)
{
    d->synopsisVisiblity->setChecked(_visible);
}

void AudioplayInformationView::setAudioplayTextVisible(bool _visible)
{
    d->audioplayTextVisiblity->setChecked(_visible);
}

void AudioplayInformationView::setAudioplayStatisticsVisible(bool _visible)
{
    d->audioplayStatisticsVisiblity->setChecked(_visible);
}

void AudioplayInformationView::updateTranslations()
{
    d->audioplayName->setLabel(tr("Audioplay name"));
    d->audioplayTagline->setLabel(tr("Tagline"));
    d->audioplayLogline->setLabel(tr("Logline"));
    d->audioplayLogline->setTrailingIconToolTip(tr("Generate logline"));
    d->titlePageVisiblity->setText(tr("Title page"));
    d->synopsisVisiblity->setText(tr("Synopsis"));
    d->audioplayTextVisiblity->setText(tr("Audioplay"));
    d->audioplayStatisticsVisiblity->setText(tr("Statistics"));
}

void AudioplayInformationView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());

    d->audioplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : { d->audioplayName, d->audioplayTagline, d->audioplayLogline }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : { d->titlePageVisiblity, d->synopsisVisiblity, d->audioplayTextVisiblity,
                           d->audioplayStatisticsVisiblity }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->audioplayInfoLayout->setVerticalSpacing(
        static_cast<int>(Ui::DesignSystem::compactLayout().px16()));
    d->audioplayInfoLayout->setRowMinimumHeight(
        0, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->audioplayInfoLayout->setRowMinimumHeight(
        d->audioplayInfoLayout->rowCount() - 1,
        static_cast<int>(Ui::DesignSystem::compactLayout().px24()));
}

} // namespace Ui

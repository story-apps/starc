#include "screenplay_parameters_view.h"

#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QScrollArea>
#include <QStandardItemModel>


namespace Ui {

namespace {

const QLatin1String kDefaultSceneNubersTemplate("#.");

/**
 * @brief Сформиовать компоновщик для строки настроек
 */
QHBoxLayout* makeLayout()
{
    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    return layout;
};

} // namespace

class ScreenplayParametersView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* screenplayInfo = nullptr;
    QVBoxLayout* infoLayout = nullptr;
    TextField* header = nullptr;
    CheckBox* printHeaderOnTitlePage = nullptr;
    TextField* footer = nullptr;
    CheckBox* printFooterOnTitlePage = nullptr;

    H6Label* scenesNumbersTitle = nullptr;
    TextField* scenesNumbersTemplate = nullptr;
    TextField* scenesNumberingStartAt = nullptr;
    Button* lockScenesNumbers = nullptr;
    Button* relockScenesNumbers = nullptr;
    Button* unlockScenesNumbers = nullptr;
};

ScreenplayParametersView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , screenplayInfo(new Card(_parent))
    , infoLayout(new QVBoxLayout)
    , header(new TextField(screenplayInfo))
    , printHeaderOnTitlePage(new CheckBox(screenplayInfo))
    , footer(new TextField(screenplayInfo))
    , printFooterOnTitlePage(new CheckBox(screenplayInfo))
    , scenesNumbersTitle(new H6Label(screenplayInfo))
    , scenesNumbersTemplate(new TextField(screenplayInfo))
    , scenesNumberingStartAt(new TextField(screenplayInfo))
    , lockScenesNumbers(new Button(screenplayInfo))
    , relockScenesNumbers(new Button(screenplayInfo))
    , unlockScenesNumbers(new Button(screenplayInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    header->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    footer->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    scenesNumbersTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    scenesNumberingStartAt->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    UiHelper::initOptionsFor({
        header,
        footer,
        scenesNumbersTemplate,
    });

    lockScenesNumbers->setContained(true);
    relockScenesNumbers->setContained(true);
    relockScenesNumbers->hide();
    unlockScenesNumbers->setContained(true);
    unlockScenesNumbers->hide();


    infoLayout->setDirection(QBoxLayout::TopToBottom);
    infoLayout->setContentsMargins({});
    infoLayout->setSpacing(0);
    infoLayout->addWidget(header);
    infoLayout->addWidget(printHeaderOnTitlePage);
    infoLayout->addWidget(footer);
    infoLayout->addWidget(printFooterOnTitlePage);
    infoLayout->addWidget(scenesNumbersTitle);
    infoLayout->addWidget(scenesNumbersTemplate);
    infoLayout->addWidget(scenesNumberingStartAt);
    {
        auto layout = makeLayout();
        layout->addWidget(lockScenesNumbers);
        layout->addWidget(relockScenesNumbers);
        layout->addWidget(unlockScenesNumbers);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    screenplayInfo->setContentLayout(infoLayout);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(screenplayInfo);
    layout->addStretch(1);
    contentWidget->setLayout(layout);
}


// ****


ScreenplayParametersView::ScreenplayParametersView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->header, &TextField::textChanged, this,
            [this] { emit headerChanged(d->header->text()); });
    connect(d->printHeaderOnTitlePage, &CheckBox::checkedChanged, this,
            &ScreenplayParametersView::printHeaderOnTitlePageChanged);
    connect(d->footer, &TextField::textChanged, this,
            [this] { emit footerChanged(d->footer->text()); });
    connect(d->printFooterOnTitlePage, &CheckBox::checkedChanged, this,
            &ScreenplayParametersView::printFooterOnTitlePageChanged);
    connect(d->scenesNumbersTemplate, &TextField::textChanged, this, [this] {
        const auto numbersTemplate = d->scenesNumbersTemplate->text();
        d->scenesNumbersTemplate->setTrailingIcon(
            numbersTemplate == kDefaultSceneNubersTemplate ? "" : u8"\U000F0450");

        emit scenesNumbersTemplateChanged(numbersTemplate);
    });
    connect(d->scenesNumbersTemplate, &TextField::trailingIconPressed, this,
            [this] { d->scenesNumbersTemplate->setText(kDefaultSceneNubersTemplate); });
    connect(d->scenesNumberingStartAt, &TextField::textChanged, this, [this] {
        bool isNumberValid = false;
        const auto startNumber = d->scenesNumberingStartAt->text().toInt(&isNumberValid);
        if (isNumberValid) {
            emit scenesNumberingStartAtChanged(startNumber);
        } else {
            d->scenesNumberingStartAt->undo();
        }
    });
    connect(d->lockScenesNumbers, &Button::clicked, this,
            [this] { emit isScenesNumberingLockedChanged(true); });
    connect(d->relockScenesNumbers, &Button::clicked, this,
            [this] { emit isScenesNumberingLockedChanged(true); });
    connect(d->unlockScenesNumbers, &Button::clicked, this, [this] {
        auto dialog = new Dialog(topLevelWidget());
        const int kCancelButtonId = 0;
        const int kYesButtonId = 1;
        dialog->showDialog({}, tr("Do you want unlock scene numbers?"),
                           { { kCancelButtonId, tr("Cancel"), Dialog::RejectButton },
                             { kYesButtonId, tr("Yes, unlock"), Dialog::AcceptButton } });
        QObject::connect(dialog, &Dialog::finished, dialog,
                         [this, dialog, kCancelButtonId](const Dialog::ButtonInfo& _buttonInfo) {
                             dialog->hideDialog();

                             //
                             // Пользователь не хочет обновляться
                             //
                             if (_buttonInfo.id == kCancelButtonId) {
                                 return;
                             }

                             emit isScenesNumberingLockedChanged(false);
                         });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
    });
}

ScreenplayParametersView::~ScreenplayParametersView() = default;

QWidget* ScreenplayParametersView::asQWidget()
{
    return this;
}

void ScreenplayParametersView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto readOnly = _mode != ManagementLayer::DocumentEditingMode::Edit;
    d->header->setReadOnly(readOnly);
    d->footer->setReadOnly(readOnly);
    d->scenesNumbersTemplate->setReadOnly(readOnly);
    d->scenesNumberingStartAt->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->printHeaderOnTitlePage->setEnabled(enabled);
    d->printFooterOnTitlePage->setEnabled(enabled);
    d->lockScenesNumbers->setEnabled(enabled);
    d->relockScenesNumbers->setEnabled(enabled);
    d->unlockScenesNumbers->setEnabled(enabled);
}

void ScreenplayParametersView::setHeader(const QString& _header)
{
    if (d->header->text() == _header) {
        return;
    }

    d->header->setText(_header);
}

void ScreenplayParametersView::setPrintHeaderOnTitlePage(bool _print)
{
    if (d->printHeaderOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printHeaderOnTitlePage->setChecked(_print);
}

void ScreenplayParametersView::setFooter(const QString& _footer)
{
    if (d->footer->text() == _footer) {
        return;
    }

    d->footer->setText(_footer);
}

void ScreenplayParametersView::setPrintFooterOnTitlePage(bool _print)
{
    if (d->printFooterOnTitlePage->isChecked() == _print) {
        return;
    }

    d->printFooterOnTitlePage->setChecked(_print);
}

void ScreenplayParametersView::setScenesNumbersTemplate(const QString& _template)
{
    if (d->scenesNumbersTemplate->text() == _template) {
        return;
    }

    d->scenesNumbersTemplate->setText(_template);
}

void ScreenplayParametersView::setScenesNumberingStartAt(int _startNumber)
{
    const auto startNumberText = QString::number(_startNumber);
    if (d->scenesNumberingStartAt->text() == startNumberText) {
        return;
    }

    d->scenesNumberingStartAt->setText(startNumberText);
}

void ScreenplayParametersView::setScenesNumbersLocked(bool _locked)
{
    //
    // Тут шаманим немного с порядком скрытия/отображения, чтобы фокус не скакал
    // на следующий за кнопками элемент интерфейса
    //
    if (_locked) {
        d->relockScenesNumbers->setVisible(_locked);
        d->unlockScenesNumbers->setVisible(_locked);
        d->lockScenesNumbers->setVisible(!_locked);
    } else {
        d->lockScenesNumbers->setVisible(!_locked);
        d->relockScenesNumbers->setVisible(_locked);
        d->unlockScenesNumbers->setVisible(_locked);
    }
}

void ScreenplayParametersView::updateTranslations()
{
    d->header->setLabel(tr("Header"));
    d->printHeaderOnTitlePage->setText(tr("Print header on title page"));
    d->footer->setLabel(tr("Footer"));
    d->printFooterOnTitlePage->setText(tr("Print footer on title page"));
    d->scenesNumbersTitle->setText(tr("Scenes numbering"));
    d->scenesNumbersTemplate->setLabel(tr("Scenes numbers' template"));
    d->scenesNumbersTemplate->setHelper(tr("Use # mark for the scene number"));
    d->scenesNumbersTemplate->setTrailingIconToolTip(tr("Reset scene numbers template"));
    d->scenesNumberingStartAt->setLabel(tr("Scenes numbering start at"));
    d->lockScenesNumbers->setText(tr("Lock numbering"));
    d->relockScenesNumbers->setText(tr("Lock numbering again"));
    d->unlockScenesNumbers->setText(tr("Unlock numbering"));
}

void ScreenplayParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::compactLayout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24())
            .toMargins());

    d->screenplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : std::vector<TextField*>{
             d->header,
             d->footer,
             d->scenesNumbersTemplate,
             d->scenesNumberingStartAt,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto checkBox : {
             d->printHeaderOnTitlePage,
             d->printFooterOnTitlePage,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    auto titleMargins = Ui::DesignSystem::label().margins();
    titleMargins.setTop(0.0);
    titleMargins.setBottom(0.0);
    for (auto title : {
             d->scenesNumbersTitle,
         }) {
        title->setBackgroundColor(Ui::DesignSystem::color().background());
        title->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onBackground(),
                                                     Ui::DesignSystem::inactiveTextOpacity()));
        title->setContentsMargins(titleMargins.toMargins());
    }
    for (auto button : {
             d->lockScenesNumbers,
             d->relockScenesNumbers,
             d->unlockScenesNumbers,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().onAccent());
    }
    for (auto button : {
             d->lockScenesNumbers,
             d->relockScenesNumbers,
         }) {
        if (isLeftToRight()) {
            button->setContentsMargins(Ui::DesignSystem::layout().px16(), 0, 0, 0);
        } else {
            button->setContentsMargins(0, 0, Ui::DesignSystem::layout().px16(), 0);
        }
    }

    d->infoLayout->setSpacing(static_cast<int>(Ui::DesignSystem::compactLayout().px16()));
    d->infoLayout->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                      static_cast<int>(Ui::DesignSystem::layout().px12()));
}

} // namespace Ui

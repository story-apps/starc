#include "screenplay_parameters_view.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QGridLayout>
#include <QScrollArea>
#include <QStandardItemModel>


namespace Ui {

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

    CheckBox* overrideCommonSettings = nullptr;
    ComboBox* screenplayTemplate = nullptr;
    CheckBox* showSceneNumbers = nullptr;
    CheckBox* showSceneNumbersOnLeft = nullptr;
    CheckBox* showSceneNumbersOnRight = nullptr;
    CheckBox* showDialoguesNumbers = nullptr;
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
    , overrideCommonSettings(new CheckBox(screenplayInfo))
    , screenplayTemplate(new ComboBox(screenplayInfo))
    , showSceneNumbers(new CheckBox(screenplayInfo))
    , showSceneNumbersOnLeft(new CheckBox(screenplayInfo))
    , showSceneNumbersOnRight(new CheckBox(screenplayInfo))
    , showDialoguesNumbers(new CheckBox(screenplayInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    screenplayInfo->setResizingActive(false);

    header->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    footer->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    scenesNumbersTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    scenesNumberingStartAt->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    lockScenesNumbers->setContained(true);
    relockScenesNumbers->setContained(true);
    relockScenesNumbers->hide();
    unlockScenesNumbers->setContained(true);
    unlockScenesNumbers->hide();

    screenplayTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayTemplate->setModel(BusinessLayer::TemplatesFacade::screenplayTemplates());
    screenplayTemplate->hide();

    showSceneNumbers->hide();
    showSceneNumbersOnLeft->setEnabled(false);
    showSceneNumbersOnLeft->hide();
    showSceneNumbersOnRight->setEnabled(false);
    showSceneNumbersOnRight->hide();
    showDialoguesNumbers->hide();

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
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(lockScenesNumbers);
        layout->addWidget(relockScenesNumbers);
        layout->addWidget(unlockScenesNumbers);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    infoLayout->addWidget(overrideCommonSettings, 1, Qt::AlignTop);
    infoLayout->addWidget(screenplayTemplate);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(showSceneNumbers);
        layout->addWidget(showSceneNumbersOnLeft);
        layout->addWidget(showSceneNumbersOnRight);
        layout->addStretch();
        infoLayout->addLayout(layout);
    }
    infoLayout->addWidget(showDialoguesNumbers);
    screenplayInfo->setContentLayout(infoLayout);

    //
    // TODO: С лёту не завелось, т.к. при отображении скрытых виджетов, виджеты, которые были видны,
    // сжимаются лейаутом, что даёт некрасивый эффект дёргания (собственно это актуально и для
    // диалогов, просто там это не так сильно заметно как на больших карточках).
    //
    // Во время экспериментов не помогли ни фиксация размера виджета, ни установка минимального
    // размера строки лейаута, ни разные полтики лейута, надо смотреть код лейаута и искать лазейку,
    // как заставить его не сжимать некоторые из виджетов
    //
    screenplayInfo->setResizingActive(true);

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(screenplayInfo);
    layout->addStretch();
    contentWidget->setLayout(layout);
}


// ****


ScreenplayParametersView::ScreenplayParametersView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
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
    connect(d->scenesNumbersTemplate, &TextField::textChanged, this,
            [this] { emit scenesNumbersTemplateChanged(d->scenesNumbersTemplate->text()); });
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
    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this,
            &ScreenplayParametersView::overrideCommonSettingsChanged);
    connect(d->screenplayTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto templateId
                    = _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
                emit screenplayTemplateChanged(templateId);
            });
    connect(d->showSceneNumbers, &CheckBox::checkedChanged, this,
            &ScreenplayParametersView::showSceneNumbersChanged);
    connect(d->showSceneNumbersOnLeft, &CheckBox::checkedChanged, this,
            &ScreenplayParametersView::showSceneNumbersOnLeftChanged);
    connect(d->showSceneNumbersOnRight, &CheckBox::checkedChanged, this,
            &ScreenplayParametersView::showSceneNumbersOnRightChanged);
    connect(d->showDialoguesNumbers, &CheckBox::checkedChanged, this,
            &ScreenplayParametersView::showDialoguesNumbersChanged);

    connect(d->overrideCommonSettings, &CheckBox::checkedChanged, this, [this](bool _checked) {
        d->screenplayTemplate->setVisible(_checked);
        d->showSceneNumbers->setVisible(_checked);
        d->showSceneNumbersOnLeft->setVisible(_checked);
        d->showSceneNumbersOnRight->setVisible(_checked);
        d->showDialoguesNumbers->setVisible(_checked);
    });
    connect(d->showSceneNumbers, &CheckBox::checkedChanged, d->showSceneNumbersOnLeft,
            &CheckBox::setEnabled);
    connect(d->showSceneNumbers, &CheckBox::checkedChanged, d->showSceneNumbersOnRight,
            &CheckBox::setEnabled);
    auto correctShownSceneNumber = [this] {
        if (!d->showSceneNumbersOnLeft->isChecked() && !d->showSceneNumbersOnRight->isChecked()) {
            d->showSceneNumbersOnLeft->setChecked(true);
        }
    };
    connect(d->showSceneNumbersOnLeft, &CheckBox::checkedChanged, this, correctShownSceneNumber);
    connect(d->showSceneNumbersOnRight, &CheckBox::checkedChanged, this, correctShownSceneNumber);
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
    d->screenplayTemplate->setReadOnly(readOnly);
    const auto enabled = !readOnly;
    d->printHeaderOnTitlePage->setEnabled(enabled);
    d->printFooterOnTitlePage->setEnabled(enabled);
    d->overrideCommonSettings->setEnabled(enabled);
    d->showSceneNumbers->setEnabled(enabled);
    d->showSceneNumbersOnLeft->setEnabled(enabled);
    d->showSceneNumbersOnRight->setEnabled(enabled);
    d->showDialoguesNumbers->setEnabled(enabled);
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

void ScreenplayParametersView::setOverrideCommonSettings(bool _override)
{
    if (d->overrideCommonSettings->isChecked() == _override) {
        return;
    }

    d->overrideCommonSettings->setChecked(_override);
}

void ScreenplayParametersView::setScreenplayTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::screenplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::screenplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        if (d->screenplayTemplate->currentIndex() != item->index()) {
            d->screenplayTemplate->setCurrentIndex(item->index());
        }
        break;
    }
}

void ScreenplayParametersView::setShowSceneNumbers(bool _show)
{
    if (d->showSceneNumbers->isChecked() == _show) {
        return;
    }

    d->showSceneNumbers->setChecked(_show);
}

void ScreenplayParametersView::setShowSceneNumbersOnLeft(bool _show)
{
    if (d->showSceneNumbersOnLeft->isChecked() == _show) {
        return;
    }

    d->showSceneNumbersOnLeft->setChecked(_show);
}

void ScreenplayParametersView::setShowSceneNumbersOnRight(bool _show)
{
    if (d->showSceneNumbersOnRight->isChecked() == _show) {
        return;
    }

    d->showSceneNumbersOnRight->setChecked(_show);
}

void ScreenplayParametersView::setShowDialoguesNumbers(bool _show)
{
    if (d->showDialoguesNumbers->isChecked() == _show) {
        return;
    }

    d->showDialoguesNumbers->setChecked(_show);
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
    d->scenesNumberingStartAt->setLabel(tr("Scenes numbering start at"));
    d->lockScenesNumbers->setText(tr("Lock numbering"));
    d->relockScenesNumbers->setText(tr("Lock numbering agian"));
    d->unlockScenesNumbers->setText(tr("Unlock numbering"));
    d->overrideCommonSettings->setText(tr("Override common settings for this screenplay"));
    d->screenplayTemplate->setLabel(tr("Template"));
    d->showSceneNumbers->setText(tr("Print scenes numbers"));
    d->showSceneNumbersOnLeft->setText(tr("on the left"));
    d->showSceneNumbersOnRight->setText(tr("on the right"));
    d->showDialoguesNumbers->setText(tr("Print dialogues numbers"));
}

void ScreenplayParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->screenplayInfo->setBackgroundColor(DesignSystem::color().background());
    for (auto textField : std::vector<TextField*>{
             d->header,
             d->footer,
             d->scenesNumbersTemplate,
             d->scenesNumberingStartAt,
             d->screenplayTemplate,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->screenplayTemplate,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }
    for (auto checkBox : {
             d->printHeaderOnTitlePage,
             d->printFooterOnTitlePage,
             d->overrideCommonSettings,
             d->showSceneNumbers,
             d->showSceneNumbersOnLeft,
             d->showSceneNumbersOnRight,
             d->showDialoguesNumbers,
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
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().onSecondary());
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
    d->infoLayout->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->infoLayout->setContentsMargins(0, static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                      static_cast<int>(Ui::DesignSystem::layout().px12()));
}

} // namespace Ui

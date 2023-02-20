#include "screenplay_item_parameters_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/toggle/toggle.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QScrollArea>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

class ScreenplayItemParametersView::Implementation
{
public:
    explicit Implementation(ScreenplayItemParametersView* _q);

    /**
     * @brief Настроить поля с описанием карточки
     */
    void initCardBeats();

    /**
     * @brief Обновить отступы поля кастомного номера сцены
     */
    void updateCustomNumberMargins();

    /**
     * @brief Обновить размер списка тэгов
     */
    void updateTagsSize();

    /**
     * @brief Уведомить о том, что список тэгов изменился
     */
    void notifyTagsChanged();


    ScreenplayItemParametersView* q = nullptr;

    /**
     * @brief Текущий тип элемента для отображения
     */
    ScreenplayItemType currentItemType = ScreenplayItemType::Folder;

    /**
     * @brief Попап выбора цвета
     */
    ColorPickerPopup* colorPickerPopup = nullptr;

    /**
     * @brief Параметры сцены
     */
    QScrollArea* content = nullptr;
    QVBoxLayout* contentLayout = nullptr;
    TextField* title = nullptr;
    TextField* heading = nullptr;
    QVector<TextField*> beats;
    TextField* storyDay = nullptr;
    QStringListModel* storyDaysModel = nullptr;
    bool isStampVisible = true;
    TextField* stamp = nullptr;
    Subtitle2Label* numberingTitle = nullptr;
    Toggle* autoNumbering = nullptr;
    TextField* customNumber = nullptr;
    CheckBox* eatNumber = nullptr;
    bool isTagsVisible = true;
    Subtitle2Label* tagsTitle = nullptr;
    IconButton* addTagButton = nullptr;
    Tree* tags = nullptr;
    QStandardItemModel* tagsModel = nullptr;
    TextFieldItemDelegate* tagsDelegate = nullptr;
};

ScreenplayItemParametersView::Implementation::Implementation(ScreenplayItemParametersView* _q)
    : q(_q)
    , colorPickerPopup(new ColorPickerPopup(_q))
    , content(new QScrollArea(_q))
    , contentLayout(new QVBoxLayout)
    , title(new TextField(content))
    , heading(new TextField(content))
    , beats({ new TextField(content) })
    , storyDay(new TextField(content))
    , storyDaysModel(new QStringListModel(storyDay))
    , stamp(new TextField(content))
    , numberingTitle(new Subtitle2Label(content))
    , autoNumbering(new Toggle(content))
    , customNumber(new TextField(content))
    , eatNumber(new CheckBox(content))
    , tagsTitle(new Subtitle2Label(content))
    , addTagButton(new IconButton(content))
    , tags(new Tree(content))
    , tagsModel(new QStandardItemModel(tags))
    , tagsDelegate(new TextFieldItemDelegate(tags))
{
    colorPickerPopup->setColorCanBeDeselected(true);

    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    title->setTrailingIcon(u8"\U000F0766");
    UiHelper::initSpellingFor({
        title,
        heading,
        beats.constFirst(),
        storyDay,
        stamp,
    });
    storyDay->setCompleterActive(true);
    storyDay->completer()->setModel(storyDaysModel);
    autoNumbering->setChecked(true);
    customNumber->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    customNumber->hide();
    eatNumber->setChecked(true);
    eatNumber->hide();
    addTagButton->setIcon(u8"\U000F0415");
    tags->setRootIsDecorated(false);
    tags->setItemDelegate(tagsDelegate);
    tags->setModel(tagsModel);
    tagsDelegate->setHoverTrailingIcon(u8"\U000F01B4");
    tagsDelegate->setTrailingIconPickColor(true);

    auto cardInfoPageContent = new QWidget;
    content->setWidget(cardInfoPageContent);
    content->setWidgetResizable(true);
    contentLayout->setContentsMargins({});
    contentLayout->setSpacing(0);
    contentLayout->addWidget(title);
    contentLayout->addWidget(heading);
    contentLayout->addWidget(beats.constFirst());
    contentLayout->addWidget(storyDay);
    contentLayout->addWidget(stamp);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(numberingTitle, 1, Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(autoNumbering);
        contentLayout->addLayout(layout);
    }
    contentLayout->addWidget(customNumber);
    contentLayout->addWidget(eatNumber);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(tagsTitle, 1, Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(addTagButton);
        contentLayout->addLayout(layout);
    }
    contentLayout->addWidget(tags);
    contentLayout->addStretch();
    cardInfoPageContent->setLayout(contentLayout);
}

void ScreenplayItemParametersView::Implementation::initCardBeats()
{
    for (auto cardBeat : std::as_const(beats)) {
        cardBeat->setLabelVisible(false);
        cardBeat->setLabel({});
        cardBeat->setHelper({});
        cardBeat->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
        cardBeat->setTextColor(Ui::DesignSystem::color().onPrimary());
        cardBeat->setCustomMargins(
            { Ui::DesignSystem::layout().px24(), 0, Ui::DesignSystem::layout().px24(), 0 });
    }

    for (int index = 1; index < beats.size(); ++index) {
        QWidget::setTabOrder(beats[index - 1], beats[index]);
    }
    QWidget::setTabOrder(beats.constLast(), storyDay);

    beats.constFirst()->setLabelVisible(true);
    beats.constFirst()->setLabel(tr("Description"));
    beats.constFirst()->setCustomMargins({ Ui::DesignSystem::layout().px24(),
                                           Ui::DesignSystem::layout().px16(),
                                           Ui::DesignSystem::layout().px24(), 0 });
    beats.constLast()->setHelper(tr("Each line is a separate beat"));
}

void ScreenplayItemParametersView::Implementation::updateCustomNumberMargins()
{
    const auto isNumberingFixed = !autoNumbering->isVisible();
    customNumber->setCustomMargins({ Ui::DesignSystem::layout().px24(),
                                     (isNumberingFixed ? Ui::DesignSystem::layout().px16() : 0.0),
                                     Ui::DesignSystem::layout().px24(), 0.0 });
}

void ScreenplayItemParametersView::Implementation::updateTagsSize()
{
    tags->setFixedHeight(tagsModel->rowCount() * Ui::DesignSystem::treeOneLineItem().height());
}

void ScreenplayItemParametersView::Implementation::notifyTagsChanged()
{
    QVector<QPair<QString, QColor>> tags;
    for (int row = 0; row < tagsModel->rowCount(); ++row) {
        const auto tagIndex = tagsModel->index(row, 0);
        tags.append({ tagIndex.data(Qt::DisplayRole).toString(),
                      tagIndex.data(Qt::DecorationPropertyRole).value<QColor>() });
    }
    emit q->tagsChanged(tags);
}


// ****


ScreenplayItemParametersView::ScreenplayItemParametersView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->title);

    d->beats.constFirst()->installEventFilter(this);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);


    connect(d->title, &TextField::textChanged, this,
            [this] { emit titleChanged(d->title->text()); });
    connect(d->title, &TextField::trailingIconPressed, this, [this] {
        d->colorPickerPopup->showPopup(d->title, Qt::AlignBottom | Qt::AlignRight);
        d->colorPickerPopup->disconnect(this);
        connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged, this,
                &ScreenplayItemParametersView::colorChanged);
    });
    connect(d->heading, &TextField::textChanged, this,
            [this] { emit headingChanged(d->heading->text()); });
    connect(d->beats.constFirst(), &TextField::textChanged, this,
            [this] { emit beatChanged(0, d->beats.constFirst()->text()); });
    connect(d->storyDay, &TextField::textChanged, this,
            [this] { emit storyDayChanged(d->storyDay->text()); });
    connect(d->stamp, &TextField::textChanged, this,
            [this] { emit stampChanged(d->stamp->text()); });
    connect(d->autoNumbering, &Toggle::checkedChanged, this, [this](bool _checked) {
        const auto isCustomNumber = !_checked;
        d->customNumber->setVisible(isCustomNumber);
        d->eatNumber->setVisible(isCustomNumber);
    });
    auto notifyNumberingChanged = [this] {
        emit numberChanged(d->customNumber->text(), !d->autoNumbering->isChecked(),
                           d->eatNumber->isChecked());
    };
    connect(d->autoNumbering, &Toggle::checkedChanged, this, notifyNumberingChanged);
    connect(d->customNumber, &TextField::textChanged, this, notifyNumberingChanged);
    connect(d->eatNumber, &CheckBox::checkedChanged, this, notifyNumberingChanged);
    connect(d->addTagButton, &IconButton::clicked, this, [this] {
        auto tagItem = new QStandardItem;
        tagItem->setData(u8"\U000F0315", Qt::DecorationRole);
        d->tagsModel->appendRow(tagItem);
        d->updateTagsSize();
        d->tags->setCurrentIndex(d->tagsModel->indexFromItem(tagItem));
        d->tags->edit(d->tags->currentIndex());
    });
    connect(d->tags, &Tree::clicked, this, [this] {
        const auto clickPosition = d->tags->mapFromGlobal(QCursor::pos());
        if (d->tags->isEnabled() && d->tags->isOnItemTrilingIcon(clickPosition)) {
            d->tagsModel->removeRow(d->tags->currentIndex().row());
            d->notifyTagsChanged();
        }
    });
    connect(d->tagsDelegate, &TextFieldItemDelegate::closeEditor, this, [this] {
        const auto tagName = d->tags->currentIndex().data().toString();
        const auto tagColor
            = d->tags->currentIndex().data(Qt::DecorationPropertyRole).value<QColor>();
        if (tagName.isEmpty() && !tagColor.isValid()) {
            d->tagsModel->removeRow(d->tags->currentIndex().row());
        }
        d->notifyTagsChanged();
    });
}

ScreenplayItemParametersView::~ScreenplayItemParametersView() = default;

void ScreenplayItemParametersView::setItemType(ScreenplayItemType _type)
{
    if (d->currentItemType == _type) {
        return;
    }

    d->currentItemType = _type;
    switch (d->currentItemType) {
    case ScreenplayItemType::Folder: {
        d->title->setVisible(true);
        d->heading->setVisible(false);
        for (auto beat : std::as_const(d->beats)) {
            beat->setVisible(false);
        }
        d->storyDay->setVisible(false);
        d->stamp->setVisible(d->isStampVisible && true);
        d->numberingTitle->setVisible(false);
        d->autoNumbering->setVisible(false);
        d->customNumber->setVisible(false);
        d->eatNumber->setVisible(false);
        d->tagsTitle->setVisible(d->isTagsVisible && false);
        d->addTagButton->setVisible(d->isTagsVisible && false);
        d->tags->setVisible(d->isTagsVisible && false);
        break;
    }

    case ScreenplayItemType::Scene: {
        d->title->setVisible(true);
        d->heading->setVisible(true);
        for (auto beat : std::as_const(d->beats)) {
            beat->setVisible(true);
        }
        d->storyDay->setVisible(true);
        d->stamp->setVisible(d->isTagsVisible && true);
        d->numberingTitle->setVisible(true);
        d->autoNumbering->setVisible(true);
        d->customNumber->setVisible(false);
        d->eatNumber->setVisible(false);
        d->tagsTitle->setVisible(d->isTagsVisible && true);
        d->addTagButton->setVisible(d->isTagsVisible && true);
        d->tags->setVisible(d->isTagsVisible && true);
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }

    updateTranslations();
}

ScreenplayItemType ScreenplayItemParametersView::itemType() const
{
    return d->currentItemType;
}

void ScreenplayItemParametersView::setColor(const QColor& _color)
{
    d->colorPickerPopup->setSelectedColor(_color);
    const bool isColorSet = _color.isValid();
    d->title->setTrailingIcon(isColorSet ? u8"\U000F0765" : u8"\U000f0766");
    d->title->setTrailingIconColor(isColorSet ? _color : Ui::DesignSystem::color().onPrimary());
}

void ScreenplayItemParametersView::setTitle(const QString& _title)
{
    if (d->title->text() == _title) {
        return;
    }

    auto lastCursorPosition = d->title->textCursor().position();
    d->title->setText(_title);
    auto cursor = d->title->textCursor();
    cursor.setPosition(lastCursorPosition);
    d->title->setTextCursor(cursor);
}

void ScreenplayItemParametersView::setHeading(const QString& _heading)
{
    if (d->heading->text() == _heading) {
        return;
    }

    auto lastCursorPosition = d->heading->textCursor().position();
    d->heading->setText(_heading);
    auto cursor = d->heading->textCursor();
    cursor.setPosition(lastCursorPosition);
    d->heading->setTextCursor(cursor);
}

void ScreenplayItemParametersView::setBeats(const QVector<QString>& _beats)
{
    //
    // Настраиваем поля для отображения битов
    //
    const int beatsSize = std::max(1, _beats.size());
    if (d->beats.size() != beatsSize) {
        while (d->beats.size() > beatsSize) {
            auto beat = d->beats.takeLast();
            beat->disconnect();
            beat->deleteLater();
        }
        while (d->beats.size() < beatsSize) {
            auto beat = new TextField(d->content->widget());
            beat->installEventFilter(this);
            connect(beat, &TextField::textChanged, this,
                    [this, beat] { emit beatChanged(d->beats.indexOf(beat), beat->text()); });
            //
            // ... вставляем новые поля перед штампом и тэгами (заголовок + список)
            //
            d->contentLayout->insertWidget(d->contentLayout->indexOf(d->beats.constLast()) + 1,
                                           beat);
            d->beats.append(beat);
        }
        d->initCardBeats();
    }

    //
    // Помещаем биты в поля
    //
    if (_beats.isEmpty()) {
        d->beats.constFirst()->clear();
        return;
    }

    for (int index = 0; index < beatsSize; ++index) {
        if (d->beats[index]->text() == _beats[index]) {
            continue;
        }

        d->beats[index]->setText(_beats[index]);
    }
}

void ScreenplayItemParametersView::setStoryDay(const QString& _storyDay,
                                               const QVector<QString>& _storyDays)
{
    QStringList storyDaysToComplete = { _storyDays.begin(), _storyDays.end() };
    storyDaysToComplete.removeAll(_storyDay);
    if (d->storyDaysModel->stringList() != storyDaysToComplete) {
        d->storyDaysModel->setStringList(storyDaysToComplete);
    }

    if (d->storyDay->text() != _storyDay) {
        d->storyDay->setText(_storyDay);
    }
}

void ScreenplayItemParametersView::setStampVisible(bool _visible)
{
    d->isStampVisible = _visible;
    d->stamp->setVisible(_visible);
}

void ScreenplayItemParametersView::setStamp(const QString& _stamp)
{
    if (d->stamp->text() == _stamp) {
        return;
    }

    d->stamp->setText(_stamp);
}

void ScreenplayItemParametersView::setNumber(const QString& _number, bool _isCustom,
                                             bool _isEatNumber, bool _isLocked)
{
    if (d->autoNumbering->isChecked() == !_isCustom && d->customNumber->text() == _number
        && d->eatNumber->isChecked() == _isEatNumber && d->autoNumbering->isVisible() == !_isLocked
        && d->eatNumber->isVisible() == !_isLocked) {
        return;
    }

    d->numberingTitle->setVisible(!_isLocked);
    d->autoNumbering->setChecked(!_isCustom && !_isLocked);
    d->autoNumbering->setVisible(!_isLocked);
    d->customNumber->setText(_number);
    d->updateCustomNumberMargins();
    d->customNumber->setVisible(!d->autoNumbering->isChecked() || _isLocked);
    d->eatNumber->setChecked(_isEatNumber);
    d->eatNumber->setVisible(!d->autoNumbering->isChecked() && !_isLocked);
}

void ScreenplayItemParametersView::setTagsVisible(bool _visible)
{
    d->isTagsVisible = _visible;
    for (auto widget : std::vector<QWidget*>{
             d->tagsTitle,
             d->addTagButton,
             d->tags,
         }) {
        widget->setVisible(_visible);
    }
}

void ScreenplayItemParametersView::setTags(const QVector<QPair<QString, QColor>>& _tags)
{
    d->tagsModel->clear();
    for (const auto& tag : _tags) {
        auto tagItem = new QStandardItem(tag.first);
        tagItem->setData(u8"\U000F0315", Qt::DecorationRole);
        tagItem->setData(tag.second, Qt::DecorationPropertyRole);
        d->tagsModel->appendRow(tagItem);
    }

    d->updateTagsSize();
}

void ScreenplayItemParametersView::setReadOnly(bool _readOnly)
{
    const auto enabled = !_readOnly;
    d->title->setEnabled(enabled);
    d->heading->setEnabled(enabled);
    for (auto cardBeat : std::as_const(d->beats)) {
        cardBeat->setEnabled(enabled);
    }
    d->storyDay->setEnabled(enabled);
    d->stamp->setEnabled(enabled);
    d->addTagButton->setEnabled(enabled);
}

bool ScreenplayItemParametersView::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Биты
    //
    if (auto textField = qobject_cast<TextField*>(_watched); textField != nullptr
        && d->beats.contains(textField) && _event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(_event);
        switch (keyEvent->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return: {
            if (textField->text().isEmpty()) {
                break;
            }

            const int beatIndex = d->beats.indexOf(textField);
            const auto beatText = textField->text();
            const auto beatTextSplitPosition = textField->textCursor().position();
            emit beatChanged(beatIndex, beatText.mid(0, beatTextSplitPosition));
            emit beatAdded(beatIndex + 1);
            emit beatChanged(beatIndex + 1, beatText.mid(beatTextSplitPosition));
            d->beats[beatIndex + 1]->setFocus();
            break;
        }

        case Qt::Key_Backspace:
        case Qt::Key_Delete: {
            if (!textField->text().isEmpty() || d->beats.size() == 1) {
                break;
            }

            const int beatIndex = d->beats.indexOf(textField);
            QMetaObject::invokeMethod(
                this,
                [this, beatIndex, key = keyEvent->key()] {
                    d->beats[std::max(0,
                                      key == Qt::Key_Delete && beatIndex < d->beats.size()
                                          ? beatIndex
                                          : beatIndex - 1)]
                        ->setFocus();
                    emit beatRemoved(beatIndex);
                },
                Qt::QueuedConnection);

            break;
        }
        }
    }

    return Widget::eventFilter(_watched, _event);
}

void ScreenplayItemParametersView::updateTranslations()
{
    switch (d->currentItemType) {
    case ScreenplayItemType::Folder: {
        d->title->setLabel(tr("Heading"));
        d->title->setHelper({});
        d->title->setTrailingIconToolTip(tr("Select item color"));
        break;
    }

    case ScreenplayItemType::Scene: {
        d->title->setLabel(tr("Title"));
        d->title->setHelper(tr("Human-readable name (hidden in screenplay)"));
        d->title->setTrailingIconToolTip(tr("Select scene color"));
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }

    d->heading->setLabel(tr("Heading"));
    d->heading->setHelper(tr("Header text (visible in screenplay)"));
    d->initCardBeats();
    d->storyDay->setLabel(tr("Story day"));
    d->stamp->setLabel(tr("Stamp"));
    d->numberingTitle->setText(tr("Auto scene numbering"));
    d->customNumber->setLabel(tr("Scene number"));
    d->eatNumber->setText(tr("Skip the number of this scene"));
    d->eatNumber->setToolTip(
        tr("Skip logical number of this scene for determining the next scene number"));
    d->tagsTitle->setText(tr("Tags"));
    d->addTagButton->setToolTip(tr("Add tag to the selected card"));
    d->tagsDelegate->setLabel(tr("Tag name"));
}

void ScreenplayItemParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    d->colorPickerPopup->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->colorPickerPopup->setTextColor(Ui::DesignSystem::color().onPrimary());
    //
    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setTop(0);
    if (isLeftToRight()) {
        labelMargins.setRight(0);
    } else {
        labelMargins.setLeft(0);
    }
    labelMargins.setBottom(0);
    for (auto label : std::vector<Widget*>{
             d->numberingTitle,
             d->tagsTitle,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().primary());
        label->setTextColor(Ui::DesignSystem::color().onPrimary());
        label->setContentsMargins(labelMargins);
    }
    auto numberingTitleMargins = labelMargins;
    numberingTitleMargins.setTop(Ui::DesignSystem::layout().px8());
    d->numberingTitle->setContentsMargins(numberingTitleMargins);
    //
    for (auto iconButton : {
             d->addTagButton,
         }) {
        iconButton->setBackgroundColor(Ui::DesignSystem::color().primary());
        iconButton->setTextColor(Ui::DesignSystem::color().onPrimary());
    }
    const QMargins lastIconButtonMargins(isLeftToRight() ? 0 : Ui::DesignSystem::layout().px12(), 0,
                                         isLeftToRight() ? Ui::DesignSystem::layout().px12() : 0,
                                         0);
    d->addTagButton->setContentsMargins(lastIconButtonMargins);
    //
    for (auto textField : std::vector<TextField*>{
             d->title,
             d->heading,
             d->storyDay,
             d->stamp,
             d->customNumber,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
        textField->setTextColor(Ui::DesignSystem::color().onPrimary());
        textField->setCustomMargins({ Ui::DesignSystem::layout().px24(),
                                      Ui::DesignSystem::layout().px16(),
                                      Ui::DesignSystem::layout().px24(), 0.0 });
    }
    d->updateCustomNumberMargins();
    //
    d->storyDay->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->storyDay->completer()->setBackgroundColor(Ui::DesignSystem::color().background());
    //
    d->initCardBeats();
    //
    d->autoNumbering->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->autoNumbering->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->autoNumbering->setContentsMargins(
        isLeftToRight() ? 0 : Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
        isLeftToRight() ? Ui::DesignSystem::layout().px12() : 0, 0);
    //
    d->eatNumber->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->eatNumber->setTextColor(Ui::DesignSystem::color().onPrimary());
    //
    d->tags->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->tags->setTextColor(Ui::DesignSystem::color().onPrimary());

    d->contentLayout->setContentsMargins(0.0, Ui::DesignSystem::layout().px8(), 0.0,
                                         Ui::DesignSystem::layout().px24());
}

} // namespace Ui

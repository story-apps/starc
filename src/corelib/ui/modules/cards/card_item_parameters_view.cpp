#include "card_item_parameters_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/date_picker/date_picker_popup.h>
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

#include <QDateTime>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

class CardItemParametersView::Implementation
{
public:
    explicit Implementation(CardItemParametersView* _q);

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


    CardItemParametersView* q = nullptr;

    /**
     * @brief Текущий тип элемента для отображения
     */
    CardItemType currentItemType = CardItemType::Undefined;

    /**
     * @brief Биты текущего элемента для отображения
     */
    QVector<QString> currentItemBeats;

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
    TextField* description = nullptr;
    QVector<TextField*> beats;
    TextField* storyDay = nullptr;
    QStringListModel* allStoryDaysModel = nullptr;
    bool isStartDateTimeVisible = true;
    TextField* startDateTime = nullptr;
    DatePickerPopup* datePickerPopup = nullptr;
    bool isStampVisible = true;
    TextField* stamp = nullptr;
    bool isNumberingVisible = true;
    Subtitle2Label* numberingTitle = nullptr;
    Toggle* autoNumbering = nullptr;
    TextField* customNumber = nullptr;
    CheckBox* eatNumber = nullptr;
    bool isTagsVisible = true;
    Subtitle2Label* tagsTitle = nullptr;
    IconButton* addTagButton = nullptr;
    Tree* tags = nullptr;
    QStandardItemModel* tagsModel = nullptr;
    QStandardItemModel* allTagsModel = nullptr;
    TextFieldItemDelegate* tagsDelegate = nullptr;
};

CardItemParametersView::Implementation::Implementation(CardItemParametersView* _q)
    : q(_q)
    , colorPickerPopup(new ColorPickerPopup(_q))
    , content(new QScrollArea(_q))
    , contentLayout(new QVBoxLayout)
    , title(new TextField(content))
    , heading(new TextField(content))
    , description(new TextField(content))
    , beats({ new TextField(content) })
    , storyDay(new TextField(content))
    , allStoryDaysModel(new QStringListModel(storyDay))
    , startDateTime(new TextField(content))
    , datePickerPopup(new DatePickerPopup(content))
    , stamp(new TextField(content))
    , numberingTitle(new Subtitle2Label(content))
    , autoNumbering(new Toggle(content))
    , customNumber(new TextField(content))
    , eatNumber(new CheckBox(content))
    , tagsTitle(new Subtitle2Label(content))
    , addTagButton(new IconButton(content))
    , tags(new Tree(content))
    , tagsModel(new QStandardItemModel(tags))
    , allTagsModel(new QStandardItemModel(tags))
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
        description,
        beats.constFirst(),
        storyDay,
        startDateTime,
        stamp,
    });
    description->setEnterMakesNewLine(true);
    storyDay->setCompleterActive(true);
    storyDay->completer()->setModel(allStoryDaysModel);
    startDateTime->setTrailingIcon(u8"\U000F0B67");
    autoNumbering->setChecked(true);
    customNumber->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    customNumber->hide();
    eatNumber->setChecked(false);
    eatNumber->hide();
    addTagButton->setIcon(u8"\U000F0415");
    tags->setRootIsDecorated(false);
    tags->setItemDelegate(tagsDelegate);
    tags->setModel(tagsModel);
    tagsDelegate->setHoverTrailingIcon(u8"\U000F01B4");
    tagsDelegate->setTrailingIconPickColor(true);
    tagsDelegate->setCompletionModel(allTagsModel);

    auto cardInfoPageContent = new QWidget;
    content->setWidget(cardInfoPageContent);
    content->setWidgetResizable(true);
    contentLayout->setContentsMargins({});
    contentLayout->setSpacing(0);
    contentLayout->addWidget(title);
    contentLayout->addWidget(heading);
    contentLayout->addWidget(description);
    contentLayout->addWidget(beats.constFirst());
    contentLayout->addWidget(storyDay);
    contentLayout->addWidget(startDateTime);
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

void CardItemParametersView::Implementation::initCardBeats()
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

void CardItemParametersView::Implementation::updateCustomNumberMargins()
{
    const auto isNumberingFixed = !autoNumbering->isVisible();
    customNumber->setCustomMargins({ Ui::DesignSystem::layout().px24(),
                                     (isNumberingFixed ? Ui::DesignSystem::layout().px16() : 0.0),
                                     Ui::DesignSystem::layout().px24(), 0.0 });
}

void CardItemParametersView::Implementation::updateTagsSize()
{
    tags->setFixedHeight(tagsModel->rowCount() * Ui::DesignSystem::treeOneLineItem().height());
}

void CardItemParametersView::Implementation::notifyTagsChanged()
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


CardItemParametersView::CardItemParametersView(QWidget* _parent)
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
                &CardItemParametersView::colorChanged);
    });
    connect(d->heading, &TextField::textChanged, this,
            [this] { emit headingChanged(d->heading->text()); });
    connect(d->description, &TextField::textChanged, this,
            [this] { emit descriptionChanged(d->description->text()); });
    connect(d->beats.constFirst(), &TextField::textChanged, this,
            [this] { emit beatChanged(0, d->beats.constFirst()->text()); });
    connect(d->storyDay, &TextField::textChanged, this,
            [this] { emit storyDayChanged(d->storyDay->text()); });
    connect(d->startDateTime, &TextField::textChanged, this, [this] {
        auto datetime = QDateTime::fromString(d->startDateTime->text(), "dd.MM.yyyy hh:mm");
        if (!datetime.isValid()) {
            d->startDateTime->setError(tr("Value should be in format DD.MM.YYYY HH:MM"));
            return;
        }

        d->startDateTime->clearError();
        emit startDateTimeChanged(datetime);
    });
    auto initDatePicker = [this](TextField* _editor) {
        d->datePickerPopup->disconnect();

        connect(d->datePickerPopup, &DatePickerPopup::selectedDateChanged, _editor,
                [_editor](const QDate& _date) {
                    _editor->setText(_date.startOfDay().toString("dd.MM.yyyy hh:mm"));
                });

        if (auto date = QDate::fromString(_editor->text(), "dd.MM.yyyy hh:mm"); date.isValid()) {
            d->datePickerPopup->setCurrentDate(date);
            d->datePickerPopup->setSelectedDate(date);
        } else {
            d->datePickerPopup->setCurrentDate(QDate::currentDate());
            d->datePickerPopup->setSelectedDate({});
        }
        d->datePickerPopup->showPopup(_editor, Qt::AlignBottom | Qt::AlignRight);
    };
    for (auto dateTextField : {
             d->startDateTime,
         }) {
        connect(dateTextField, &TextField::trailingIconPressed, this,
                [dateTextField, initDatePicker] { initDatePicker(dateTextField); });
        connect(dateTextField, &TextField::textChanged, dateTextField, [dateTextField] {
            const auto sourceText = dateTextField->text();
            auto text = sourceText;
            //
            // Убираем не числа
            //
            for (int i = text.length() - 1; i >= 0; --i) {
                if (!text[i].isDigit() && !text[i].isSpace() && !text[i].isPunct()) {
                    text.remove(i, 1);
                }
            }
            //
            // Убираем лишние символы
            //
            if (text.length() > 16) {
                text = text.left(16);
            }
            //
            // Добавляем точки
            //
            for (int i = 0; i < text.length(); ++i) {
                if (i == 2 || i == 5) {
                    if (text[i] != '.') {
                        text.insert(i, '.');
                    }
                } else if (i == 10) {
                    if (text[i] != ' ') {
                        text.insert(i, ' ');
                    }
                } else if (i == 13) {
                    if (text[i] != ':') {
                        text.insert(i, ':');
                    }
                }
            }

            if (sourceText != text) {
                auto cursorPosition = dateTextField->textCursor().position();
                //
                dateTextField->setText(text);
                //
                if (sourceText.length() < text.length()) {
                    //
                    // ... если новый текст длиннее чем исходный, значит добавили тут символов и
                    //     курсор нужно сдвинуть на кол-во добавленных символов
                    //
                    cursorPosition += text.length() - sourceText.length();
                }
                auto cursor = dateTextField->textCursor();
                cursor.setPosition(cursorPosition);
                dateTextField->setTextCursor(cursor);
            }
        });
    }
    connect(d->stamp, &TextField::textChanged, this,
            [this] { emit stampChanged(d->stamp->text()); });
    connect(d->numberingTitle, &AbstractLabel::clicked, this, [this] {
        if (d->autoNumbering->isVisible() && d->autoNumbering->isEnabled()) {
            d->autoNumbering->toggle();
        }
    });
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

CardItemParametersView::~CardItemParametersView() = default;

void CardItemParametersView::setItemType(CardItemType _type)
{
    if (d->currentItemType == _type) {
        return;
    }

    d->currentItemType = _type;
    switch (d->currentItemType) {
    case CardItemType::Folder: {
        d->title->setVisible(true);
        d->heading->setVisible(false);
        d->description->setVisible(true);
        for (auto beat : std::as_const(d->beats)) {
            beat->setVisible(false);
        }
        d->storyDay->setVisible(false);
        d->startDateTime->setVisible(false);
        d->stamp->setVisible(d->isStampVisible && true);
        d->numberingTitle->setVisible(d->isNumberingVisible && false);
        d->autoNumbering->setVisible(d->isNumberingVisible && false);
        d->customNumber->setVisible(d->isNumberingVisible && false);
        d->eatNumber->setVisible(d->isNumberingVisible && false);
        d->tagsTitle->setVisible(d->isTagsVisible && false);
        d->addTagButton->setVisible(d->isTagsVisible && false);
        d->tags->setVisible(d->isTagsVisible && false);
        break;
    }

    case CardItemType::Scene: {
        d->title->setVisible(true);
        d->heading->setVisible(true);
        d->description->setVisible(false);
        for (auto beat : std::as_const(d->beats)) {
            beat->setVisible(true);
        }
        d->storyDay->setVisible(true);
        d->startDateTime->setVisible(d->isStartDateTimeVisible && true);
        d->stamp->setVisible(d->isStampVisible && true);
        d->numberingTitle->setVisible(d->isNumberingVisible && true);
        d->autoNumbering->setVisible(d->isNumberingVisible && true);
        d->customNumber->setVisible(d->isNumberingVisible && false);
        d->eatNumber->setVisible(d->isNumberingVisible && false);
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

CardItemType CardItemParametersView::itemType() const
{
    return d->currentItemType;
}

void CardItemParametersView::setColor(const QColor& _color)
{
    d->colorPickerPopup->setSelectedColor(_color);
    const bool isColorSet = _color.isValid();
    d->title->setTrailingIcon(isColorSet ? u8"\U000F0765" : u8"\U000f0766");
    d->title->setTrailingIconColor(isColorSet ? _color : Ui::DesignSystem::color().onPrimary());
}

void CardItemParametersView::setTitle(const QString& _title)
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

void CardItemParametersView::setHeading(const QString& _heading)
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

void CardItemParametersView::setDescription(const QString& _description)
{
    if (d->description->text() == _description) {
        return;
    }

    auto lastCursorPosition = d->description->textCursor().position();
    d->description->setText(_description);
    auto cursor = d->description->textCursor();
    cursor.setPosition(lastCursorPosition);
    d->description->setTextCursor(cursor);
}

void CardItemParametersView::setBeats(const QVector<QString>& _beats)
{
    d->currentItemBeats = _beats;

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
            connect(beat, &TextField::textChanged, this, [this, beat] {
                const auto beatIndex = d->beats.indexOf(beat);
                if (d->currentItemBeats.isEmpty()
                    || d->currentItemBeats.at(beatIndex) != beat->text()) {
                    emit beatChanged(beatIndex, beat->text());
                }
            });
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
        //
        // Если в элементе нет битов, то блокируем редактор бита, чтобы предотвратить испускание
        // сигнала о том, что бит изменился, т.к. это приведёт к созданию пустого бита внутри сцены
        //
        QSignalBlocker signalBlocker(d->beats.constFirst());
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

void CardItemParametersView::setStoryDay(const QString& _storyDay,
                                         const QVector<QString>& _allStoryDays)
{
    QStringList storyDaysToComplete = { _allStoryDays.begin(), _allStoryDays.end() };
    storyDaysToComplete.removeAll(_storyDay);
    if (d->allStoryDaysModel->stringList() != storyDaysToComplete) {
        d->allStoryDaysModel->setStringList(storyDaysToComplete);
    }

    if (d->storyDay->text() != _storyDay) {
        d->storyDay->setText(_storyDay);
    }
}

void CardItemParametersView::setStartDateTimeVisible(bool _visible)
{
    d->isStartDateTimeVisible = _visible;
    d->startDateTime->setVisible(d->isStartDateTimeVisible);
}

void CardItemParametersView::setStartDateTime(const QDateTime& _startDateTime)
{
    if (d->startDateTime->text() == _startDateTime.toString("dd.MM.yyyy hh:mm")) {
        return;
    }

    d->startDateTime->setText(_startDateTime.toString("dd.MM.yyyy hh:mm"));
}

void CardItemParametersView::setStampVisible(bool _visible)
{
    d->isStampVisible = _visible;
    d->stamp->setVisible(d->isStampVisible);
}

void CardItemParametersView::setStamp(const QString& _stamp)
{
    if (d->stamp->text() == _stamp) {
        return;
    }

    d->stamp->setText(_stamp);
}

void CardItemParametersView::setNumberingVisible(bool _visible)
{
    d->isNumberingVisible = _visible;
    for (auto widget : std::vector<QWidget*>{ d->numberingTitle, d->autoNumbering, d->customNumber,
                                              d->eatNumber }) {
        widget->setVisible(d->isNumberingVisible);
    }
}

void CardItemParametersView::setNumber(const QString& _number, bool _isCustom, bool _isEatNumber,
                                       bool _isLocked)
{
    if (!d->isNumberingVisible) {
        return;
    }

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

void CardItemParametersView::setTagsVisible(bool _visible)
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

void CardItemParametersView::setTags(const QVector<QPair<QString, QColor>>& _tags,
                                     const QVector<QPair<QString, QColor>>& _allTags)
{
    d->tagsModel->clear();
    for (const auto& tag : _tags) {
        auto tagItem = new QStandardItem(tag.first);
        tagItem->setData(u8"\U000F0315", Qt::DecorationRole);
        tagItem->setData(tag.second, Qt::DecorationPropertyRole);
        d->tagsModel->appendRow(tagItem);
    }

    d->allTagsModel->clear();
    for (const auto& tag : _allTags) {
        if (_tags.contains(tag)) {
            continue;
        }

        auto tagItem = new QStandardItem(tag.first);
        tagItem->setData(u8"\U000F0315", Qt::DecorationRole);
        tagItem->setData(tag.second, Qt::DecorationPropertyRole);
        d->allTagsModel->appendRow(tagItem);
    }

    d->updateTagsSize();
}

void CardItemParametersView::setReadOnly(bool _readOnly)
{
    const auto enabled = !_readOnly;
    d->title->setEnabled(enabled);
    d->heading->setEnabled(enabled);
    d->description->setEnabled(enabled);
    for (auto cardBeat : std::as_const(d->beats)) {
        cardBeat->setEnabled(enabled);
    }
    d->storyDay->setEnabled(enabled);
    d->startDateTime->setEnabled(enabled);
    d->stamp->setEnabled(enabled);
    d->autoNumbering->setEnabled(enabled);
    d->customNumber->setEnabled(enabled);
    d->eatNumber->setEnabled(enabled);
    d->addTagButton->setEnabled(enabled);
    d->tags->setEnabled(enabled);
}

bool CardItemParametersView::eventFilter(QObject* _watched, QEvent* _event)
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

void CardItemParametersView::updateTranslations()
{
    switch (d->currentItemType) {
    case CardItemType::Folder: {
        d->title->setLabel(tr("Heading"));
        d->title->setHelper({});
        d->title->setTrailingIconToolTip(tr("Select item color"));
        break;
    }

    case CardItemType::Scene: {
        d->title->setLabel(tr("Title"));
        d->title->setHelper(tr("Human-readable name (hidden in text)"));
        d->title->setTrailingIconToolTip(tr("Select scene color"));
        break;
    }

    default: {
        break;
    }
    }

    d->heading->setLabel(tr("Heading"));
    d->heading->setHelper(tr("Header text (visible in text)"));
    d->description->setLabel(tr("Description"));
    d->initCardBeats();
    d->storyDay->setLabel(tr("Story day"));
    d->startDateTime->setLabel(tr("Start date and time"));
    d->startDateTime->setPlaceholderText(tr("DD.MM.YYYY HH:MM"));
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

void CardItemParametersView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
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
             d->description,
             d->storyDay,
             d->startDateTime,
             d->stamp,
             d->customNumber,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
        textField->setTextColor(Ui::DesignSystem::color().onPrimary());
        textField->setCustomMargins({ Ui::DesignSystem::layout().px24(),
                                      Ui::DesignSystem::compactLayout().px16(),
                                      Ui::DesignSystem::layout().px24(), 0.0 });
    }
    d->updateCustomNumberMargins();
    //
    d->storyDay->completer()->setTextColor(Ui::DesignSystem::color().onBackground());
    d->storyDay->completer()->setBackgroundColor(Ui::DesignSystem::color().background());
    //
    d->datePickerPopup->setBackgroundColor(DesignSystem::color().background());
    d->datePickerPopup->setTextColor(DesignSystem::color().onBackground());
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

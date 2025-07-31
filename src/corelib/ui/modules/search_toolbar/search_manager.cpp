#include "search_manager.h"

#include "search_toolbar.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QTextBlock>


namespace BusinessLayer {

class SearchManager::Implementation
{
public:
    Implementation(QWidget* _parent, BaseTextEdit* _textEdit);

    /**
     * @brief Получить тип блока, в котором будем искать
     */
    TextParagraphType searchInType() const;

    /**
     * @brief Найти текст в заданном направлении
     */
    void find(bool _backward = false);
    void findText(bool _backward);
    void findNumber();


    /**
     * @brief Панель поиска
     */
    Ui::SearchToolbar* toolbar = nullptr;

    /**
     * @brief Текстовый редактор, где будет осуществляться поиск
     */
    BaseTextEdit* textEdit = nullptr;

    /**
     * @brief Последний искомый текст
     */
    QString m_lastSearchText;

    QVector<TextParagraphType> blockTypes;
};

SearchManager::Implementation::Implementation(QWidget* _parent, BaseTextEdit* _textEdit)
    : toolbar(new Ui::SearchToolbar(_parent))
    , textEdit(_textEdit)
{
    toolbar->hide();
}

TextParagraphType SearchManager::Implementation::searchInType() const
{
    return blockTypes.value(toolbar->searchInType(), TextParagraphType::Undefined);
}

void SearchManager::Implementation::find(bool _backward)
{
    const QString searchText = toolbar->searchText();
    if (searchText.isEmpty()) {
        //
        // Сохраняем искомый текст
        //
        m_lastSearchText = searchText;

        //
        // Возвращаем фокус в панель поиска
        //
        toolbar->refocus();

        return;
    }

    if (searchText.startsWith("#")) {
        findNumber();
    } else {
        findText(_backward);
    }

    //
    // Возвращаем фокус в панель поиска
    //
    toolbar->refocus();
}

void SearchManager::Implementation::findText(bool _backward)
{
    const QString searchText = toolbar->searchText();

    //
    // Поиск осуществляется от позиции курсора
    //
    TextCursor cursor = textEdit->textCursor();
    if (searchText != m_lastSearchText) {
        cursor.setPosition(cursor.selectionInterval().from);
    }

    //
    // Настроить направление поиска
    //
    QTextDocument::FindFlags findFlags;
    if (_backward) {
        findFlags |= QTextDocument::FindBackward;
    }
    //
    // Учёт регистра
    //
    if (toolbar->isCaseSensitive()) {
        findFlags |= QTextDocument::FindCaseSensitively;
    }

    //
    // Поиск
    //
    bool searchRestarted = false;
    bool restartSearch = false;
    do {
        restartSearch = false;
        cursor = textEdit->document()->find(searchText, cursor, findFlags);
        const auto searchType = searchInType();
        auto blockType = TextBlockStyle::forBlock(cursor.block());
        if (!cursor.isNull()) {
            if (cursor.block().isVisible()
                && (searchType == TextParagraphType::Undefined || searchType == blockType)) {
                textEdit->ensureCursorVisible(cursor);
            } else {
                restartSearch = true;
            }
        } else {
            //
            // Если достигнут конец, или начало документа зацикливаем поиск, если это первый проход
            //
            if (searchRestarted == false) {
                searchRestarted = true;
                cursor = textEdit->textCursor();
                cursor.movePosition(_backward ? QTextCursor::End : QTextCursor::Start);
                cursor = textEdit->document()->find(searchText, cursor, findFlags);
                blockType = TextBlockStyle::forBlock(cursor.block());
                if (!cursor.isNull()) {
                    if (cursor.block().isVisible()
                        && (searchType == TextParagraphType::Undefined
                            || searchType == blockType)) {
                        textEdit->ensureCursorVisible(cursor);
                    } else {
                        restartSearch = true;
                    }
                }
            } else {
                break;
            }
        }
    } while (!cursor.block().isVisible() || restartSearch);

    //
    // Сохраняем искомый текст
    //
    m_lastSearchText = searchText;
}

void SearchManager::Implementation::findNumber()
{
    //
    // Искомая строка идёт сразу за символом решётки
    //
    const QString searchText = toolbar->searchText().mid(1);
    if (searchText.isEmpty()) {
        return;
    }

    //
    // Поиск осуществляется от начала документа
    //
    TextCursor cursor(textEdit->document());
    auto block = cursor.block();
    while (block.isValid()) {
        do {
            if (block.userData() == nullptr) {
                break;
            }

            const auto blockData = static_cast<TextBlockData*>(block.userData());
            if (blockData == nullptr) {
                break;
            }

            if (blockData->item() == nullptr
                || blockData->item()->type() != TextModelItemType::Text) {
                break;
            }

            const auto textItem = static_cast<TextModelTextItem*>(blockData->item());
            if (textItem->paragraphType() != TextParagraphType::SceneHeading
                || textItem->parent() == nullptr
                || textItem->parent()->type() != TextModelItemType::Group) {
                break;
            }

            const auto groupItem = static_cast<TextModelGroupItem*>(textItem->parent());
            if (groupItem->number()->text.startsWith(searchText)) {
                cursor.setPosition(block.position());
                textEdit->setTextCursor(cursor);
                textEdit->ensureCursorVisible(cursor);
                return;
            }
        }
        once;

        block = block.next();
    }
}


// ****


SearchManager::SearchManager(QWidget* _parent, BaseTextEdit* _textEdit)
    : QObject(_parent)
    , d(new Implementation(_parent, _textEdit))
{
    connect(d->toolbar, &Ui::SearchToolbar::reactivatePressed, this,
            &SearchManager::activateSearhToolbar);
    connect(d->toolbar, &Ui::SearchToolbar::closePressed, this,
            &SearchManager::hideToolbarRequested);
    connect(d->toolbar, &Ui::SearchToolbar::focusTextRequested, _parent,
            qOverload<>(&QWidget::setFocus));
    connect(d->toolbar, &Ui::SearchToolbar::findTextRequested, this, [this] { d->find(); });
    connect(d->toolbar, &Ui::SearchToolbar::findNextRequested, this, [this] { d->find(); });
    connect(d->toolbar, &Ui::SearchToolbar::findPreviousRequested, this, [this] {
        const bool backward = true;
        d->find(backward);
    });
    connect(d->toolbar, &Ui::SearchToolbar::replaceOnePressed, this, [this] {
        auto cursor = d->textEdit->textCursor();
        if (!cursor.hasSelection()) {
            d->find();
            return;
        }

        const QString searchText = d->toolbar->searchText();
        const bool selectedTextEqual = d->toolbar->isCaseSensitive()
            ? cursor.selectedText() == searchText
            : TextHelper::smartToLower(cursor.selectedText())
                == TextHelper::smartToLower(searchText);
        if (selectedTextEqual) {
            cursor.insertText(d->toolbar->replaceText());
            d->find();
        }
    });
    connect(d->toolbar, &Ui::SearchToolbar::replaceAllPressed, this, [this] {
        const QString searchText = d->toolbar->searchText();
        const QString replaceText = d->toolbar->replaceText();
        if (searchText == replaceText) {
            return;
        }

        d->find();
        auto cursor = d->textEdit->textCursor();
        int firstCursorPosition = cursor.selectionStart();
        const auto caseSensitive
            = d->toolbar->isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
        const int diffBefore = replaceText.startsWith(searchText, caseSensitive)
            ? 0
            : replaceText.indexOf(searchText, 0, caseSensitive);
        const int diffAfter = replaceText.size() - diffBefore - searchText.size();
        firstCursorPosition += diffBefore;
        cursor.beginEditBlock();
        while (cursor.hasSelection()) {
            cursor.insertText(replaceText);

            d->find();
            cursor = d->textEdit->textCursor();

            //
            // Прерываем случай, когда пользователь пытается заменить слово без учёта регистра
            // на такое же, например "иван" на "Иван" или когда заменяемое слово является частью
            // нового, но т.к. поиск производится без учёта регистра, он зацикливается
            //
            if (cursor.selectionStart() == firstCursorPosition) {
                break;
            }

            //
            // Корректируем начальную позицию поиска, для корректного завершения при втором проходе
            // по документу
            //
            if (cursor.selectionStart() < firstCursorPosition) {
                firstCursorPosition += diffBefore + diffAfter;
            }
        }
        cursor.endEditBlock();
    });
}

SearchManager::~SearchManager() = default;

void SearchManager::activateSearhToolbar()
{
    if (const auto selectedText = d->textEdit->textCursor().selectedText();
        !selectedText.isEmpty()) {
        d->toolbar->setSearchText(selectedText);
    }
    d->toolbar->setFocus();
}

void SearchManager::setSearchInBlockTypes(
    const QVector<QPair<QString, TextParagraphType>>& _blockTypes)
{
    d->blockTypes.clear();
    QStringList list;
    for (const auto& type : _blockTypes) {
        list.append(type.first);
        d->blockTypes.append(type.second);
    }
    d->toolbar->setPopupStringList(list);
}

void SearchManager::setReadOnly(bool _readOnly)
{
    d->toolbar->setReadOnly(_readOnly);
}

Widget* SearchManager::toolbar() const
{
    return d->toolbar;
}

} // namespace BusinessLayer

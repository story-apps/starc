#include "search_manager.h"

#include "search_toolbar.h"

#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

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
    void findText(bool _backward = false);


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

void SearchManager::Implementation::findText(bool _backward)
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

    //
    // Возвращаем фокус в панель поиска
    //
    toolbar->refocus();
}


// ****


SearchManager::SearchManager(QWidget* _parent, BaseTextEdit* _textEdit)
    : QObject(_parent)
    , d(new Implementation(_parent, _textEdit))
{
    connect(d->toolbar, &Ui::SearchToolbar::closePressed, this,
            &SearchManager::hideToolbarRequested);
    connect(d->toolbar, &Ui::SearchToolbar::focusTextRequested, _parent,
            qOverload<>(&QWidget::setFocus));
    connect(d->toolbar, &Ui::SearchToolbar::findTextRequested, this, [this] { d->findText(); });
    connect(d->toolbar, &Ui::SearchToolbar::findNextRequested, this, [this] { d->findText(); });
    connect(d->toolbar, &Ui::SearchToolbar::findPreviousRequested, this, [this] {
        const bool backward = true;
        d->findText(backward);
    });
    connect(d->toolbar, &Ui::SearchToolbar::replaceOnePressed, this, [this] {
        auto cursor = d->textEdit->textCursor();
        if (!cursor.hasSelection()) {
            d->findText();
            return;
        }

        const QString searchText = d->toolbar->searchText();
        const bool selectedTextEqual = d->toolbar->isCaseSensitive()
            ? cursor.selectedText() == searchText
            : TextHelper::smartToLower(cursor.selectedText())
                == TextHelper::smartToLower(searchText);
        if (selectedTextEqual) {
            cursor.insertText(d->toolbar->replaceText());
            d->findText();
        }
    });
    connect(d->toolbar, &Ui::SearchToolbar::replaceAllPressed, this, [this] {
        const QString searchText = d->toolbar->searchText();
        const QString replaceText = d->toolbar->replaceText();
        if (searchText == replaceText) {
            return;
        }

        d->findText();
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

            d->findText();
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
        d->toolbar->selectSearchText();
    } else {
        d->toolbar->selectSearchText();
    }
}

void SearchManager::setSearchInBlockTypes(
    const QVector<QPair<QString, TextParagraphType>>& _blockTypes)
{
    d->blockTypes.clear();
    QStringList list;
    foreach (QPair type, _blockTypes) {
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

#include "novel_outline_search_manager.h"

#include "novel_outline_edit.h"
#include "novel_outline_search_toolbar.h"

#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/novel_template.h>
#include <utils/helpers/text_helper.h>

#include <QTextBlock>


namespace BusinessLayer {

class NovelOutlineSearchManager::Implementation
{
public:
    Implementation(QWidget* _parent, Ui::NovelOutlineEdit* _textEdit);

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
    Ui::NovelOutlineSearchToolbar* toolbar = nullptr;

    /**
     * @brief Текстовый редактор, где будет осуществляться поиск
     */
    Ui::NovelOutlineEdit* textEdit = nullptr;

    /**
     * @brief Последний искомый текст
     */
    QString m_lastSearchText;
};

NovelOutlineSearchManager::Implementation::Implementation(QWidget* _parent,
                                                          Ui::NovelOutlineEdit* _textEdit)
    : toolbar(new Ui::NovelOutlineSearchToolbar(_parent))
    , textEdit(_textEdit)
{
    toolbar->hide();
}

TextParagraphType NovelOutlineSearchManager::Implementation::searchInType() const
{
    switch (toolbar->searchInType()) {
    default:
        return TextParagraphType::Undefined;
    case 1:
        return TextParagraphType::SceneHeading;
    case 2:
        return TextParagraphType::BeatHeading;
    }
}

void NovelOutlineSearchManager::Implementation::findText(bool _backward)
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
            if (searchType == TextParagraphType::Undefined || searchType == blockType) {
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
                    if (searchType == TextParagraphType::Undefined || searchType == blockType) {
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


NovelOutlineSearchManager::NovelOutlineSearchManager(QWidget* _parent,
                                                     Ui::NovelOutlineEdit* _textEdit)
    : QObject(_parent)
    , d(new Implementation(_parent, _textEdit))
{
    connect(d->toolbar, &Ui::NovelOutlineSearchToolbar::closePressed, this,
            &NovelOutlineSearchManager::hideToolbarRequested);
    connect(d->toolbar, &Ui::NovelOutlineSearchToolbar::focusTextRequested, _parent,
            qOverload<>(&QWidget::setFocus));
    connect(d->toolbar, &Ui::NovelOutlineSearchToolbar::findTextRequested, this,
            [this] { d->findText(); });
    connect(d->toolbar, &Ui::NovelOutlineSearchToolbar::findNextRequested, this,
            [this] { d->findText(); });
    connect(d->toolbar, &Ui::NovelOutlineSearchToolbar::findPreviousRequested, this, [this] {
        const bool backward = true;
        d->findText(backward);
    });
    connect(d->toolbar, &Ui::NovelOutlineSearchToolbar::replaceOnePressed, this, [this] {
        const QString searchText = d->toolbar->searchText();
        auto cursor = d->textEdit->textCursor();
        bool selectedTextEqual = d->toolbar->isCaseSensitive()
            ? cursor.selectedText() == searchText
            : TextHelper::smartToLower(cursor.selectedText())
                == TextHelper::smartToLower(searchText);
        if (selectedTextEqual) {
            cursor.insertText(d->toolbar->replaceText());
            d->findText();
        }
    });
    connect(d->toolbar, &Ui::NovelOutlineSearchToolbar::replaceAllPressed, this, [this] {
        const QString searchText = d->toolbar->searchText();
        const QString replaceText = d->toolbar->replaceText();
        if (searchText == replaceText) {
            return;
        }

        d->findText();
        auto cursor = d->textEdit->textCursor();
        int firstCursorPosition = cursor.selectionStart();
        const int diffBefore
            = replaceText.startsWith(searchText) ? 0 : replaceText.indexOf(searchText);
        const int diffAfter
            = replaceText.size() - replaceText.indexOf(searchText) - searchText.size();
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

NovelOutlineSearchManager::~NovelOutlineSearchManager() = default;

Widget* NovelOutlineSearchManager::toolbar() const
{
    return d->toolbar;
}

void NovelOutlineSearchManager::setReadOnly(bool _readOnly)
{
    d->toolbar->setReadOnly(_readOnly);
}

} // namespace BusinessLayer

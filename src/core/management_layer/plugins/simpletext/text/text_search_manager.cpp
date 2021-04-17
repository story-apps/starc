#include "text_search_manager.h"

#include "text_edit.h"
#include "text_search_toolbar.h"

#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/text_template.h>

#include <utils/helpers/text_helper.h>

#include <QTextBlock>


namespace BusinessLayer {

class TextSearchManager::Implementation {
public:
    Implementation(QWidget* _parent, Ui::TextEdit* _textEdit);

    /**
     * @brief Найти текст в заданном направлении
     */
    void findText(bool _backward = false);


    /**
     * @brief Панель поиска
     */
    Ui::TextSearchToolbar* toolbar = nullptr;

    /**
     * @brief Текстовый редактор, где будет осуществляться поиск
     */
    Ui::TextEdit* textEdit = nullptr;

    /**
     * @brief Последний искомый текст
     */
    QString m_lastSearchText;
};

TextSearchManager::Implementation::Implementation(QWidget* _parent, Ui::TextEdit* _textEdit)
    : toolbar(new Ui::TextSearchToolbar(_parent)),
      textEdit(_textEdit)
{
    toolbar->hide();
}

void TextSearchManager::Implementation::findText(bool _backward)
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
        if (!cursor.isNull()) {
            textEdit->ensureCursorVisible(cursor);
        } else {
            //
            // Если достигнут конец, или начало документа зацикливаем поиск, если это первый проход
            //
            if (searchRestarted == false) {
                searchRestarted = true;
                cursor = textEdit->textCursor();
                cursor.movePosition(_backward ? QTextCursor::End : QTextCursor::Start);
                cursor = textEdit->document()->find(searchText, cursor, findFlags);
                if (!cursor.isNull()) {
                    textEdit->ensureCursorVisible(cursor);
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


TextSearchManager::TextSearchManager(QWidget* _parent, Ui::TextEdit* _textEdit)
    : QObject(_parent),
      d(new Implementation(_parent, _textEdit))
{
    connect(d->toolbar, &Ui::TextSearchToolbar::closePressed,
            this, &TextSearchManager::hideToolbarRequested);
    connect(d->toolbar, &Ui::TextSearchToolbar::findTextRequested, this, [this] {
        d->findText();
    });
    connect(d->toolbar, &Ui::TextSearchToolbar::findNextRequested, this, [this] {
        d->findText();
    });
    connect(d->toolbar, &Ui::TextSearchToolbar::findPreviousRequested, this, [this] {
        const bool backward = true;
        d->findText(backward);
    });
    connect(d->toolbar, &Ui::TextSearchToolbar::replaceOnePressed, this, [this] {
        const QString searchText = d->toolbar->searchText();
        auto cursor = d->textEdit->textCursor();
        bool selectedTextEqual = d->toolbar->isCaseSensitive()
                                 ? cursor.selectedText() == searchText
                                 : TextHelper::smartToLower(cursor.selectedText()) == TextHelper::smartToLower(searchText);
        if (selectedTextEqual) {
            cursor.insertText(d->toolbar->replaceText());
            d->findText();
        }
    });
    connect(d->toolbar, &Ui::TextSearchToolbar::replaceAllPressed, this, [this] {
        const QString searchText = d->toolbar->searchText();
        const QString replaceText = d->toolbar->replaceText();
        if (searchText == replaceText) {
            return;
        }

        const int diffSize = replaceText.size() - searchText.size();
        d->findText();
        auto cursor = d->textEdit->textCursor();
        cursor.beginEditBlock();
        int firstCursorPosition = cursor.selectionStart();
        while (cursor.hasSelection()) {
            cursor.insertText(replaceText);
            d->findText();
            cursor = d->textEdit->textCursor();

            //
            // Корректируем начальную позицию поиска, для корректного завершения при втором проходе по документу
            //
            if (cursor.selectionStart() < firstCursorPosition) {
                firstCursorPosition += diffSize;
            }

            //
            // Прерываем случай, когда пользователь пытается заменить слово без учёта регистра
            // на такое же, например "иван" на "Иван" или когда заменяемое слово является частью нового,
            // но т.к. поиск производится без учёта регистра, он зацикливается
            //
            if (cursor.selectionStart() == firstCursorPosition) {
                break;
            }
        }
        cursor.endEditBlock();
    });
}

TextSearchManager::~TextSearchManager() = default;

Widget* TextSearchManager::toolbar() const
{
    return d->toolbar;
}

}

#pragma once

#include <QString>


/*

https://screencraft.org/2013/01/23/50-great-screenwriting-quotes/
https://www.brainyquote.com/topics/screenwriter-quotes
https://scriptlarva.wordpress.com/inspiration-for-writers/
https://www.writersdigest.com/editor-blogs/there-are-no-rules/72-of-the-best-quotes-about-writing
https://gointothestory.blcklst.com/free-screenwriting-resource-writers-on-writing-quotes-e2f6f0d1f710
https://www.la-screenwriter.com/screenwriting-quotes/

 */

/**
 * @brief Генератор цитат великих
 */
class QuotesHelper
{
public:
    /**
     * @brief Цитата известного автора
     */
    struct Quote {
        Quote() : index(kInvalidIndex) {}
        Quote(int _index, const QString& _text, const QString& _author)
            : index(_index), text(_text), author(_author) {}

        int index;
        QString text;
        QString author;
    };

    /**
     * @brief Сформировать цитату, или извлечь цитату под заданным индексом (необходимо при изменении языка)
     */
    static Quote generateQuote(int _index = kInvalidIndex);

private:
    static const int kInvalidIndex = -1;
};

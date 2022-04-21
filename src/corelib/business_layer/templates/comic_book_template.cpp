#include "comic_book_template.h"

#include "screenplay_template.h"

#include <QCoreApplication>


namespace BusinessLayer {

ComicBookTemplate::ComicBookTemplate()
    : TextTemplate()
{
}

ComicBookTemplate::ComicBookTemplate(const QString& _fromFile)
    : TextTemplate(_fromFile)
{
}

QString ComicBookTemplate::name() const
{
    if (id() == "world") {
        return QCoreApplication::translate(
            "BusinessLayer::ComicBookTemplate",
            "International template (page: A4; font: Courier Prime)");
    } else if (id() == "us") {
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate",
                                           "US template (page: Letter; font: Courier Prime)");
    } else {
        return TextTemplate::name();
    }
}

bool ComicBookTemplate::canMergeParagraph() const
{
    return false;
}

} // namespace BusinessLayer

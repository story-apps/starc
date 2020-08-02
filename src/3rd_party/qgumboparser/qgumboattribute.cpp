#include "qgumboattribute.h"
#include <stdexcept>


QGumboAttribute::QGumboAttribute(const char* n, const char* value) {
    if (!n)
        throw std::invalid_argument("attribute name can't be empty");

    name_ = QString::fromUtf8(n);

    if (value)
        value_ = QString::fromUtf8(value);
}

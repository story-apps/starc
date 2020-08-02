#ifndef QGUMBOATTRIBUTE_H
#define QGUMBOATTRIBUTE_H

#include <QString>

class QGumboAttribute {
public:
    QGumboAttribute(const char* utf8name, const char* utf8value);

    const QString& name() const { return name_; }
    const QString& value() const { return value_; }

private:
    QString name_;
    QString value_;
};

#endif // QGUMBOATTRIBUTE_H

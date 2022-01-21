#include "email_validator.h"

#include <QRegularExpressionValidator>


bool EmailValidator::isValid(const QString& _email)
{
    //
    // Для валидатора нужна неконстантная ссылка,
    // поэтому копируем
    //
    QString toCheck = _email;
    int pos = 0;
    QRegularExpressionValidator validator(QRegularExpression("[^\\s]+@[^\\s]{2,}\\.[^\\s]{2,}"));
    if (validator.validate(toCheck, pos) != QValidator::Acceptable) {
        return false;
    } else {
        return true;
    }
}

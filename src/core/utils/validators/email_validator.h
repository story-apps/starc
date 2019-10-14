#pragma once

#include <QRegExpValidator>
#include <QString>


/**
 * @brief Класс - сборник валидоторов под разные нужды
 */
class EmailValidator
{
public:
    /**
	 * @brief Проверить валидность адреса электронной почты
	 */
    static bool isValid(const QString& _email)
	{
		//
		// Для валидатора нужна неконстантная ссылка,
		// поэтому копируем
		//
		QString toCheck = _email;

        QRegExpValidator validator(QRegExp("[^\\s]+@[^\\s]{2,}\\.[^\\s]{2,}"));
		int pos = 0;
		if (validator.validate(toCheck, pos) != QValidator::Acceptable) {
			return false;
		} else {
			return true;
		}
    }
};

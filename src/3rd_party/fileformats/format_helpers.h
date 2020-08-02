#ifndef FORMAT_HELPERS
#define FORMAT_HELPERS

#include <QTextFormat>


/**
 * @brief Вспомогательная функция, для возможности размещения цвета в хэше
 */
inline uint qHash(const QColor& _key) { return qHash(_key.name()); }


/**
 * @brief Специфичные параметры и методы для формата DOCX
 */
class Docx
{
public:
	/**
	 * @brief Дополнительные параметры стиля блока (QTextCharFormat)
	 * @note Параметры касающиеся комментирования содержат списки строк
	 */
	enum Property {
		IsForeground = QTextFormat::UserProperty + 1,
		IsBackground,
		IsHighlight,
		IsComment,
		Comments,
		CommentsAuthors,
		CommentsDates
	};

	/**
	 * @brief Получить название цвета выделения
	 */
	static QString highlightColorName(const QColor& _color) {
		return highlightColors().value(_color, "none");
	}

	/**
	 * @brief Получить цвет выделения по названию
	 */
	static QColor highlightColor(const QString& _name) {
		return highlightColors().key(_name);
	}

	/**
	 * @brief Получить цвет комментария для заданного комментатора
	 */
	static QColor commentColor(const QString& _author) {
		static QStringList s_authors;
		if (!s_authors.contains(_author)) {
			s_authors.append(_author);
		}

		const int MAX_COLORS = 7;
		int commentatorId = s_authors.indexOf(_author);
		while (commentatorId >= MAX_COLORS) {
			commentatorId -= MAX_COLORS;
		}

		QColor result;
		switch (commentatorId) {
			default:
			case 0: result = QColor("#ff0101"); break;
			case 1: result = QColor("#0101ff"); break;
			case 2: result = QColor("#800080"); break;
			case 3: result = QColor("#c85100"); break;
			case 4: result = QColor("#008380"); break;
			case 5: result = QColor("#800000"); break;
			case 6: result = QColor("#a4a000"); break;
		}
		return result;
	}

private:
	static QHash<QColor, QString> highlightColors() {
		static QHash<QColor, QString> s_highlightColors;
		if (s_highlightColors.isEmpty()) {
			s_highlightColors.insert(Qt::black, "black");
			s_highlightColors.insert(Qt::blue, "blue");
			s_highlightColors.insert(Qt::cyan, "cyan");
			s_highlightColors.insert(Qt::green, "green");
			s_highlightColors.insert(Qt::magenta, "magenta");
			s_highlightColors.insert(Qt::red, "red");
			s_highlightColors.insert(Qt::yellow, "yellow");
			s_highlightColors.insert(Qt::white, "white");
			s_highlightColors.insert(Qt::darkBlue, "darkBlue");
			s_highlightColors.insert(Qt::darkCyan, "darkCyan");
			s_highlightColors.insert(Qt::darkGreen, "darkGreen");
			s_highlightColors.insert(Qt::darkMagenta, "darkMagenta");
			s_highlightColors.insert(Qt::darkRed, "darkRed");
			s_highlightColors.insert(Qt::darkYellow, "darkYellow");
			s_highlightColors.insert(Qt::darkGray, "darkGray");
			s_highlightColors.insert(Qt::lightGray, "lightGray");
		}
		return s_highlightColors;
	}
};

#endif // FORMAT_HELPERS


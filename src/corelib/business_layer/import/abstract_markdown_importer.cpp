#include "abstract_markdown_importer.h"

#include <business_layer/model/text/text_model_xml.h>

#include <QPair>
#include <QRegularExpression>
#include <QSet>
#include <QVector>
#include <QXmlStreamWriter>

namespace BusinessLayer {

namespace {

/**
 * @brief Информация о типах выделения в тексте
 */
struct SelectionTypeInText {
    int position = -1;
    QString formatSymbols;
};

/**
 * @brief Информация о форматах в тексте
 */
struct FormatInText {
    QSet<QLatin1String> attributes;
    int from = -1;
    int length = 0;
};

} // namespace

class AbstractMarkdownImporter::Implementation
{
public:
    Implementation(const QMap<QString, QLatin1String>& _selectionTypes,
                   const QRegularExpression& _selectionTypeChecker, const QString& _capturedGroup);


    /**
     * @brief Форматы в тексте
     */
    QVector<FormatInText> formatsInText;

    /**
     * @brief Возможные типы выделения текста в markdown или fountain
     */
    const QMap<QString, QLatin1String> markdownSelectionTypes;

    /**
     * @brief Регулярное выражение для поиска любого типа выделения текста
     */
    const QRegularExpression& selectionTypeChecker;

    /**
     * @brief Имя группы захвата форматных символов
     */
    const QString capturedGroup;
};

AbstractMarkdownImporter::Implementation::Implementation(
    const QMap<QString, QLatin1String>& _selectionTypes,
    const QRegularExpression& _selectionTypeChecker, const QString& _capturedGroup)
    : markdownSelectionTypes(_selectionTypes)
    , selectionTypeChecker(_selectionTypeChecker)
    , capturedGroup(_capturedGroup)
{
}


// ****


AbstractMarkdownImporter::AbstractMarkdownImporter(
    const QMap<QString, QLatin1String>& _selectionTypes,
    const QRegularExpression& _selectionTypeChecker, const QString& _capturedGroup)
    : d(new Implementation(_selectionTypes, _selectionTypeChecker, _capturedGroup))
{
}

AbstractMarkdownImporter::~AbstractMarkdownImporter() = default;

void AbstractMarkdownImporter::movePreviousTypes(int _position, int _offset) const
{
    for (auto& type : d->formatsInText) {
        if (_position < type.from) {
            type.from -= _offset;
        }
    }
}

//
// Чтобы не возникало путаницы, обозначим термины, которые будут использоваться при описании того,
// что здесь происходит: ФОРМАТНЫЕ СИМВОЛЫ - символовы, которые делают текст жирным, подчеркнутым,
// зачеркнутым или курсивным, ТИП ВЫДЕЛЕНИЯ - жирный, курсив, подчеркнутый или зачеркнутый - что-то
// одно (задается XML-аттрибутом), а ФОРМАТОМ будем называть комбинацию из нескольких типов
// выделения
//
void AbstractMarkdownImporter::collectSelectionTypes(QString& _paragraphText) const
{
    //
    // Собираем типы выделения
    //
    QVector<SelectionTypeInText> selectionTypes;
    QRegularExpressionMatch match = d->selectionTypeChecker.match(_paragraphText);
    while (match.hasMatch()) {
<<<<<<< HEAD
        const SelectionTypeInText type
=======
        SelectionTypeInText type
>>>>>>> 9202b8bb (Abstract markdown importer for novels and screenplays)
            = { match.capturedStart(d->capturedGroup), match.captured(d->capturedGroup) };
        selectionTypes.append(type);
        int position = match.capturedEnd(d->capturedGroup);
        if (position > 1) {
            --position;
        }
        match = d->selectionTypeChecker.match(_paragraphText, position);
    }

    //
    // Отбираем только те, у которых есть закрывающая пара
    //
    QVector<SelectionTypeInText> pairedTypes;
    while (!selectionTypes.isEmpty()) {
<<<<<<< HEAD
        const auto first = selectionTypes.takeFirst();
        bool isPaired = false;
        for (int i = 0; i != selectionTypes.size(); ++i) {
=======
        const auto first = selectionTypes.first();
        bool isPaired = false;
        for (int i = 1; i != selectionTypes.size(); ++i) {
>>>>>>> 9202b8bb (Abstract markdown importer for novels and screenplays)
            if (first.formatSymbols == selectionTypes[i].formatSymbols) {
                //
                // ... дополнительно проверим, что между форматными символами есть расстояние,
                // иначе это не типы выделения, а просто набор символов (напр. ****)
                //
                SelectionTypeInText minType, maxType;
                if (first.position < selectionTypes[i].position) {
                    minType = first;
                    maxType = selectionTypes[i];
                } else {
                    minType = selectionTypes[i];
                    maxType = first;
                }
                int distance = maxType.position - (minType.position + minType.formatSymbols.size());
                if (distance != 0) {
                    pairedTypes.append(first);
                    pairedTypes.append(selectionTypes[i]);
                }

                selectionTypes.remove(i);
<<<<<<< HEAD
=======
                selectionTypes.removeFirst();
>>>>>>> 9202b8bb (Abstract markdown importer for novels and screenplays)
                isPaired = true;
                break;
            }
        }
        if (!isPaired) {
            selectionTypes.removeFirst();
        }
    }

    //
    // Удаляем форматные символы и корректируем позиции типов выделения
    //
    for (int i = 0; i != pairedTypes.size(); ++i) {
        _paragraphText.remove(pairedTypes[i].position, pairedTypes[i].formatSymbols.size());
        for (int j = 0; j != pairedTypes.size(); ++j) {
            if (pairedTypes[j].position > pairedTypes[i].position) {
                pairedTypes[j].position -= pairedTypes[i].formatSymbols.size();
            }
        }
    }

    //
    // Сортируем по позиции в тексте
    //
    std::sort(pairedTypes.begin(), pairedTypes.end(),
<<<<<<< HEAD
              [](const SelectionTypeInText& _lhs, const SelectionTypeInText& _rhs) {
                  return _lhs.position < _rhs.position;
=======
              [](const SelectionTypeInText& type1, const SelectionTypeInText& type2) {
                  return type1.position < type2.position;
>>>>>>> 9202b8bb (Abstract markdown importer for novels and screenplays)
              });

    //
    // Парсим
    //
    FormatInText currentFormat;
    for (int i = 0; i != pairedTypes.size(); ++i) {
        //
        // Собираем все типы выделения в текущей позиции
        //
        QSet<QLatin1String> typesInPosition;
        int count = 0;
        while (i + count < pairedTypes.size()
               && pairedTypes[i].position == pairedTypes[i + count].position) {
            typesInPosition.insert(
                d->markdownSelectionTypes.value(pairedTypes[i + count].formatSymbols));
            ++count;
        }
        i += count - 1;

        //
        // Если ли уже есть открытый формат
        //
        if (!currentFormat.attributes.isEmpty()) {
            //
            // ... закрываем его (пишем его длину и добавляем в список форматов)
            //
            currentFormat.length = pairedTypes[i].position - currentFormat.from;
            d->formatsInText.append(currentFormat);

            //
            // ... заполняем новый формат в ту же переменную
            //
            currentFormat.from = pairedTypes[i].position;
            currentFormat.length = 0;

            //
            // ... смотрим, какие типы выделения закрываются в текущей позиции, а какие
            // открываются
            //
            for (const auto& type : typesInPosition) {
                if (currentFormat.attributes.contains(type)) {
                    //
                    // ... закрывающиеся удаляем из текущего формата
                    //
                    currentFormat.attributes.remove(type);
                } else {
                    //
                    // ... а открывающиеся - добавляем
                    //
                    currentFormat.attributes.insert(type);
                }
            }

        }
        //
        // Если открытого формата нет, то откроем
        //
        else {
            currentFormat.attributes = typesInPosition;
            currentFormat.from = pairedTypes[i].position;
        }
    }
}

void AbstractMarkdownImporter::writeSelectionTypes(QXmlStreamWriter& _writer) const
{
    if (!d->formatsInText.isEmpty()) {
        _writer.writeStartElement(xml::kFormatsTag);
        for (const auto& type : d->formatsInText) {
            _writer.writeStartElement(xml::kFormatTag);
            _writer.writeAttribute(xml::kFromAttribute, QString::number(type.from));
            _writer.writeAttribute(xml::kLengthAttribute, QString::number(type.length));
            for (const auto& attribute : type.attributes) {
                _writer.writeAttribute(attribute, "true");
            }
            _writer.writeEndElement(); // format
        }
        _writer.writeEndElement(); // formats
        d->formatsInText.clear();
    }
}

} // namespace BusinessLayer

#include "abstract_fountain_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/text_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QStack>

namespace BusinessLayer {

namespace {

/**
 * @brief Регулярное выражение для поиска любого типа выделения текста
 */
static const QRegularExpression kSelectionTypeChecker("(^|[^\\\\])(?<format>(\\*\\*)|(\\*)|(_))");

/**
 * @brief Имя группы захвата форматных символов
 */
static const QString kCapturedGroup("format");

/**
 * @brief Возможные типы выделения текста в fountain
 */
static const QMap<QString, QLatin1String> kFountainSelectionTypes({
    { "**", xml::kBoldAttribute },
    { "*", xml::kItalicAttribute },
    { "_", xml::kUnderlineAttribute },
});

static const QString kDoubleWhitespace = QLatin1String("  ");

/**
 * @brief С чего может начинаться название сцены
 */
static QStringList sceneHeadingsDictionary()
{
    return {
        QCoreApplication::translate("BusinessLayer::FountainImporter", "INT"),
        QCoreApplication::translate("BusinessLayer::FountainImporter", "EXT"),
        QCoreApplication::translate("BusinessLayer::FountainImporter", "EST"),
        QCoreApplication::translate("BusinessLayer::FountainImporter", "INT./EXT"),
        QCoreApplication::translate("BusinessLayer::FountainImporter", "INT/EXT"),
        QCoreApplication::translate("BusinessLayer::FountainImporter", "EXT./INT"),
        QCoreApplication::translate("BusinessLayer::FountainImporter", "EXT/INT"),
        QCoreApplication::translate("BusinessLayer::FountainImporter", "I/E"),
    };
}

//
// TODO: Пока нигде не используется, но возможно пригодится потом
//
/**
 * @brief Ключи титульной страницы
 */
QHash<QString, QString> titleKeysDictionary()
{
    return { std::make_pair(QString("Title"), QString("name")),
             std::make_pair(QString("Author"), QString("author")),
             std::make_pair(QString("Authors"), QString("author")),
             std::make_pair(QString("Draft date"), QString("year")),
             std::make_pair(QString("Copyright"), QString("year")),
             std::make_pair(QString("Contact"), QString("contacts")),
             std::make_pair(QString("Credit"), QString("genre")),
             std::make_pair(QString("Source"), QString("additional_info")) };
}

} // namespace

class AbstractFountainImporter::Implementation
{
public:
    explicit Implementation(AbstractFountainImporter* _q, bool _includeBeats);

    /**
     * @brief Добавление блока
     */
    void appendBlock(const QString& _paragraphText, TextParagraphType _type,
                     QXmlStreamWriter& _writer);

    /**
     * @brief Добавление комментариев к блоку
     */
    void appendComments(QXmlStreamWriter& _writer);


    AbstractFountainImporter* q = nullptr;

    /**
     * @brief Зашли ли мы уже в бит
     */
    bool alreadyInBeat = false;

    /**
     * @brief Зашли ли мы уже в сцену
     */
    bool alreadyInScene = false;

    /**
     * @brief Начало позиции в блоке для потенциальной будущей редакторской заметки
     */
    unsigned noteStartPos = 0;

    /**
     * @brief Длина потенциальной будущей редакторской заметки
     */
    unsigned noteLen = 0;

    /**
     * @brief Идет ли сейчас редакторская заметка
     */
    bool isNotation = false;

    /**
     * @brief Идет ли сейчас комментарий
     */
    bool isCommenting = false;
    /**
     * @brief Является ли текущий блок первым
     */
    bool isFirstBlock = true;

    /**
     * @brief Текст последнего сохранённого блока
     */
    QString lastBlockText;

    /**
     * @brief Текст блока
     */
    QString blockText;

    /**
     * @brief Текст редакторской заметки
     */
    QString note;

    /**
     * @brief Редакторская заметка к текущему блоку
     * 		  tuple содержит комментарий, позиция и длина области редакторской заметки
     */
    QVector<std::tuple<QString, unsigned, unsigned>> notes;

    /**
     * @brief Нужно ли включать биты в импортируемый документ
     */
    const bool shouldIncludeBeats = false;
};

AbstractFountainImporter::Implementation::Implementation(AbstractFountainImporter* _q,
                                                         bool _includeBeats)
    : q(_q)
    , shouldIncludeBeats(_includeBeats)
{
}

void AbstractFountainImporter::Implementation::appendBlock(const QString& _paragraphText,
                                                           TextParagraphType _type,
                                                           QXmlStreamWriter& _writer)
{
    int leadSpaceCount = 0;
    QString paragraphText = _paragraphText;
    while (!paragraphText.isEmpty() && paragraphText.startsWith(" ")) {
        ++leadSpaceCount;
        paragraphText = paragraphText.mid(1);
    }

    //
    // Формируем блок сценария
    //
    switch (_type) {
    case TextParagraphType::SequenceHeading: {
        if (alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
            alreadyInBeat = false; // вышли из бита
        }

        if (alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
            alreadyInScene = false; // вышли из сцены
        }

        _writer.writeStartElement(toString(TextFolderType::Sequence));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        break;
    }

    case TextParagraphType::SequenceFooter: {
        if (alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
            alreadyInBeat = false; // вышли из бита
        }

        if (alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
            alreadyInScene = false; // вышли из сцены
        }

        _writer.writeEndElement(); // контент текущей папки
        _writer.writeEndElement(); // текущая папка
        break;
    }

    case TextParagraphType::SceneHeading: {
        if (alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
            alreadyInBeat = false; // вышли из бита
        }

        if (alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }

        alreadyInScene = true; // вошли в новую сцену

        _writer.writeStartElement(toString(TextGroupType::Scene));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        break;
    }

    case TextParagraphType::BeatHeading: {
        if (shouldIncludeBeats) {
            if (alreadyInBeat) {
                _writer.writeEndElement(); // контент предыдущего бита
                _writer.writeEndElement(); // предыдущий бит
            }

            alreadyInBeat = true; // вошли в новый бит

            _writer.writeStartElement(toString(TextGroupType::Beat));
            _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
            _writer.writeStartElement(xml::kContentTag);
            break;
        } else {
            return;
        }
    }

    default:
        break;
    }
    _writer.writeStartElement(toString(_type));
    _writer.writeStartElement(xml::kValueTag);
    _writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
    _writer.writeEndElement(); // value

    //
    // Пишем форматирование, если оно есть
    //
    q->writeSelectionTypes(_writer);

    lastBlockText = blockText;

    //
    // Первый блок в тексте может встретиться лишь однажды
    //
    if (isFirstBlock) {
        isFirstBlock = false;
    }

    //
    // Не закрываем блок, чтобы можно было добавить редакторских заметок
    //
}

void AbstractFountainImporter::Implementation::appendComments(QXmlStreamWriter& _writer)
{
    if (notes.isEmpty()) {
        return;
    }

    _writer.writeStartElement(xml::kReviewMarksTag);

    for (int i = 0; i != notes.size(); ++i) {
        int endPos = std::get<2>(notes[i]);
        if (endPos == 0) {
            endPos = lastBlockText.length();
        }

        if (i != 0) {
            _writer.writeEndElement(); // review mark
        }
        _writer.writeStartElement(xml::kReviewMarkTag);
        _writer.writeAttribute(xml::kFromAttribute, QString::number(std::get<1>(notes[i])));
        _writer.writeAttribute(xml::kLengthAttribute, QString::number(endPos));
        _writer.writeAttribute(xml::kBackgroundColorAttribute, "#FFD302");

        _writer.writeStartElement(xml::kCommentTag);
        _writer.writeAttribute(xml::kAuthorAttribute, "fountain author");
        _writer.writeAttribute(xml::kDateAttribute,
                               QDateTime::currentDateTime().toString(Qt::ISODate));
        _writer.writeCDATA(TextHelper::toHtmlEscaped(std::get<0>(notes[i])));
        _writer.writeEndElement(); // comment
    }

    _writer.writeEndElement(); // review mark
    _writer.writeEndElement(); // review marks

    notes.clear();
}


// ****


AbstractFountainImporter::AbstractFountainImporter(bool _includeBeats)
    : AbstractMarkdownImporter(kFountainSelectionTypes, kSelectionTypeChecker, kCapturedGroup)
    , d(new Implementation(this, _includeBeats))
{
}

AbstractFountainImporter::~AbstractFountainImporter() = default;

QString AbstractFountainImporter::documentText(const QString& _text) const
{
    QString result;

    if (_text.simplified().isEmpty()) {
        return result;
    }

    //
    // Читаем plain text
    //
    // ... и пишем в документ
    //
    QXmlStreamWriter writer(&result);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Сформируем список строк, содержащий текст документа
    //
    QVector<QString> paragraphs;
    bool isTitle = false;
    bool isFirstLine = true;
    for (QString str : QString(_text).remove('\r').split("\n")) {
        //
        // Если первая строка содержит ':', то в начале идет титульная страница,
        // которую мы обрабатываем не здесь
        //
        if (isFirstLine) {
            isFirstLine = false;
            if (str.contains(':')) {
                isTitle = true;
            }
        }

        if (isTitle) {
            //
            // Титульная страница заканчивается пустой строкой
            //
            if (str.simplified().isEmpty()) {
                isTitle = false;
            }
        } else {
            if (str == kDoubleWhitespace) {
                //
                // Если строка состоит из 2 пробелов, то это нужно сохранить
                // Используется для многострочных диалогов с пустыми строками
                //
                paragraphs.push_back(kDoubleWhitespace);
            } else {
                paragraphs.push_back(str.simplified());
            }
        }
    }

    preprocessBlock(paragraphs, writer);

    //
    // Закроем документ
    //
    writer.writeEndElement();
    writer.writeEndDocument();

    return result;
}

QVector<QPair<TextParagraphType, QString>> AbstractFountainImporter::parapraphsForDocuments(
    const ImportOptions& _options) const
{
    QVector<QPair<TextParagraphType, QString>> documents;

    //
    // Открываем файл
    //
    QFile fountainFile(_options.filePath);
    if (!fountainFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Читаем plain text
    //
    QString scriptText = fountainFile.readAll();

    //
    // Сформируем список строк, содержащий текст сценария
    //
    QVector<QString> paragraphs;
    bool isTitle = false;
    bool isFirstLine = true;
    for (const auto& str : scriptText.remove('\r').split("\n")) {
        //
        // Если первая строка содержит ':', то в начале идет титульная страница,
        // которую мы обрабатываем не здесь
        //
        if (isFirstLine) {
            isFirstLine = false;
            if (str.contains(':')) {
                isTitle = true;
            }
        }

        if (isTitle) {
            //
            // Титульная страница заканчивается пустой строкой
            //
            if (str.simplified().isEmpty()) {
                isTitle = false;
            }
        } else {
            if (str == kDoubleWhitespace) {
                //
                // Если строка состоит из 2 пробелов, то это нужно сохранить
                // Используется для многострочных диалогов с пустыми строками
                //
                paragraphs.push_back(kDoubleWhitespace);
            } else {
                paragraphs.push_back(str.simplified());
            }
        }
    }

    const int paragraphsCount = paragraphs.size();
    auto prevBlockType = TextParagraphType::Undefined;
    TextParagraphType blockType;
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    for (int i = 0; i != paragraphsCount; ++i) {
        if (paragraphs[i].isEmpty()) {
            continue;
        }

        blockType = TextParagraphType::Undefined;
        QString paragraphText;

        switch (paragraphs[i][0].toLatin1()) {
        case '.': {
            blockType = TextParagraphType::SceneHeading;
            //
            // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
            //
            int sharpPos = paragraphs[i].size();
            if (paragraphs[i].endsWith("#")) {
                sharpPos = paragraphs[i].lastIndexOf('#', paragraphs[i].size() - 2);
            }
            if (sharpPos == -1) {
                sharpPos = paragraphs[i].size();
            }
            paragraphText = paragraphs[i].mid(1, sharpPos - 1);
            break;
        }

        case '@': {
            blockType = TextParagraphType::Character;
            paragraphText = paragraphs[i].mid(1);
            break;
        }

        case '!':
        case '>':
        case '=':
        case '~':
        case '#': {
            continue;
        }

        default: {
            bool startsWithHeading = false;
            for (const QString& sceneHeading : sceneHeadingsDictionary()) {
                if (paragraphs[i].startsWith(sceneHeading)) {
                    startsWithHeading = true;
                    break;
                }
            }

            if (startsWithHeading && i + 1 < paragraphsCount && paragraphs[i + 1].isEmpty()) {
                //
                // Если начинается с одного из времен действия, а после обязательно пустая строка
                // Значит это заголовок сцены
                //
                blockType = TextParagraphType::SceneHeading;

                //
                // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
                //
                int sharpPos = paragraphs[i].size();
                if (paragraphs[i].endsWith("#")) {
                    sharpPos = paragraphs[i].lastIndexOf('#', paragraphs[i].size() - 2);
                }
                if (sharpPos == -1) {
                    sharpPos = paragraphs[i].size();
                }
                paragraphText = paragraphs[i].left(sharpPos);
            } else if (paragraphs[i].startsWith("[[") && paragraphs[i].endsWith("]]")) {
                continue;
            } else if (paragraphs[i].startsWith("/*")) {
                continue;
            } else if (paragraphs[i] == TextHelper::smartToUpper(paragraphs[i]) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && paragraphs[i + 1].isEmpty() && paragraphs[i].endsWith("TO:")) {
                continue;
            } else if (paragraphs[i].startsWith("(") && paragraphs[i].endsWith(")")
                       && (prevBlockType == TextParagraphType::Character
                           || prevBlockType == TextParagraphType::Dialogue)) {
                continue;
            } else if (paragraphs[i] == TextHelper::smartToUpper(paragraphs[i]) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && !paragraphs[i + 1].isEmpty()) {
                //
                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
                // пустая Значит это имя персонажа (для реплики)
                //
                blockType = TextParagraphType::Character;
                if (paragraphs[i].endsWith("^")) {
                    //
                    // Двойной диалог, который мы пока что не умеем обрабатывать
                    //
                    paragraphText = paragraphs[i].left(paragraphs[i].size() - 1);
                } else {
                    paragraphText = paragraphs[i];
                }
            } else {
                continue;
            }
        }
        }
        documents.append(QPair(blockType, paragraphText));
    }
    return documents;
}

void AbstractFountainImporter::processBlock(const QString& _paragraphText,
                                                            TextParagraphType _type,
                                                            QXmlStreamWriter& _writer) const
{
    //
    // Начинается новая сущность
    //
    if (!d->isNotation && !d->isCommenting) {
        d->blockText.reserve(_paragraphText.size());

        //
        // Добавим комментарии к предыдущему блоку
        //
        d->appendComments(_writer);

        d->noteLen = 0;
        d->noteStartPos = 0;
    }
    //
    // Продолжается комментарий или заметка
    //
    else {
        if (d->isCommenting) {
            d->blockText.append(QChar::LineSeparator);
        } else {
            d->note.append(QChar::LineFeed);
        }
    }

    //
    // Комментарии и заметки
    //
    char prevSymbol = '\0';
    for (int i = 0; i != _paragraphText.size(); ++i) {
        //
        // Если предыдущий символ - \, то просто добавим текущий
        //
        if (prevSymbol == '\\') {
            if (d->isNotation) {
                d->note.append(_paragraphText[i]);
            } else {
                d->blockText.append(_paragraphText[i]);
            }
            //
            // Раз мы добавляем символ через '\',
            // то он не должен участвовать в какой-либо обработке
            //
            prevSymbol = '\0';
            continue;
        }

        char curSymbol = _paragraphText[i].toLatin1();
        switch (curSymbol) {
        case '\\': {
            movePreviousTypes(i, 1);
            break;
        }

        case '/': {
            if (prevSymbol == '*' && d->isCommenting) {
                //
                // Заканчивается комментирование
                //
                d->isCommenting = false;
                d->noteStartPos += d->noteLen;
                d->noteLen = d->blockText.size();

                //
                // Закроем предыдущий блок, добавим текущий
                //
                if (!d->lastBlockText.isEmpty()) {
                    _writer.writeEndElement();
                }
                d->appendBlock(d->blockText.left(d->blockText.size()), TextParagraphType::InlineNote,
                            _writer);
                d->blockText.clear();
            } else {
                if (d->isNotation) {
                    d->note.append('/');
                } else {
                    d->blockText.append('/');
                }
            }
            break;
        }

        case '*': {
            if (prevSymbol == '/' && !d->isCommenting && !d->isNotation) {
                //
                // Начинается комментирование
                //
                d->isCommenting = true;
                d->noteStartPos += d->noteLen;
                d->noteLen = d->blockText.size() - 1;

                //
                // Закроем предыдущий блок и, если комментирование начинается в середние текущего
                // блока то добавим этот текущий блок
                //
                if (d->blockText.size() != 1) {
                    _writer.writeEndElement();
                    d->appendBlock(d->blockText.left(d->blockText.size() - 1), _type, _writer);
                    d->appendComments(_writer);
                    d->notes.clear();
                }
                d->blockText.clear();
            } else {
                if (d->isNotation) {
                    d->note.append('*');
                }
            }
            break;
        }

        case '[': {
            if (prevSymbol == '[' && !d->isCommenting && !d->isNotation) {
                //
                // Начинается редакторская заметка
                //
                d->isNotation = true;
                d->noteLen = d->blockText.size() - 1 - d->noteStartPos;
                d->blockText = d->blockText.left(d->blockText.size() - 1);
            } else {
                if (d->isNotation) {
                    d->note.append('[');
                } else {
                    d->blockText.append('[');
                }
            }
            break;
        }

        case ']': {
            if (prevSymbol == ']' && d->isNotation) {
                //
                // Закончилась редакторская заметка. Добавим ее в список редакторских заметок к
                // текущему блоку
                //
                d->isNotation = false;
                d->notes.append(std::make_tuple(d->note.left(d->note.size() - 1), d->noteStartPos, d->noteLen));
                d->noteStartPos += d->noteLen;
                d->noteLen = 0;
                d->note.clear();
            } else {
                if (d->isNotation) {
                    d->note.append(']');
                } else {
                    d->blockText.append(']');
                }
            }
            break;
        }

        default: {
            //
            // Самый обычный символ
            //
            if (d->isNotation) {
                d->note.append(_paragraphText[i]);
            } else {
                d->blockText.append(_paragraphText[i]);
            }
            break;
        }
        }

        prevSymbol = curSymbol;
    }

    if (!d->isNotation && !d->isCommenting) {
        //
        // Если блок действительно закончился
        //
        d->noteLen += d->blockText.size() - d->noteStartPos;

        //
        // Добавим текущий блок
        //
        if (!d->blockText.isEmpty() || _type == TextParagraphType::SequenceFooter) {
            //
            // ... но перед добавлением закроем предыдущий блок
            //
            if (!d->isFirstBlock) {
                _writer.writeEndElement();
            }

            d->appendBlock(d->blockText, _type, _writer);
        }
        d->blockText.clear();
    }
}

} // namespace BusinessLayer

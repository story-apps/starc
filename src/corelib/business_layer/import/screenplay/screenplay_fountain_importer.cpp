#include "screenplay_fountain_importer.h"

#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStack>
#include <QXmlStreamWriter>

#include <set>

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

/**
 * @brief С чего может начинаться название сцены
 */
QStringList sceneHeadingsDictionary()
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

const QString kDoubleWhitespace = QLatin1String("  ");

} // namespace

class ScreenplayFountainImporter::Implementation
{
public:
    explicit Implementation(ScreenplayFountainImporter* _q);

    /**
     * @brief Обработка конкретного блока перед его добавлением
     */
    void processBlock(const QString& _paragraphText, TextParagraphType _type,
                      QXmlStreamWriter& _writer);

    /**
     * @brief Добавление блока
     */
    void appendBlock(const QString& _paragraphText, TextParagraphType _type,
                     QXmlStreamWriter& _writer);

    /**
     * @brief Добавление комментариев к блоку
     */
    void appendComments(QXmlStreamWriter& _writer);


    ScreenplayFountainImporter* q = nullptr;

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
     * @brief Зашли ли мы уже в сцену
     */
    bool alreadyInScene = false;

    /**
     * @brief Зашли ли мы уже в бит
     */
    bool alreadyInBeat = false;

    /**
     * @brief Текст блока
     */
    QString blockText;

    /**
     * @brief Текст последнего сохранённого блока
     */
    QString lastBlockText;

    /**
     * @brief Текст редакторской заметки
     */
    QString note;

    /**
     * @brief Редакторская заметка к текущему блоку
     * 		  tuple содержит комментарий, позиция и длина области редакторской заметки
     */
    QVector<std::tuple<QString, unsigned, unsigned>> notes;
};

ScreenplayFountainImporter::Implementation::Implementation(ScreenplayFountainImporter* _q)
    : q(_q)
{
}

void ScreenplayFountainImporter::Implementation::processBlock(const QString& _paragraphText,
                                                              TextParagraphType _type,
                                                              QXmlStreamWriter& _writer)
{
    //
    // Начинается новая сущность
    //
    if (!isNotation && !isCommenting) {
        blockText.reserve(_paragraphText.size());

        //
        // Добавим комментарии к предыдущему блоку
        //
        appendComments(_writer);

        noteLen = 0;
        noteStartPos = 0;
    }
    //
    // Продолжается комментарий или заметка
    //
    else {
        if (isCommenting) {
            blockText.append(QChar::LineSeparator);
        } else {
            note.append(QChar::LineFeed);
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
            if (isNotation) {
                note.append(_paragraphText[i]);
            } else {
                blockText.append(_paragraphText[i]);
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
            q->movePreviousTypes(i, 1);
            break;
        }

        case '/': {
            if (prevSymbol == '*' && isCommenting) {
                //
                // Заканчивается комментирование
                //
                isCommenting = false;
                noteStartPos += noteLen;
                noteLen = blockText.size();

                //
                // Закроем предыдущий блок, добавим текущий
                //
                if (!lastBlockText.isEmpty()) {
                    _writer.writeEndElement();
                }
                appendBlock(blockText.left(blockText.size()), TextParagraphType::InlineNote,
                            _writer);
                blockText.clear();
            } else {
                if (isNotation) {
                    note.append('/');
                } else {
                    blockText.append('/');
                }
            }
            break;
        }

        case '*': {
            if (prevSymbol == '/' && !isCommenting && !isNotation) {
                //
                // Начинается комментирование
                //
                isCommenting = true;
                noteStartPos += noteLen;
                noteLen = blockText.size() - 1;

                //
                // Закроем предыдущий блок и, если комментирование начинается в середние текущего
                // блока то добавим этот текущий блок
                //
                if (blockText.size() != 1) {
                    _writer.writeEndElement();
                    appendBlock(blockText.left(blockText.size() - 1), _type, _writer);
                    appendComments(_writer);
                    notes.clear();
                }
                blockText.clear();
            } else {
                if (isNotation) {
                    note.append('*');
                }
            }
            break;
        }

        case '[': {
            if (prevSymbol == '[' && !isCommenting && !isNotation) {
                //
                // Начинается редакторская заметка
                //
                isNotation = true;
                noteLen = blockText.size() - 1 - noteStartPos;
                blockText = blockText.left(blockText.size() - 1);
            } else {
                if (isNotation) {
                    note.append('[');
                } else {
                    blockText.append('[');
                }
            }
            break;
        }

        case ']': {
            if (prevSymbol == ']' && isNotation) {
                //
                // Закончилась редакторская заметка. Добавим ее в список редакторских заметок к
                // текущему блоку
                //
                isNotation = false;
                notes.append(std::make_tuple(note.left(note.size() - 1), noteStartPos, noteLen));
                noteStartPos += noteLen;
                noteLen = 0;
                note.clear();
            } else {
                if (isNotation) {
                    note.append(']');
                } else {
                    blockText.append(']');
                }
            }
            break;
        }

        default: {
            //
            // Самый обычный символ
            //
            if (isNotation) {
                note.append(_paragraphText[i]);
            } else {
                blockText.append(_paragraphText[i]);
            }
            break;
        }
        }

        prevSymbol = curSymbol;
    }

    if (!isNotation && !isCommenting) {
        //
        // Если блок действительно закончился
        //
        noteLen += blockText.size() - noteStartPos;

        //
        // Добавим текущий блок
        //
        if (!blockText.isEmpty() || _type == TextParagraphType::SequenceFooter) {
            //
            // ... но перед добавлением закроем предыдущий блок
            //
            if (!isFirstBlock) {
                _writer.writeEndElement();
            }

            appendBlock(blockText, _type, _writer);
        }
        blockText.clear();
    }
}

void ScreenplayFountainImporter::Implementation::appendBlock(const QString& _paragraphText,
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
        if (alreadyInBeat) {
            _writer.writeEndElement(); // контент предыдущего бита
            _writer.writeEndElement(); // предыдущий бит
        }

        alreadyInBeat = true; // вошли в новый бит

        _writer.writeStartElement(toString(TextGroupType::Beat));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        break;
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

void ScreenplayFountainImporter::Implementation::appendComments(QXmlStreamWriter& _writer)
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


ScreenplayFountainImporter::ScreenplayFountainImporter()
    : AbstractScreenplayImporter()
    , AbstractMarkdownImporter(kFountainSelectionTypes, kSelectionTypeChecker, kCapturedGroup)
    , d(new Implementation(this))
{
}

ScreenplayFountainImporter::~ScreenplayFountainImporter() = default;

AbstractScreenplayImporter::Documents ScreenplayFountainImporter::importDocuments(
    const ScreenplayImportOptions& _options) const
{
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

        switch (blockType) {
        case TextParagraphType::SceneHeading: {
            if (!_options.importLocations) {
                break;
            }

            const auto locationName = ScreenplaySceneHeadingParser::location(paragraphText);
            if (locationName.isEmpty()) {
                break;
            }

            locationNames.emplace(locationName);
            break;
        }

        case TextParagraphType::Character: {
            if (!_options.importCharacters) {
                break;
            }

            const auto characterName = ScreenplayCharacterParser::name(paragraphText);
            if (characterName.isEmpty()) {
                break;
            }

            characterNames.emplace(characterName);
            break;
        }

        default:
            break;
        }

        //
        // И очищаем текст
        //
        paragraphText.clear();
    }

    Documents documents;
    for (const auto& characterName : characterNames) {
        documents.characters.append(
            { Domain::DocumentObjectType::Character, characterName, {}, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ Domain::DocumentObjectType::Location, locationName, {}, {} });
    }
    return documents;
}

QVector<AbstractScreenplayImporter::Screenplay> ScreenplayFountainImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    if (_options.importText == false) {
        return {};
    }

    //
    // Открываем файл
    //
    QFile fountainFile(_options.filePath);
    if (!fountainFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Импортируем
    //
    auto screenplay = importScreenplay(fountainFile.readAll());
    if (screenplay.name.isEmpty()) {
        screenplay.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return { screenplay };
}

AbstractScreenplayImporter::Screenplay ScreenplayFountainImporter::importScreenplay(
    const QString& _screenplayText) const
{
    if (_screenplayText.simplified().isEmpty()) {
        return {};
    }

    Screenplay result;

    //
    // Читаем plain text
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&result.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Сформируем список строк, содержащий текст сценария
    //
    QVector<QString> paragraphs;
    bool isTitle = false;
    bool isFirstLine = true;
<<<<<<< HEAD
    for (const auto& str : QString(_screenplayText).remove('\r').split("\n")) {
=======
    for (QString str : QString(_screenplayText).remove('\r').split("\n")) {
>>>>>>> eb375d5b (Abstract markdown importer for novels and screenplays)
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
    QStack<QString> dirs;
    auto prevBlockType = TextParagraphType::Undefined;
    TextParagraphType blockType = TextParagraphType::Undefined;

    for (int i = 0; i != paragraphsCount; ++i) {
        QString paragraphText = paragraphs[i];

        if (d->isNotation || d->isCommenting) {
            //
            // Если мы комментируем или делаем заметку, то продолжим это
            //
            d->processBlock(paragraphText, prevBlockType, writer);
            continue;
        }

        if (paragraphText.isEmpty()) {
            continue;
        }

        blockType = TextParagraphType::Action;

        //
        // Собираем типы выделения текста и очищаем текст от форматных символов
        //
        collectSelectionTypes(paragraphText);

        switch (paragraphText[0].toLatin1()) {
        case '.': {
            blockType = TextParagraphType::SceneHeading;
            //
            // TODO: номера сцен игнорируем, поскольку в фонтане они являются строками
            //
            int sharpPos = paragraphText.size();
            if (paragraphText.endsWith("#")) {
                sharpPos = paragraphText.lastIndexOf('#', paragraphText.size() - 2);
            }
            if (sharpPos == -1) {
                sharpPos = paragraphText.size();
            }
            paragraphText.truncate(sharpPos);
            paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
            break;
        }

        case '!': {
            blockType = TextParagraphType::Action;
            paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
            break;
        }

        case '@': {
            blockType = TextParagraphType::Character;
            paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
            break;
        }

        case '>': {
            if (paragraphText.endsWith("<")) {
                blockType = TextParagraphType::Action;
                paragraphText.chop(1);
                paragraphText.remove(0, 1);
                movePreviousTypes(0, 1);
            } else {
                blockType = TextParagraphType::Transition;
                paragraphText.remove(0, 1);
                movePreviousTypes(0, 1);
            }
            break;
        }

        case '=': {
            bool isPageBreak = false;
            if (paragraphText.startsWith("===")) {
                isPageBreak = true;
                for (int j = 3; j != paragraphText.size(); ++j) {
                    if (paragraphText[j] != '=') {
                        isPageBreak = false;
                        break;
                    }
                }

                //
                // Если состоит из трех или более '=', то это PageBreak
                // TODO: У нас такого сейчас нет
                //
                continue;
            }
            if (!isPageBreak) {
                blockType = TextParagraphType::BeatHeading;
                paragraphText.remove(0, 1);
                movePreviousTypes(0, 1);
            }
            break;
        }

        case '~': {
            //
            // Лирика
            //
            blockType = TextParagraphType::Lyrics;
            paragraphText.remove(0, 1);
            movePreviousTypes(0, 1);
            break;
        }

        case '#': {
            //
            // Директории
            //
            int sharpCount = 0;
            while (paragraphText[sharpCount] == '#') {
                ++sharpCount;
            }

            if (sharpCount <= dirs.size()) {
                //
                // Закроем нужное число раз уже открытые
                //
                unsigned toClose = dirs.size() - sharpCount + 1;
                for (unsigned i = 0; i != toClose; ++i) {
                    d->processBlock({}, TextParagraphType::SequenceFooter, writer);
                    dirs.pop();
                }
            }
            //
            // И откроем новую
            //
            QString text = paragraphText.mid(sharpCount);
            d->processBlock(text, TextParagraphType::SequenceHeading, writer);
            dirs.push(text);
            prevBlockType = TextParagraphType::SequenceHeading;

            //
            // Поскольку директории добавляются прямо здесь без обработки, то в конец цикла идти не
            // надо
            //
            continue;
        }

        default: {
            bool startsWithHeading = false;
            for (const QString& sceneHeading : sceneHeadingsDictionary()) {
                if (paragraphText.startsWith(sceneHeading)) {
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
                int sharpPos = paragraphText.size();
                if (paragraphText.endsWith("#")) {
                    sharpPos = paragraphText.lastIndexOf('#', paragraphText.size() - 2);
                }
                if (sharpPos == -1) {
                    sharpPos = paragraphText.size();
                }
                paragraphText = paragraphText.left(sharpPos);
            } else if (paragraphText.startsWith("[[") && paragraphText.endsWith("]]")) {
                //
                // Редакторская заметка
                //
                paragraphText.chop(2);
                paragraphText.remove(0, 2);
                movePreviousTypes(0, 2);
                d->notes.append(std::make_tuple(paragraphText, d->noteStartPos, d->noteLen));
                d->noteStartPos += d->noteLen;
                d->noteLen = 0;
                continue;
            } else if (paragraphText.startsWith("/*")) {
                //
                // Начинается комментарий
                //
            } else if (paragraphText == TextHelper::smartToUpper(paragraphText) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && paragraphs[i + 1].isEmpty() && paragraphText.endsWith("TO:")) {
                //
                // Если состоит только из заглавных букв, предыдущая и следующая строки пустые
                // и заканчивается "TO:", то это переход
                //
                blockType = TextParagraphType::Transition;
<<<<<<< HEAD
=======
                paragraphText.chop(3);
>>>>>>> eb375d5b (Abstract markdown importer for novels and screenplays)
            } else if (paragraphText.startsWith("(") && paragraphText.endsWith(")")
                       && (prevBlockType == TextParagraphType::Character
                           || prevBlockType == TextParagraphType::Dialogue)) {
                //
                // Если текущий блок обернут в (), то это ремарка
                //
                blockType = TextParagraphType::Parenthetical;
                paragraphText.chop(1);
                paragraphText.remove(0, 1);
                movePreviousTypes(0, 1);
            } else if (paragraphText == TextHelper::smartToUpper(paragraphText) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && !paragraphs[i + 1].isEmpty()) {
                //
                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
                // пустая Значит это имя персонажа (для реплики)
                //
                blockType = TextParagraphType::Character;
                if (paragraphText.endsWith("^")) {
                    //
                    // Двойной диалог, который мы пока что не умеем обрабатывать
                    //
                    paragraphText.chop(1);
                }
            } else if (prevBlockType == TextParagraphType::Character
                       || prevBlockType == TextParagraphType::Parenthetical
                       || (prevBlockType == TextParagraphType::Dialogue && i > 0
                           && !paragraphs[i - 1].isEmpty())) {
                //
                // Если предыдущий блок - имя персонажа или ремарка, то сейчас диалог
                // Или предыдущая строка является диалогом
                //
                blockType = TextParagraphType::Dialogue;
            } else {
                //
                // Во всех остальных случаях - Action
                //
                blockType = TextParagraphType::Action;
            }
        }
        }
        //
        // Отправим блок на обработку
        //
        d->processBlock(paragraphText, blockType, writer);
        prevBlockType = blockType;
    }
    //
    // Добавим комментарии к последнему блоку
    //
    d->appendComments(writer);

    //
    // Закроем последний блок
    //
    writer.writeEndElement();

    //
    // Закроем директории нужное число раз
    //
    while (!dirs.empty()) {
        d->processBlock({}, TextParagraphType::SequenceFooter, writer);
        dirs.pop();
    }

    //
    // Закроем документ
    //
    writer.writeEndElement();
    writer.writeEndDocument();

    return result;
}

} // namespace BusinessLayer

#include "screenplay_fountain_importer.h"

#include "screenlay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QStack>
#include <QXmlStreamWriter>

#include <set>


namespace BusinessLayer {

namespace {

/**
 * @brief С чего может начинаться название сцены
 */
const QStringList kSceneHeadings = {
    QCoreApplication::translate("BusinessLayer::FountainImporter", "INT"),
    QCoreApplication::translate("BusinessLayer::FountainImporter", "EXT"),
    QCoreApplication::translate("BusinessLayer::FountainImporter", "EST"),
    QCoreApplication::translate("BusinessLayer::FountainImporter", "INT./EXT"),
    QCoreApplication::translate("BusinessLayer::FountainImporter", "INT/EXT"),
    QCoreApplication::translate("BusinessLayer::FountainImporter", "EXT./INT"),
    QCoreApplication::translate("BusinessLayer::FountainImporter", "EXT/INT"),
    QCoreApplication::translate("BusinessLayer::FountainImporter", "I/E"),
};

/**
 * @brief Ключит титульной страницы
 */
const QHash<QString, QString> kTitleKeys({ std::make_pair(QString("Title"), QString("name")),
                                           std::make_pair(QString("Author"), QString("author")),
                                           std::make_pair(QString("Authors"), QString("author")),
                                           std::make_pair(QString("Draft date"), QString("year")),
                                           std::make_pair(QString("Copyright"), QString("year")),
                                           std::make_pair(QString("Contact"), QString("contacts")),
                                           std::make_pair(QString("Credit"), QString("genre")),
                                           std::make_pair(QString("Source"),
                                                          QString("additional_info")) });

const QString kTripleWhitespace = QLatin1String("   ");
const QString kDoubleWhitespace = QLatin1String("  ");

} // namespace

class ScreenplayFountainImporter::Implementation
{
public:
    /**
     * @brief Обработка конкретного блока перед его добавлением
     */
    void processBlock(const QString& _paragraphText, ScreenplayParagraphType _type,
                      QXmlStreamWriter& _writer);

    /**
     * @brief Добавление блока
     */
    void appendBlock(const QString& _paragraphText, ScreenplayParagraphType _type,
                     QXmlStreamWriter& _writer);

    /**
     * @brief Добавление комментариев к блоку
     */
    void appendComments(QXmlStreamWriter& _writer);

    /**
     * @brief Убрать форматирование
     */
    QString simplify(const QString& _value);

    /**
     * @brief Добавить форматирование
     * @param _atCurrentCharacter - начинается/заканчивается ли форматирование
     *        в текущей позиции (true), или захватывает последний символ (false)
     */
    bool processFormat(bool _italics, bool _bold, bool _underline, bool _forCurrentCharacter,
                       bool _isCanStartEmphasis, bool _isCanEndEmphasis);

    /**
     * @brief Может ли предыдущий символ быть началом форматирования
     */
    bool canStartEmphasis() const;

    /**
     * @brief Может ли предыдущий символ быть концом форматирования
     */
    bool canEndEmphasis(const QString& _paragraphText, int _pos) const;


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
     * @brief Зашли ли мы уже в первую сцену
     */
    bool alreadyInScene = false;

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
     * @brief Список форматов обрабатываемых блоков
     */
    QVector<ScreenplayTextModelTextItem::TextFormat> formats;

    /**
     * @brief Последний обрабатываемый формат
     */
    ScreenplayTextModelTextItem::TextFormat lastFormat;
};

void ScreenplayFountainImporter::Implementation::processBlock(const QString& _paragraphText,
                                                              ScreenplayParagraphType _type,
                                                              QXmlStreamWriter& _writer)
{
    if (!isNotation && !isCommenting) {
        //
        // Начинается новая сущность
        //
        blockText.reserve(_paragraphText.size());

        //
        // Добавим комментарии к предыдущему блоку
        //
        appendComments(_writer);

        noteLen = 0;
        noteStartPos = 0;
    }

    if (!isCommenting) {
        formats.clear();
    }

    char prevSymbol = '\0';
    int asteriskLen = 0;
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
            // Раз мы добавляем символ через '/',
            // то он не должен участвовать в какой-либо обработке
            //
            prevSymbol = '\0';
            continue;
        }

        char curSymbol = _paragraphText[i].toLatin1();
        switch (curSymbol) {
        case '\\': {
            break;
        }

        case '/': {
            if (prevSymbol == '*' && isCommenting) {
                //
                // Заканчивается комментирование
                //
                --asteriskLen;
                isCommenting = false;
                noteStartPos += noteLen;
                noteLen = blockText.size();

                //
                // Закроем предыдущий блок, добавим текущий
                //
                _writer.writeEndElement();
                appendBlock(blockText.left(blockText.size()), ScreenplayParagraphType::InlineNote,
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
                _writer.writeEndElement();
                if (blockText.size() != 1) {
                    appendBlock(blockText.left(blockText.size() - 1), _type, _writer);
                    appendComments(_writer);
                    notes.clear();
                }
                blockText.clear();
            } else {
                if (isNotation) {
                    note.append('*');
                } else {
                    ++asteriskLen;
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

        case '_': {
            //
            // Подчеркивания обрабатываются в другом месте, поэтому тут игнорируем его обработку
            //
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

        const bool isCanStartEmphasis = canStartEmphasis();
        const bool isCanEndEmphasis = canEndEmphasis(_paragraphText, i);
        //
        // Underline
        //
        if (prevSymbol == '_') {
            if (!processFormat(false, false, true, curSymbol == '*', isCanStartEmphasis,
                               isCanEndEmphasis)) {
                blockText.insert(std::max(static_cast<qsizetype>(0), blockText.size() - 1),
                                 prevSymbol);
            }
        }

        if (curSymbol != '*') {
            bool success = false;
            switch (asteriskLen) {
            //
            // Italics
            //
            case 1: {
                success = processFormat(true, false, false, curSymbol == '_', isCanStartEmphasis,
                                        isCanEndEmphasis);
                break;
            }

            //
            // Bold
            //
            case 2: {
                success = processFormat(false, true, false, curSymbol == '_', isCanStartEmphasis,
                                        isCanEndEmphasis);
                break;
            }

            //
            // Bold & Italics
            //
            case 3: {
                success = processFormat(true, true, false, curSymbol == '_', isCanStartEmphasis,
                                        isCanEndEmphasis);
                break;
            }

            default:
                break;
            }
            if (!success) {
                for (int i = 0; i != asteriskLen; ++i) {
                    blockText.insert(std::max(static_cast<qsizetype>(0), blockText.size() - 1),
                                     '*');
                }
            }
            asteriskLen = 0;
        }

        prevSymbol = curSymbol;
    }

    //
    // Underline
    //
    if (prevSymbol == '_') {
        if (!processFormat(false, false, true, true, false, true)) {
            blockText.append(prevSymbol);
        }
    }

    bool success = false;
    switch (asteriskLen) {
    //
    // Italics
    //
    case 1: {
        success = processFormat(true, false, false, true, false, true);
        break;
    }

    //
    // Bold
    //
    case 2: {
        success = processFormat(false, true, false, true, false, true);
        break;
    }

    //
    // Bold & Italics
    //
    case 3: {
        success = processFormat(true, true, false, true, false, true);
        break;
    }

    default:
        break;
    }

    if (!success) {
        for (int i = 0; i != asteriskLen; ++i) {
            blockText.append('*');
        }
    }
    asteriskLen = 0;


    if (!isNotation && !isCommenting) {
        //
        // Если блок действительно закончился
        //
        noteLen += blockText.size() - noteStartPos;

        //
        // Закроем предыдущий блок
        //
        if (!isFirstBlock) {
            _writer.writeEndElement();
        }

        //
        // Добавим текущий блок
        //
        if (!blockText.isEmpty() || _type == ScreenplayParagraphType::FolderFooter) {
            appendBlock(blockText, _type, _writer);
        }
        blockText.clear();
    }

    //
    // Первый блок в тексте может встретиться лишь однажды
    //
    if (isFirstBlock) {
        isFirstBlock = false;
    }
}

void ScreenplayFountainImporter::Implementation::appendBlock(const QString& _paragraphText,
                                                             ScreenplayParagraphType _type,
                                                             QXmlStreamWriter& _writer)
{
    int leadSpaceCount = 0;
    QString paragraphText = _paragraphText;
    while (!paragraphText.isEmpty() && paragraphText.startsWith(" ")) {
        ++leadSpaceCount;
        paragraphText = paragraphText.mid(1);
    }

    //
    // У нас осталось незакрытое форматирование, а значит его нужно не закрыть, а убрать
    //
    if (lastFormat.isValid()) {
        QVector<ScreenplayTextModelTextItem::TextFormat> removedFormats;
        if (!formats.empty()) {
            for (int i = formats.size() - 1; i >= 0; --i) {
                ScreenplayTextModelTextItem::TextFormat& format = formats[i];
                ScreenplayTextModelTextItem::TextFormat removed;

                //
                // У нас остался незакрытый жирный формат
                //
                if (lastFormat.isBold) {
                    if (!format.isBold) {
                        //
                        // Формат, начиная отсюда не является жирным. Значит, предыдущий был
                        // открывающим Значит, на место предыдущего надо вернуть звездочки, а жирный
                        // незакрытый мы больше не ищем
                        //
                        lastFormat.isBold = false;
                        removed.isBold = true;
                    } else {
                        //
                        // Формат здесь все еще является жирным, значит просто перестаем его таковым
                        // считать
                        //
                        format.isBold = false;
                    }
                }

                //
                // Аналогично для остальных форматов
                //
                if (lastFormat.isItalic) {
                    if (!format.isItalic) {
                        lastFormat.isItalic = false;
                        removed.isItalic = true;
                    } else {
                        format.isItalic = false;
                    }
                }

                if (lastFormat.isUnderline) {
                    if (!format.isUnderline) {
                        lastFormat.isUnderline = false;
                        removed.isUnderline = true;
                    } else {
                        format.isUnderline = false;
                    }
                }

                //
                // У нас есть формат, который мы удалили (нам важна его позиция, чтобы вернуть
                // символы)
                //
                if (removed.isValid()) {
                    removed.from = lastFormat.from;
                    removedFormats.push_back(removed);
                }
                lastFormat.from = format.from;

                //
                // Может быть текущий формат стал бесполезным
                //
                if (!format.isValid()) {
                    formats.removeAt(i);
                }

                //
                // Все закрыли, мы молодцы
                //
                if (!lastFormat.isValid()) {
                    break;
                }
            }
        }

        //
        // Что то еще осталось (это нормально), поэтому просто тоже вернем эти символы форматировани
        //
        if (lastFormat.isValid()) {
            removedFormats.push_back(lastFormat);
            lastFormat = {};
        }

        //
        // Возвращаем символы форматирования
        //
        for (const auto& format : removedFormats) {
            QString addedStr;
            if (format.isBold) {
                addedStr += "**";
            }
            if (format.isItalic) {
                addedStr += "*";
            }
            if (format.isUnderline) {
                addedStr += "_";
            }

            //
            // Сдвигаем/увеличиваем форматы на длину добавленных символов
            //
            for (auto& innerFormat : formats) {
                if (innerFormat.from < format.from
                    && innerFormat.from + innerFormat.length >= format.from) {
                    innerFormat.length += addedStr.size();
                } else if (innerFormat.from >= format.from) {
                    innerFormat.from += addedStr.size();
                }
            }
            paragraphText.insert(format.from, addedStr);
        }
    }

    //
    // Формируем блок сценария
    //
    switch (_type) {
    case ScreenplayParagraphType::FolderHeader: {
        if (alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }

        alreadyInScene = false; // вышли из сцены

        _writer.writeStartElement(xml::kFolderTag);
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        break;
    }

    case ScreenplayParagraphType::FolderFooter: {
        if (alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }

        alreadyInScene = false; // вышли из сцены

        _writer.writeEndElement(); // контент текущей папки
        _writer.writeEndElement(); // текущая папка
        break;
    }

    case ScreenplayParagraphType::SceneHeading: {
        if (alreadyInScene) {
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }

        alreadyInScene = true; // вошли в новую сцену

        _writer.writeStartElement(xml::kSceneTag);
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
    if (!formats.isEmpty()) {
        _writer.writeStartElement(xml::kFormatsTag);
        for (const auto& format : std::as_const(formats)) {
            _writer.writeStartElement(xml::kFormatTag);
            //
            // Данные пользовательского форматирования
            //
            _writer.writeAttribute(xml::kFromAttribute,
                                   QString::number(format.from - leadSpaceCount));
            _writer.writeAttribute(xml::kLengthAttribute, QString::number(format.length));
            if (format.isBold) {
                _writer.writeAttribute(xml::kBoldAttribute, "true");
            }
            if (format.isItalic) {
                _writer.writeAttribute(xml::kItalicAttribute, "true");
            }
            if (format.isUnderline) {
                _writer.writeAttribute(xml::kUnderlineAttribute, "true");
            }
            //
            _writer.writeEndElement(); // format
        }
        _writer.writeEndElement(); // formats
        formats.clear();
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
        if (std::get<2>(notes[i]) != 0) {
            if (i != 0) {
                _writer.writeEndElement(); // review mark
            }
            _writer.writeStartElement(xml::kReviewMarkTag);
            _writer.writeAttribute(xml::kFromAttribute, QString::number(std::get<1>(notes[i])));
            _writer.writeAttribute(xml::kLengthAttribute, QString::number(std::get<2>(notes[i])));
            _writer.writeAttribute(xml::kBackgroundColorAttribute, "#FFD302");
        }
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

QString ScreenplayFountainImporter::Implementation::simplify(const QString& _value)
{
    QString res;
    for (int i = 0; i != _value.size(); ++i) {
        if (_value[i] == '*' || _value[i] == '_' || _value[i] == '\\') {
            if (i == 0 || (i > 0 && _value[i - 1] != '\\')) {
                continue;
            } else {
                res += _value[i];
            }
        } else {
            res += _value[i];
        }
    }
    return res;
}

bool ScreenplayFountainImporter::Implementation::processFormat(bool _italics, bool _bold,
                                                               bool _underline,
                                                               bool _forCurrentCharacter,
                                                               bool _isCanStartEmphasis,
                                                               bool _isCanEndEmphasis)
{
    //
    // Новый формат, который еще не начат
    //
    if (!lastFormat.isValid()) {
        if (!_isCanStartEmphasis) {
            return false;
        }

        lastFormat.isBold = _bold;
        lastFormat.isItalic = _italics;
        lastFormat.isUnderline = _underline;
        lastFormat.from = blockText.size();
        if (!_forCurrentCharacter) {
            --lastFormat.from;
        }
        return true;
    }
    //
    // Формат уже начат
    //
    else {
        if ((lastFormat.isBold & _bold) == _bold && (lastFormat.isItalic & _italics) == _italics
            && (lastFormat.isUnderline & _underline) == _underline) {
            //
            // Если тут появилось что то новенькое, то может ли это быть началом
            //
            if (!_isCanEndEmphasis) {
                return false;
            }
        } else {
            //
            // Иначе, может ли быть концом
            //
            if (!_isCanStartEmphasis) {
                return false;
            }
        }
        //
        // Добавим его в список форматов
        //
        lastFormat.length = blockText.size() - lastFormat.from;
        if (!_forCurrentCharacter) {
            --lastFormat.length;
        }
        if (lastFormat.length != 0) {
            formats.push_back(lastFormat);
        }

        //
        // Если необходимо, созданим новый, частично унаследованный от текущего
        //
        if (lastFormat.isBold != _bold || lastFormat.isItalic != _italics
            || lastFormat.isUnderline != _underline) {
            lastFormat.isItalic = lastFormat.isItalic ^ _italics;
            lastFormat.isBold = lastFormat.isBold ^ _bold;
            lastFormat.isUnderline = lastFormat.isUnderline ^ _underline;
            lastFormat.from = lastFormat.from + lastFormat.length;
        }
        //
        // Либо просто закроем
        //
        else {
            lastFormat = {};
        }
        return true;
    }
}

bool ScreenplayFountainImporter::Implementation::canStartEmphasis() const
{
    return blockText.size() <= 1 || !blockText[blockText.size() - 2].isLetterOrNumber();
}

bool ScreenplayFountainImporter::Implementation::canEndEmphasis(const QString& _paragraphText,
                                                                int _pos) const
{
    return _pos >= _paragraphText.size() || !_paragraphText[_pos].isLetterOrNumber();
}


// ****


ScreenplayFountainImporter::ScreenplayFountainImporter()
    : d(new Implementation)
{
}

ScreenplayFountainImporter::~ScreenplayFountainImporter() = default;

ScreenplayAbstractImporter::Documents ScreenplayFountainImporter::importDocuments(
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
    const QString& scriptText = fountainFile.readAll();

    //
    // Сформируем список строк, содержащий текст сценария
    //
    QVector<QString> paragraphs;
    bool isTitle = false;
    bool isFirstLine = true;
    for (QString str : scriptText.split("\n")) {
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
            if (str.endsWith("\r")) {
                str.chop(1);
            }

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
    auto prevBlockType = ScreenplayParagraphType::Undefined;
    ScreenplayParagraphType blockType;
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    for (int i = 0; i != paragraphsCount; ++i) {
        if (paragraphs[i].isEmpty()) {
            continue;
        }

        blockType = ScreenplayParagraphType::Undefined;
        QString paragraphText;

        switch (paragraphs[i][0].toLatin1()) {
        case '.': {
            blockType = ScreenplayParagraphType::SceneHeading;
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
            blockType = ScreenplayParagraphType::Character;
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
            for (const QString& sceneHeading : kSceneHeadings) {
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
                blockType = ScreenplayParagraphType::SceneHeading;

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
                       && (prevBlockType == ScreenplayParagraphType::Character
                           || prevBlockType == ScreenplayParagraphType::Dialogue)) {
                continue;
            } else if (paragraphs[i] == TextHelper::smartToUpper(paragraphs[i]) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && !paragraphs[i + 1].isEmpty()) {
                //
                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
                // пустая Значит это имя персонажа (для реплики)
                //
                blockType = ScreenplayParagraphType::Character;
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
        case ScreenplayParagraphType::SceneHeading: {
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

        case ScreenplayParagraphType::Character: {
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
        documents.characters.append({ characterName, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ locationName, {} });
    }
    return documents;
}

QVector<ScreenplayAbstractImporter::Screenplay> ScreenplayFountainImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    if (_options.importScreenplay == false) {
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

ScreenplayAbstractImporter::Screenplay ScreenplayFountainImporter::importScreenplay(
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
    for (QString str : _screenplayText.split("\n")) {
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
            if (str.endsWith("\r")) {
                str.chop(1);
            }

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

    //
    // Очищаем форматы перед импортом
    //
    d->formats.clear();
    d->lastFormat = {};

    const int paragraphsCount = paragraphs.size();
    auto prevBlockType = ScreenplayParagraphType::Undefined;
    QStack<QString> dirs;
    ScreenplayParagraphType blockType;
    for (int i = 0; i != paragraphsCount; ++i) {
        if (d->isNotation || d->isCommenting) {
            //
            // Если мы комментируем или делаем заметку, то продолжим это
            //
            d->processBlock(paragraphs[i], prevBlockType, writer);
            continue;
        }

        if (paragraphs[i].isEmpty()) {
            continue;
        }

        blockType = ScreenplayParagraphType::Action;
        QString paragraphText;

        switch (paragraphs[i][0].toLatin1()) {
        case '.': {
            blockType = ScreenplayParagraphType::SceneHeading;
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

        case '!': {
            blockType = ScreenplayParagraphType::Action;
            paragraphText = paragraphs[i].mid(1);
            break;
        }

        case '@': {
            blockType = ScreenplayParagraphType::Character;
            paragraphText = paragraphs[i].mid(1);
            break;
        }

        case '>': {
            if (paragraphs[i].endsWith("<")) {
                blockType = ScreenplayParagraphType::Action;
                paragraphText = paragraphs[i].mid(1, paragraphs[i].size() - 2);
            } else {
                blockType = ScreenplayParagraphType::Transition;
                paragraphText = paragraphs[i].mid(1);
            }
            break;
        }

        case '=': {
            bool isPageBreak = false;
            if (paragraphs[i].startsWith("===")) {
                isPageBreak = true;
                for (int j = 3; j != paragraphs[i].size(); ++j) {
                    if (paragraphs[i][j] != '=') {
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
                //
                // TODO: тритмент подгружается тут
                //
                continue;
                //                    blockType = ScreenplayParagraphType::SceneDescription;
                //                    paragraphText = paragraphs[i].mid(1);
            }
            break;
        }

        case '~': {
            //
            // Лирика
            //
            blockType = ScreenplayParagraphType::Lyrics;
            paragraphText = paragraphs[i].mid(1);
            break;
        }

        case '#': {
            //
            // Директории
            //
            int sharpCount = 0;
            while (paragraphs[i][sharpCount] == '#') {
                ++sharpCount;
            }

            if (sharpCount <= dirs.size()) {
                //
                // Закроем нужное число раз уже открытые
                //
                unsigned toClose = dirs.size() - sharpCount + 1;
                for (unsigned i = 0; i != toClose; ++i) {
                    d->processBlock({}, ScreenplayParagraphType::FolderFooter, writer);
                    dirs.pop();
                }
                prevBlockType = ScreenplayParagraphType::FolderFooter;
            }
            //
            // И откроем новую
            //
            QString text = paragraphs[i].mid(sharpCount);
            d->processBlock(text, ScreenplayParagraphType::FolderHeader, writer);
            dirs.push(text);
            prevBlockType = ScreenplayParagraphType::FolderHeader;

            //
            // Поскольку директории добавляются прямо здесь без обработки, то в конец цикла идти не
            // надо
            //
            continue;
            break;
        }

        default: {
            bool startsWithHeading = false;
            for (const QString& sceneHeading : kSceneHeadings) {
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
                blockType = ScreenplayParagraphType::SceneHeading;

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
                //
                // Редакторская заметка
                //
                d->notes.append(std::make_tuple(paragraphs[i].mid(2, paragraphs[i].size() - 4),
                                                d->noteStartPos, d->noteLen));
                d->noteStartPos += d->noteLen;
                d->noteLen = 0;
                continue;
            } else if (paragraphs[i].startsWith("/*")) {
                //
                // Начинается комментарий
                paragraphText = paragraphs[i];
            } else if (paragraphs[i] == TextHelper::smartToUpper(paragraphs[i]) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && paragraphs[i + 1].isEmpty() && paragraphs[i].endsWith("TO:")) {
                //
                // Если состоит только из заглавных букв, предыдущая и следующая строки пустые
                // и заканчивается "TO:", то это переход
                //
                blockType = ScreenplayParagraphType::Transition;
                paragraphText = paragraphs[i].left(paragraphs[i].size() - 4);
            } else if (paragraphs[i].startsWith("(") && paragraphs[i].endsWith(")")
                       && (prevBlockType == ScreenplayParagraphType::Character
                           || prevBlockType == ScreenplayParagraphType::Dialogue)) {
                //
                // Если текущий блок обернут в (), то это ремарка
                //
                blockType = ScreenplayParagraphType::Parenthetical;
                paragraphText = paragraphs[i].mid(1, paragraphs[i].length() - 2);
            } else if (paragraphs[i] == TextHelper::smartToUpper(paragraphs[i]) && i != 0
                       && paragraphs[i - 1].isEmpty() && i + 1 < paragraphsCount
                       && !paragraphs[i + 1].isEmpty()) {
                //
                // Если состоит из только из заглавных букв, впереди не пустая строка, а перед
                // пустая Значит это имя персонажа (для реплики)
                //
                blockType = ScreenplayParagraphType::Character;
                if (paragraphs[i].endsWith("^")) {
                    //
                    // Двойной диалог, который мы пока что не умеем обрабатывать
                    //
                    paragraphText = paragraphs[i].left(paragraphs[i].size() - 1);
                } else {
                    paragraphText = paragraphs[i];
                }
            } else if (prevBlockType == ScreenplayParagraphType::Character
                       || prevBlockType == ScreenplayParagraphType::Parenthetical
                       || (prevBlockType == ScreenplayParagraphType::Dialogue && i > 0
                           && !paragraphs[i - 1].isEmpty())) {
                //
                // Если предыдущий блок - имя персонажа или ремарка, то сейчас диалог
                // Или предыдущая строка является диалогом
                //
                blockType = ScreenplayParagraphType::Dialogue;
                paragraphText = paragraphs[i];
            } else {
                //
                // Во всех остальных случаях - Action
                //
                blockType = ScreenplayParagraphType::Action;
                paragraphText = paragraphs[i];
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
        d->processBlock({}, ScreenplayParagraphType::FolderFooter, writer);
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

#pragma once

#include "abstract_markdown_importer.h"

#include <QVector>

#include <set>

namespace BusinessLayer {

enum class TextParagraphType;
struct ImportOptions;

/**
 * @brief Абстрактный импортер из формата fountain
 */
class AbstractFountainImporter : public AbstractMarkdownImporter
{
public:
    AbstractFountainImporter();
    ~AbstractFountainImporter() override;

protected:
    /**
     * @brief Получить основной текст документа в формате xml из заданного текста
     */
    QString documentText(const QString& _text) const;

    /**
     * @brief Получить список параграфов и их типов (всех кроме основного текста)
     * @note Для парсинга локаций и персонажей
     */
    QVector<QPair<TextParagraphType, QString>> parapraphsForDocuments(
        const ImportOptions& _options) const;

    /**
     * @brief Идет ли сейчас редакторская заметка
     */
    bool isNotation() const;

    /**
     * @brief Идет ли сейчас комментарий
     */
    bool isCommenting() const;

    /**
     * @brief С чего может начинаться название сцены
     */
    QStringList sceneHeadingsDictionary() const;

    /**
     * @brief Добавить редакторскую заметку к текущему блоку
     */
    void appendNotes(const QString& _paragraphText) const;

    /**
     * @brief Обработать комментарии и редакторские заметки
     */
    void processNotes(const QString& _paragraphText, TextParagraphType _type,
                      QXmlStreamWriter& _writer) const;

    /**
     * @brief Записать текст текущего блока в переменную, храняющую текст последнего обработанного
     * блока
     */
    void setCurrentBlockTextLast() const;

    /**
     * @brief Добавить блок
     */
    virtual void appendBlock(const QString& _paragraphText, TextParagraphType _type,
                             QXmlStreamWriter& _writer, bool _shouldClosePrevBlock) const = 0;

    /**
     * @brief Обработка блоков текста
     * @return Стек незакрытых дирректорий
     */
    virtual QStack<QString> processBlocks(QVector<QString>& paragraphs,
                                          QXmlStreamWriter& _writer) const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

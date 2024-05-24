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
    AbstractFountainImporter(bool _includeBeats);
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
     * @brief Обработка конкретного блока перед его добавлением
     * @note Обрабатываются комментарии и редакторские заметки после чего блок отправляется на
     * добавление
     */
    void processBlock(const QString& _paragraphText, TextParagraphType _type,
                      QXmlStreamWriter& _writer) const;

    /**
     * @brief Предобработка конкретного блока (определяется его тип)
     */
    virtual void preprocessBlock(QVector<QString>& paragraphs, QXmlStreamWriter& _writer) const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

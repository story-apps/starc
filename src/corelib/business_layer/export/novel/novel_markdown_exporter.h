#pragma once

#include "novel_exporter.h"

#include <business_layer/export/abstract_markdown_exporter.h>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT NovelMarkdownExporter : public NovelExporter,
                                                  public AbstractMarkdownExporter
{
public:
    NovelMarkdownExporter();
    ~NovelMarkdownExporter() override;

protected:
    /**
     * @brief Обработать блок в зависимости от его типа
     * @return Был ли блок обработан
     */
    bool processBlock(QString& _paragraph, const QTextBlock& _block,
                      const ExportOptions& _exportOptions) const override;

    /**
     * @brief Получить символы типа выделения текста
     */
    QString formatSymbols(TextSelectionTypes _type) const override;

    /**
     * @brief Добавить пустые строки перед абзацем
     */
    void addIndentationAtBegin(QString& _paragraph, TextParagraphType _previosBlockType,
                               TextParagraphType _currentBlockType) const override;
};

} // namespace BusinessLayer

#pragma once

#include "abstract_exporter.h"

#include <QTextLayout>

namespace BusinessLayer {

enum class TextParagraphType;

class CORE_LIBRARY_EXPORT AbstractMarkdownExporter : virtual public AbstractExporter
{
public:
    AbstractMarkdownExporter(const QSet<QChar> _escapedCharacters);
    ~AbstractMarkdownExporter() override;

    /**
     * @brief Экспортировать текстовый документ
     */
    void exportTo(AbstractModel* _model, ExportOptions& _exportOptions) const override;

    /**
     * @brief Экспортировать текстовый документ в заданном интервале текста
     */
    void exportTo(AbstractModel* _model, int _fromPosition, int _toPosition,
                  ExportOptions& _exportOptions) const;

protected:
    /**
     * @brief Типы выделения текста, которые поддерживаются markdown и fountain
     */
    enum TextSelectionTypes {
        Bold,
        Italic,
        StrikeOut,
        Underline,
    };

    /**
     * @brief Добавить пустые строки перед абзацем
     */
    virtual void addIndentationAtBegin(QString& _paragraph, TextParagraphType _previosBlockType,
                                       TextParagraphType _currentBlockType) const = 0;

    /**
     * @brief Обработать блок в зависимости от его типа
     * @return Был ли блок обработан
     */
    virtual bool processBlock(QString& _paragraph, const QTextBlock& _block,
                              const ExportOptions& _exportOptions) const = 0;

    /**
     * @brief Получить символы типа выделения текста
     */
    virtual QString formatSymbols(TextSelectionTypes _type) const = 0;

    /**
     * @brief Очистить начало параграфа от пробельных символов
     */
    void removeWhitespaceAtBegin(QString& _paragraph) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

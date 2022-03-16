#pragma once

#include <business_layer/document/text/text_document.h>


namespace BusinessLayer {

/**
 * @brief Класс текстового документа
 */
class CORE_LIBRARY_EXPORT SimpleTextDocument : public TextDocument
{
    Q_OBJECT

public:
    explicit SimpleTextDocument(QObject* _parent = nullptr);

    /**
     * @brief Получить номер главы для заданного блока
     */
    QString chapterNumber(const QTextBlock& _forBlock) const;
};

} // namespace BusinessLayer

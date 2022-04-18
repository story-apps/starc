#pragma once

#include <business_layer/document/text/text_document.h>
#include <business_layer/model/text/text_model_text_item.h>


namespace BusinessLayer {

/**
 * @brief Класс документа сценария
 */
class CORE_LIBRARY_EXPORT ComicBookTextDocument : public TextDocument
{
    Q_OBJECT

public:
    explicit ComicBookTextDocument(QObject* _parent = nullptr);

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectCharactersNames, bool _needToCorrectPageBreaks);

    /**
     * @brief Получить номер страницы для заданного блока
     */
    QString pageNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить номер панели для заданного блока
     */
    QString panelNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить номер реплики для заданного блока
     */
    QString dialogueNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить закладку блока
     */
    TextModelTextItem::Bookmark bookmark(const QTextBlock& _forBlock) const;
};

} // namespace BusinessLayer

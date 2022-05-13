#pragma once

#include <business_layer/document/text/text_document.h>
#include <business_layer/model/text/text_model_text_item.h>


namespace BusinessLayer {

/**
 * @brief Класс документа пьесы
 */
class CORE_LIBRARY_EXPORT StageplayTextDocument : public TextDocument
{
    Q_OBJECT

public:
    explicit StageplayTextDocument(QObject* _parent = nullptr);
    ~StageplayTextDocument() override;

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectPageBreaks);

    /**
     * @brief Получить номер заданного блока
     */
    QString blockNumber(const QTextBlock& _forBlock) const;
};

} // namespace BusinessLayer

#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

/**
 * @brief Модель синопсиса аудиопостановки
 */
class CORE_LIBRARY_EXPORT StageplaySynopsisModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit StageplaySynopsisModel(QObject* _parent = nullptr);

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;
};

} // namespace BusinessLayer

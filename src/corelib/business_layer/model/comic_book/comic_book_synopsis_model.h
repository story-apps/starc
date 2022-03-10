#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

/**
 * @brief Модель синопсиса комикса
 */
class CORE_LIBRARY_EXPORT ComicBookSynopsisModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit ComicBookSynopsisModel(QObject* _parent = nullptr);

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;
};

} // namespace BusinessLayer

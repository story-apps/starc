#pragma once

#include "../text/text_model.h"


namespace BusinessLayer {

/**
 * @brief Модель
 */
class CORE_LIBRARY_EXPORT ScreenplaySynopsisModel : public TextModel
{
    Q_OBJECT

public:
    explicit ScreenplaySynopsisModel(QObject* _parent = nullptr);

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;
};

} // namespace BusinessLayer

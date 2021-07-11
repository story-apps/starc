#pragma once

#include "../text/text_model.h"


namespace BusinessLayer {

/**
 * @brief Модель
 */
class CORE_LIBRARY_EXPORT ScreenplayTitlePageModel : public TextModel
{
    Q_OBJECT

public:
    explicit ScreenplayTitlePageModel(QObject* _parent = nullptr);

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

protected:
    /**
     * @brief Реализуем собственную инициализацию для пустого документа
     */
    void initDocument() override;
};

} // namespace BusinessLayer

#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

/**
 * @brief Модель титульной страницы комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTitlePageModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit ComicBookTitlePageModel(QObject* _parent = nullptr);

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

#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class ComicBookInformationModel;

/**
 * @brief Модель титульной страницы комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTitlePageModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit ComicBookTitlePageModel(QObject* _parent = nullptr);
    ~ComicBookTitlePageModel() override;

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(ComicBookInformationModel* _model);
    ComicBookInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

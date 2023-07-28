#pragma once

#include "../base//title_page_model.h"


namespace BusinessLayer {

class ComicBookInformationModel;

/**
 * @brief Модель титульной страницы комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTitlePageModel : public TitlePageModel
{
    Q_OBJECT

public:
    explicit ComicBookTitlePageModel(QObject* _parent = nullptr);
    ~ComicBookTitlePageModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

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

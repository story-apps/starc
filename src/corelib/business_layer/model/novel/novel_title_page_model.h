#pragma once

#include "../base//title_page_model.h"


namespace BusinessLayer {

class NovelInformationModel;

/**
 * @brief Модель титульной страницы
 */
class CORE_LIBRARY_EXPORT NovelTitlePageModel : public TitlePageModel
{
    Q_OBJECT

public:
    explicit NovelTitlePageModel(QObject* _parent = nullptr);
    ~NovelTitlePageModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Задать модель информации
     */
    void setInformationModel(NovelInformationModel* _model);
    NovelInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

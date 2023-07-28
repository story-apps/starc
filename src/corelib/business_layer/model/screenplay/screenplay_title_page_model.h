#pragma once

#include "../base//title_page_model.h"


namespace BusinessLayer {

class ScreenplayInformationModel;

/**
 * @brief Модель
 */
class CORE_LIBRARY_EXPORT ScreenplayTitlePageModel : public TitlePageModel
{
    Q_OBJECT

public:
    explicit ScreenplayTitlePageModel(QObject* _parent = nullptr);
    ~ScreenplayTitlePageModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(ScreenplayInformationModel* _model);
    ScreenplayInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

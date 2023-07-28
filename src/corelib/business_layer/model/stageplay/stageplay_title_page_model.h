#pragma once

#include "../base//title_page_model.h"


namespace BusinessLayer {

class StageplayInformationModel;

/**
 * @brief Модель титульной страницы аудиопостановки
 */
class CORE_LIBRARY_EXPORT StageplayTitlePageModel : public TitlePageModel
{
    Q_OBJECT

public:
    explicit StageplayTitlePageModel(QObject* _parent = nullptr);
    ~StageplayTitlePageModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(StageplayInformationModel* _model);
    StageplayInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

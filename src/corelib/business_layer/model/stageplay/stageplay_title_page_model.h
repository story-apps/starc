#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class StageplayInformationModel;

/**
 * @brief Модель титульной страницы аудиопостановки
 */
class CORE_LIBRARY_EXPORT StageplayTitlePageModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit StageplayTitlePageModel(QObject* _parent = nullptr);
    ~StageplayTitlePageModel() override;

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

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

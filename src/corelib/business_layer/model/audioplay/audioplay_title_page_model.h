#pragma once

#include "../base//title_page_model.h"


namespace BusinessLayer {

class AudioplayInformationModel;
class CharacterModel;

/**
 * @brief Модель титульной страницы аудиопостановки
 */
class CORE_LIBRARY_EXPORT AudioplayTitlePageModel : public TitlePageModel
{
    Q_OBJECT

public:
    explicit AudioplayTitlePageModel(QObject* _parent = nullptr);
    ~AudioplayTitlePageModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(AudioplayInformationModel* _model);
    AudioplayInformationModel* informationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class AudioplayInformationModel;

/**
 * @brief Модель титульной страницы аудиопостановки
 */
class CORE_LIBRARY_EXPORT AudioplayTitlePageModel : public SimpleTextModel
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
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

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

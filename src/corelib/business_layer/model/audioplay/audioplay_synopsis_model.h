#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class AudioplayInformationModel;

/**
 * @brief Модель синопсиса аудиопостановки
 */
class CORE_LIBRARY_EXPORT AudioplaySynopsisModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit AudioplaySynopsisModel(QObject* _parent = nullptr);
    ~AudioplaySynopsisModel() override;

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

#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

class ComicBookInformationModel;

/**
 * @brief Модель синопсиса комикса
 */
class CORE_LIBRARY_EXPORT ComicBookSynopsisModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit ComicBookSynopsisModel(QObject* _parent = nullptr);
    ~ComicBookSynopsisModel() override;

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

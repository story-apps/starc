#pragma once

#include "../text/text_model.h"


namespace BusinessLayer {

class ScreenplayInformationModel;

/**
 * @brief Модель
 */
class CORE_LIBRARY_EXPORT ScreenplayTitlePageModel : public TextModel
{
    Q_OBJECT

public:
    explicit ScreenplayTitlePageModel(QObject* _parent = nullptr);
    ~ScreenplayTitlePageModel() override;

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

    /**
     * @brief Задать модель информации о сценарии
     */
    void setInformationModel(ScreenplayInformationModel* _model);
    ScreenplayInformationModel* informationModel() const;

protected:
    /**
     * @brief Реализуем собственную инициализацию для пустого документа
     */
    void initDocument() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

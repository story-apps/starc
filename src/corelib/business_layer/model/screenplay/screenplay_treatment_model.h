#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

/**
 * @brief Модель
 */
class CORE_LIBRARY_EXPORT ScreenplayTreatmentModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplayTreatmentModel(QObject* _parent = nullptr);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */
};

} // namespace BusinessLayer

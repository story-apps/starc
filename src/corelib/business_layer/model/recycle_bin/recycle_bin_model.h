#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

/**
 * @brief Модель корзины с удалёнными документами
 */
class CORE_LIBRARY_EXPORT RecycleBinModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit RecycleBinModel(QObject* _parent = nullptr);

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

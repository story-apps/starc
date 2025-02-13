#pragma once

#include "../../abstract_model.h"


namespace BusinessLayer {

/**
 * @brief Модель статистики сериала
 */
class CORE_LIBRARY_EXPORT ScreenplaySeriesEpisodesModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplaySeriesEpisodesModel(QObject* _parent = nullptr);
    ~ScreenplaySeriesEpisodesModel() override;

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

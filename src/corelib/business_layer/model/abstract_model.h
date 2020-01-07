#pragma once

#include <QAbstractItemModel>


namespace BusinessLayer {

/**
 * @brief Абстрактная модель для работы над документами
 */
class AbstractModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AbstractModel(QObject *_parent = nullptr);
};

} // namespace BusinessLayer

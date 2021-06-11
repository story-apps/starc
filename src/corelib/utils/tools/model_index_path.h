#pragma once

#include <QModelIndex>

#include <corelib_global.h>


/**
 * @brief Класс строящий путь элемента по его индексу, для последующей возможности сравнения
 */
class CORE_LIBRARY_EXPORT ModelIndexPath
{
public:
    explicit ModelIndexPath(const QModelIndex& _index);

    bool operator<(const ModelIndexPath& _other) const;

private:
    const QModelIndex m_index;
    QList<int> m_path;
};

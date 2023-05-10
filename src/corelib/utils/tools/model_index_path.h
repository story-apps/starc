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

    QList<int> path() const;

    bool operator==(const ModelIndexPath& _other) const;
    bool operator<(const ModelIndexPath& _other) const;

private:
    const QModelIndex m_index;
    QList<int> m_path;
};

inline uint qHash(ModelIndexPath _tag, uint _seed = 0)
{
    return qHash(_tag.path(), _seed);
}

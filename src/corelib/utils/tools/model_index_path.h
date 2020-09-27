#pragma once

#include <QModelIndex>


/**
 * @brief Класс строящий путь элемента по его индексу, для последующей возможности сравнения
 */
class ModelIndexPath
{
public:
    explicit ModelIndexPath(const QModelIndex& _index);

    bool operator<(const ModelIndexPath& _other) const;

private:
    const QModelIndex m_index;
    QList<int> m_path;
};

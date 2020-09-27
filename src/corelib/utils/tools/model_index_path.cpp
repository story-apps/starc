#include "model_index_path.h"


ModelIndexPath::ModelIndexPath(const QModelIndex& _index)
    : m_index(_index)
{
    QModelIndex parent = _index;
    while (parent.isValid()) {
        m_path.prepend(parent.row());
        parent = parent.parent();
    }
}

bool ModelIndexPath::operator<(const ModelIndexPath& _other) const
{
    return m_path < _other.m_path;
}

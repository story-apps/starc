#pragma once

#include <QVector>

namespace edit_distance
{

//
// Наивная реализация взята из https://www.geeksforgeeks.org/edit-distance-dp-5/
// TODO: Использовать более оптимальное решение
//

enum class OperationType {
    Skip,
    Insert,
    Remove,
    Replace
};

template<typename T>
struct Operation {
    OperationType type;
    T* value;
};

namespace {

template<typename T>
QVector<Operation<T>> minimumDistance(const QVector<Operation<T>>& _x, const QVector<Operation<T>>& _y, const QVector<Operation<T>>& _z) {
    auto operationWeight = [] (const QVector<Operation<T>>& _operations) {
        int weight = 0;
        for (const auto& operation : _operations) {
            weight += operation.type == OperationType::Skip ? 0 : 1;
        }
        return weight;
    };

    const auto xWeight = operationWeight(_x);
    const auto yWeight = operationWeight(_y);
    const auto zWeight = operationWeight(_z);
    if (xWeight < yWeight && xWeight < zWeight) {
        return _x;
    } else if (yWeight < xWeight && yWeight < zWeight) {
        return _y;
    } else {
        return _z;
    }
};

template<typename T>
QVector<Operation<T>> editDistanceImpl(const QVector<T*>& source, int sourceIndex,
    const QVector<T*>& target, int targetIndex, const QVector<Operation<T>>& _operations) {
    auto operations = _operations;

    // If first string is empty, the only option is to
    // insert all characters of second string into first
    if (sourceIndex == 0) {
        for (int index = targetIndex - 1; index >= 0; --index) {
            operations.prepend({OperationType::Insert, target.at(index)});
        }
        return operations;
    }

    // If second string is empty, the only option is to
    // remove all characters of first string
    if (targetIndex == 0) {
        for (int index = sourceIndex - 1; index >= 0; --index) {
            operations.prepend({OperationType::Remove, nullptr});
        }
        return operations;
    }

    // If last characters of two strings are same, nothing
    // much to do. Ignore last characters and get count for
    // remaining strings.
    if (source.at(sourceIndex - 1)->isEqual(target.at(targetIndex - 1))) {
        operations.prepend({OperationType::Skip, nullptr});
        return editDistanceImpl(source, sourceIndex - 1, target, targetIndex - 1, operations);
    }

    // If last characters are not same, consider all three
    // operations on last character of first string,
    // recursively compute minimum cost for all three
    // operations and take minimum of three values.
    auto operationsWithInsert = operations;
    operationsWithInsert.prepend({OperationType::Insert, target.at(targetIndex - 1)});
    operationsWithInsert = editDistanceImpl(source, sourceIndex, target, targetIndex - 1, operationsWithInsert);
    //
    auto operationsWithRemove = operations;
    operationsWithRemove.prepend({OperationType::Remove, {}});
    operationsWithRemove = editDistanceImpl(source, sourceIndex - 1, target, targetIndex, operationsWithRemove);
    //
    auto operationsWithReplace = operations;
    operationsWithReplace.prepend({OperationType::Replace, target.at(targetIndex - 1)});
    operationsWithReplace = editDistanceImpl(source, sourceIndex - 1, target, targetIndex - 1, operationsWithReplace);
    //
    return minimumDistance(operationsWithInsert,
                           operationsWithRemove,
                           operationsWithReplace);
};

} // namespace


template<typename T>
QVector<Operation<T>> editDistance(const QVector<T*>& _source, const QVector<T*>& _target)
{
    return editDistanceImpl(_source, _source.size(), _target, _target.size(), {});
}

} // namespace edit_distance

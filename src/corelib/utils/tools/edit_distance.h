#pragma once

#include <QVector>

namespace edit_distance
{

//
// Реализация взята из https://www.geeksforgeeks.org/edit-distance-dp-5/
//

enum class OperationType {
    Skip,
    Insert,
    Remove,
    Replace
};

template<typename T>
struct Operation {
    OperationType type = OperationType::Skip;
    T* value = nullptr;
    int weight = 0;
};

namespace {

template<typename T>
QVector<Operation<T>> minimumDistance(const QVector<Operation<T>>& _x, const QVector<Operation<T>>& _y, const QVector<Operation<T>>& _z) {
    auto operationWeight = [] (const QVector<Operation<T>>& _operations) {
        int weight = 0;
        for (const auto& operation : _operations) {
            //
            // Из-за того, что для некоторых кейсов вес может быть максимальным значением int,
            // берём максимум, чтобы не было переполнения
            //
            weight = std::max(weight + operation.weight, operation.weight);
        }
        return weight;
    };

    const auto xWeight = operationWeight(_x);
    const auto yWeight = operationWeight(_y);
    const auto zWeight = operationWeight(_z);
    const auto minimumWeight = std::min({ xWeight, yWeight, zWeight });
    if (minimumWeight == xWeight) {
        return _x;
    } else if (minimumWeight == yWeight) {
        return _y;
    } else {
        return _z;
    }
};

template<typename T>
bool isItemsParentsEqual(T* _lhs, T* _rhs) {
    return (_lhs->parent() == nullptr && _rhs->parent() == nullptr)
            || (_lhs->parent() != nullptr && _rhs->parent() != nullptr && _lhs->parent()->isEqual(_rhs->parent()));
}

template<typename T>
bool isItemsEqual(T* _lhs, T* _rhs) {
    return _lhs->isEqual(_rhs)
            && isItemsParentsEqual(_lhs, _rhs);
}

template<typename T>
int replaceOperationWeight(T* _lhs, T* _rhs) {
    //
    // Исключаем замену элементов разных типов
    //
    if (_lhs->type() != _rhs->type()) {
        return std::numeric_limits<int>::max();
    }

    return 1;
}

} // namespace


template<typename T>
QVector<Operation<T>> editDistance(const QVector<T*>& _source, const QVector<T*>& _target)
{
    // Create a DP array to memoize result
    // of previous computations
    QVector<QVector<QVector<Operation<T>>>> DP;
    DP.resize(2);
    DP[0].resize(_source.size() + 1);
    DP[1].resize(_source.size() + 1);

    // Base condition when second string
    // is empty then we remove all characters
    for (int i = 1; i <= _source.size(); i++) {
        auto operations = DP[0][i - 1];
        operations.append({OperationType::Remove, nullptr, 1});
        DP[0][i] = operations;
    }

    // Start filling the DP
    // This loop run for every
    // character in second string
    for (int i = 1; i <= _target.size(); i++) {
        // This loop compares the char from
        // second string with first string
        // characters
        for (int j = 0; j <= _source.size(); j++) {
            // if first string is empty then
            // we have to perform add character
            // operation to get second string
            if (j == 0) {
                auto operations = DP[(i - 1) % 2][j];
                operations.append({OperationType::Insert, _target[i - 1], 1});
                DP[i % 2][j] = operations;
            }

            // if character from both string
            // is same then we do not perform any
            // operation . here i % 2 is for bound
            // the row number.
            else if (isItemsEqual(_source[j - 1], _target[i - 1])) {
                auto operations = DP[(i - 1) % 2][j - 1];
                operations.append({OperationType::Skip, _target[i - 1], 0});
                DP[i % 2][j] = operations;
            }

            // if character from both string is
            // not same then we take the minimum
            // from three specified operation
            else {
                auto operationsWithInsert = DP[(i - 1) % 2][j];
                operationsWithInsert.append({OperationType::Insert, _target[i - 1], 1});
                //
                auto operationsWithRemove = DP[i % 2][j - 1];
                operationsWithRemove.append({OperationType::Remove, nullptr, 1});
                //
                auto operationsWithReplace = DP[(i - 1) % 2][j - 1];
                operationsWithReplace.append({OperationType::Replace, _target[i - 1],
                                              replaceOperationWeight(_source[j - 1], _target[i - 1])});
                //
                DP[i % 2][j] = minimumDistance(operationsWithInsert,
                                               operationsWithRemove,
                                               operationsWithReplace);
            }
        }
    }

    // after complete fill the DP array
    // if the len2 is even then we end
    // up in the 0th row else we end up
    // in the 1th row so we take len2 % 2
    // to get row
    return DP[_target.size() % 2][_source.size()];
}

} // namespace edit_distance

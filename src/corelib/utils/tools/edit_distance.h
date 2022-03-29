#pragma once

#include <utils/shugar.h>

#include <QVector>

namespace edit_distance {

//
// Реализация взята из https://www.geeksforgeeks.org/edit-distance-dp-5/
//

enum class OperationType { Skip, Insert, Remove, Replace };

template<typename T>
struct Operation {
    OperationType type = OperationType::Skip;
    T* value = nullptr;
    int weight = 0;
};

namespace {

template<typename T>
QVector<Operation<T>> minimumDistance(const QVector<Operation<T>>& _x,
                                      const QVector<Operation<T>>& _y,
                                      const QVector<Operation<T>>& _z)
{
    auto operationWeight = [](const QVector<Operation<T>>& _operations) {
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
bool isItemsParentsEqual(T* _lhs, T* _rhs)
{
    return (_lhs->parent() == nullptr && _rhs->parent() == nullptr)
        || (_lhs->parent() != nullptr && _rhs->parent() != nullptr
            && _lhs->parent()->isEqual(_rhs->parent()));
}

template<typename T>
bool isItemsEqual(T* _lhs, T* _rhs)
{
    return _lhs->isEqual(_rhs) && isItemsParentsEqual(_lhs, _rhs);
}

template<typename T>
Operation<T> skipOperation(T* _item)
{
    return { OperationType::Skip, _item, 0 };
}

template<typename T>
Operation<T> insertOperation(T* _item)
{
    return { OperationType::Insert, _item, 1 };
}

template<typename T>
Operation<T> removeOperation(const QVector<Operation<T>>& _operations, T* _item)
{
    int weight = 1;

    //
    // FIXME: придумать, как это более изящно обыграть (перенести в элемент модели)
    //
    if (_item->toXml() == "<splitter type=\"end\"/>\n") {
        weight = 100;
        for (const auto& operation : reversed(_operations)) {
            if (operation.type == OperationType::Remove
                && operation.value->toXml() == "<splitter type=\"start\"/>\n") {
                weight = 1;
                break;
            }
        }
    }

    return { OperationType::Remove, _item, weight };
}

template<typename T>
Operation<T> replaceOperation(T* _lhs, T* _rhs)
{
    int weight = 1;

    if (_lhs->type() != _rhs->type() || _lhs->subtype() != _rhs->subtype()) {
        weight = std::numeric_limits<int>::max();
    }

    return { OperationType::Replace, _rhs, weight };
}

} // namespace


template<typename T>
QVector<Operation<T>> editDistance(const QVector<T*>& _source, const QVector<T*>& _target)
{
    //
    // Определим хранилище для кеша просчитанных путей
    //
    QVector<QVector<QVector<Operation<T>>>> opertionsCache;
    opertionsCache.resize(2);
    opertionsCache[0].resize(_source.size() + 1);
    opertionsCache[1].resize(_source.size() + 1);

    //
    // Базовое состояние, когда вторая строка пуста - значит нужно просто удалить все символы
    //
    for (int i = 1; i <= _source.size(); i++) {
        auto operations = opertionsCache[0][i - 1];
        operations.append(removeOperation(operations, _source[i - 1]));
        opertionsCache[0][i] = operations;
    }

    //
    // Начинаем обсчитывать пути и заполнять кэш
    //
    for (int i = 1; i <= _target.size(); i++) {
        //
        // Сравниваем посимвольно вторую строку с первой
        //
        for (int j = 0; j <= _source.size(); j++) {
            //
            // Если первая строка пуста, значит нужно просто добавлять символы второй
            //
            if (j == 0) {
                auto operations = opertionsCache[(i - 1) % 2][j];
                operations.append(insertOperation(_target[i - 1]));
                opertionsCache[i % 2][j] = operations;
            }
            //
            // Если символы обеих строк равны, то берём минимальную из трёх операций
            // 1. Вставка символа второй строки
            // 2. Удаление символа первой строки
            // 3. Пропуск текущего символа
            //
            // NOTE: В базовой реализации, в этом кейсе просто используется операция пропуска
            //       текущего символа, но нам она не подходит, из-за того, что у нас разные операции
            //       имеют разные веса
            //
            else if (isItemsEqual(_source[j - 1], _target[i - 1])) {
                auto operationsWithInsert = opertionsCache[(i - 1) % 2][j];
                operationsWithInsert.append(insertOperation(_target[i - 1]));
                //
                auto operationsWithRemove = opertionsCache[i % 2][j - 1];
                operationsWithRemove.append(removeOperation(operationsWithRemove, _source[j - 1]));
                //
                auto operationsWithSkip = opertionsCache[(i - 1) % 2][j - 1];
                operationsWithSkip.append(skipOperation(_target[i - 1]));
                //
                opertionsCache[i % 2][j] = minimumDistance(
                    operationsWithInsert, operationsWithRemove, operationsWithSkip);
            }
            //
            // Если символы разные, то берём минимальную из трёх операций
            // 1. Вставка символа второй строки
            // 2. Удаление символа первой строки
            // 3. Замена символа первой строки на символ второй
            //
            else {
                auto operationsWithInsert = opertionsCache[(i - 1) % 2][j];
                operationsWithInsert.append(insertOperation(_target[i - 1]));
                //
                auto operationsWithRemove = opertionsCache[i % 2][j - 1];
                operationsWithRemove.append(removeOperation(operationsWithRemove, _source[j - 1]));
                //
                auto operationsWithReplace = opertionsCache[(i - 1) % 2][j - 1];
                operationsWithReplace.append(replaceOperation(_source[j - 1], _target[i - 1]));
                //
                opertionsCache[i % 2][j] = minimumDistance(
                    operationsWithInsert, operationsWithRemove, operationsWithReplace);
            }
        }
    }

    //
    // В итоге получим кратчайший и самый оптимальный набор операций для замены строк
    //
    return opertionsCache[_target.size() % 2][_source.size()];
}

} // namespace edit_distance

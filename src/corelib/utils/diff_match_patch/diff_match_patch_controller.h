#pragma once

#include <QScopedPointer>


/**
 * @brief Управляющий классом сравнения данных документов
 */
class DiffMatchPatchController final
{
public:
    explicit DiffMatchPatchController(const QVector<QString>& _tags);
    ~DiffMatchPatchController();

    /**
     * @brief Сформировать патч
     */
    QByteArray makePatch(const QString& _lhs, const QString& _rhs);

    /**
     * @brief Применить патч
     */
    QByteArray applyPatch(const QByteArray& _content, const QByteArray& _patch);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

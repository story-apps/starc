#pragma once

#include <QScopedPointer>
#include <QString>

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
template<typename, typename>
struct QPair;
#endif


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
    QByteArray makePatch(const QString& _lhs, const QString& _rhs) const;

    /**
     * @brief Применить патч
     */
    QByteArray applyPatch(const QByteArray& _content, const QByteArray& _patch) const;

    /**
     * @brief Определить куски xml из документов, которые затрагивает данное изменение
     * @return Пара: 1) текст, который был изменён; 2) текст замены
     */
    struct Change {
        QByteArray xml;
        int from = 0;
    };
    QPair<Change, Change> changedXml(const QString& _xml, const QString& _patch) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

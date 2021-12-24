#pragma once

#include <QHeaderView>
#include <QSortFilterProxyModel>

#include <corelib_global.h>


/**
 * @brief Собственная реализация заголовков для дерева
 */
class CORE_LIBRARY_EXPORT TreeHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    explicit TreeHeaderView(QWidget* _parent = nullptr);

    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем собственную отрисовку заголовка
     */
    void paintSection(QPainter* _painter, const QRect& _rect, int _section) const override;
};


//
// TODO: Переделать эту херь, совершенно непонятно, зачем нужно вводить дополнительную модель,
// которая в себе содержит ещё и модель заголовков, а не просто установить модель заголовков в
// представление хидера
//


/**
 * @brief Иерархические заголовки
 */
class CORE_LIBRARY_EXPORT HierarchicalHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    enum HeaderDataModelRoles {
        HorizontalHeaderDataRole = Qt::UserRole,
        VerticalHeaderDataRole = Qt::UserRole + 1
    };

    HierarchicalHeaderView(QWidget* _parent = nullptr);
    ~HierarchicalHeaderView();

    void setModel(QAbstractItemModel* _model);

protected:
    void paintSection(QPainter* _painter, const QRect& _rect, int _logicalIndex) const;

    QSize sectionSizeFromContents(int _logicalIndex) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


/**
 * @brief Модель, которая имеет модель заголовков (sic)
 */
class CORE_LIBRARY_EXPORT HierarchicalModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit HierarchicalModel(QObject* _parent = nullptr);

    void setHeaderModel(QAbstractItemModel* _model);
    QAbstractItemModel* headerModel() const;

    QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const;

private:
    QAbstractItemModel* m_headerModel = nullptr;
};

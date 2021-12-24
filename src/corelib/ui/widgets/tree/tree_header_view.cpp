#include "tree_header_view.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QPointer>


TreeHeaderView::TreeHeaderView(QWidget* _parent)
    : QHeaderView(Qt::Horizontal, _parent)
{
}

QSize TreeHeaderView::sizeHint() const
{
    return QSize(QHeaderView::sizeHint().width(), Ui::DesignSystem::treeOneLineItem().height());
}

void TreeHeaderView::paintSection(QPainter* _painter, const QRect& _rect, int _section) const
{
    const auto backgroundColor = palette().color(QPalette::Base);
    _painter->fillRect(_rect, backgroundColor);

    auto textColor = palette().color(QPalette::Text);
    textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    _painter->setPen(textColor);
    auto textRect
        = _rect.adjusted(_section == 0 ? Ui::DesignSystem::tree().indicatorWidth()
                                       : Ui::DesignSystem::treeOneLineItem().margins().left(),
                         0, 0, 0);
    if (model() && model()->index(0, _section).data(Qt::DecorationRole).isValid()) {
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        QRectF iconRect(
            textRect.topLeft(),
            QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(), textRect.height()));
        _painter->drawText(iconRect, Qt::AlignCenter,
                           model()->index(0, _section).data(Qt::DecorationRole).toString());

        textRect.adjust(Ui::DesignSystem::treeOneLineItem().iconSize().width()
                            + Ui::DesignSystem::treeOneLineItem().spacing(),
                        0, 0, 0);
    }
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                       model()->headerData(_section, orientation()).toString());
}


// ****


//
// TODO: выпилить всё про вертикальные заголовки
//
class HierarchicalHeaderView::Implementation
{
public:
    explicit Implementation(HierarchicalHeaderView* _q);

    void initFromNewModel(int orientation, QAbstractItemModel* model);

    QModelIndex findRootIndex(QModelIndex index) const;

    QModelIndexList parentIndexes(QModelIndex index) const;

    QModelIndex findLeaf(const QModelIndex& curentIndex, int sectionIndex, int& curentLeafIndex);

    QModelIndex leafIndex(int sectionIndex);

    QModelIndexList searchLeafs(const QModelIndex& _currentIndex) const;

    QModelIndexList leafs(const QModelIndex& searchedIndex) const;

    void setForegroundBrush(QStyleOptionHeader& opt, const QModelIndex& index) const;

    void setBackgroundBrush(QStyleOptionHeader& opt, const QModelIndex& index) const;

    QSize cellSize(const QModelIndex& leafIndex, const QHeaderView* hv,
                   QStyleOptionHeader styleOptions) const;

    int currentCellWidth(const QModelIndex& searchedIndex, const QModelIndex& leafIndex,
                         int sectionIndex, const QHeaderView* hv) const;

    int currentCellLeft(const QModelIndex& searchedIndex, const QModelIndex& leafIndex,
                        int sectionIndex, int left, const QHeaderView* hv) const;

    int paintHorizontalCell(QPainter* _painter, const QHeaderView* hv, const QModelIndex& cellIndex,
                            const QModelIndex& leafIndex, int logicalLeafIndex,
                            const QStyleOptionHeader& styleOptions, const QRect& sectionRect,
                            int top) const;

    void paintHorizontalSection(QPainter* painter, const QRect& sectionRect, int logicalLeafIndex,
                                const QHeaderView* hv, const QStyleOptionHeader& styleOptions,
                                const QModelIndex& leafIndex) const;

    int paintVerticalCell(QPainter* painter, const QHeaderView* hv, const QModelIndex& cellIndex,
                          const QModelIndex& leafIndex, int logicalLeafIndex,
                          const QStyleOptionHeader& styleOptions, const QRect& sectionRect,
                          int left) const;

    void paintVerticalSection(QPainter* painter, const QRect& sectionRect, int logicalLeafIndex,
                              const QHeaderView* hv, const QStyleOptionHeader& styleOptions,
                              const QModelIndex& leafIndex) const;

    QStyleOptionHeader styleOptionForCell(int logicalInd) const;


    HierarchicalHeaderView* q = nullptr;
    QPointer<QAbstractItemModel> headerModel;
};

HierarchicalHeaderView::Implementation::Implementation(HierarchicalHeaderView* _q)
    : q(_q)
{
}

void HierarchicalHeaderView::Implementation::initFromNewModel(int orientation,
                                                              QAbstractItemModel* model)
{
    headerModel = QPointer<QAbstractItemModel>();
    QVariant v(model->data(
        QModelIndex(),
        (orientation == Qt::Horizontal ? HorizontalHeaderDataRole : VerticalHeaderDataRole)));
    if (v.isValid()) {
        headerModel = qobject_cast<QAbstractItemModel*>(v.value<QObject*>());
    }
}

QModelIndex HierarchicalHeaderView::Implementation::findRootIndex(QModelIndex index) const
{
    while (index.parent().isValid()) {
        index = index.parent();
    }
    return index;
}

QModelIndexList HierarchicalHeaderView::Implementation::parentIndexes(QModelIndex index) const
{
    QModelIndexList indexes;
    while (index.isValid()) {
        indexes.push_front(index);
        index = index.parent();
    }
    return indexes;
}

QModelIndex HierarchicalHeaderView::Implementation::findLeaf(const QModelIndex& curentIndex,
                                                             int sectionIndex, int& curentLeafIndex)
{
    if (curentIndex.isValid()) {
        const auto model = curentIndex.model();
        int childCount = model->columnCount(curentIndex);
        if (childCount) {
            for (int i = 0; i < childCount; ++i) {
                const auto res
                    = findLeaf(model->index(0, i, curentIndex), sectionIndex, curentLeafIndex);
                if (res.isValid()) {
                    return res;
                }
            }
        } else {
            ++curentLeafIndex;
            if (curentLeafIndex == sectionIndex) {
                return curentIndex;
            }
        }
    }
    return {};
}

QModelIndex HierarchicalHeaderView::Implementation::leafIndex(int sectionIndex)
{
    if (headerModel) {
        int curentLeafIndex = -1;
        for (int i = 0; i < headerModel->columnCount(); ++i) {
            const auto res = findLeaf(headerModel->index(0, i), sectionIndex, curentLeafIndex);
            if (res.isValid()) {
                return res;
            }
        }
    }
    return {};
}

QModelIndexList HierarchicalHeaderView::Implementation::searchLeafs(
    const QModelIndex& _currentIndex) const
{
    if (!_currentIndex.isValid()) {
        return {};
    }

    QModelIndexList indexes;
    const auto model = _currentIndex.model();
    int childCount = model->columnCount(_currentIndex);
    if (childCount) {
        for (int i = 0; i < childCount; ++i) {
            indexes += searchLeafs(model->index(0, i, _currentIndex));
        }
    } else {
        indexes.push_back(_currentIndex);
    }
    return indexes;
}

QModelIndexList HierarchicalHeaderView::Implementation::leafs(
    const QModelIndex& searchedIndex) const
{
    QModelIndexList leafs;
    if (searchedIndex.isValid()) {
        const auto model = searchedIndex.model();
        int childCount = model->columnCount(searchedIndex);
        for (int i = 0; i < childCount; ++i)
            leafs += searchLeafs(model->index(0, i, searchedIndex));
    }
    return leafs;
}

void HierarchicalHeaderView::Implementation::setForegroundBrush(QStyleOptionHeader& opt,
                                                                const QModelIndex& index) const
{
    QVariant foregroundBrush = index.data(Qt::ForegroundRole);
    if (foregroundBrush.canConvert(QMetaType::QBrush))
        opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));
}

void HierarchicalHeaderView::Implementation::setBackgroundBrush(QStyleOptionHeader& opt,
                                                                const QModelIndex& index) const
{
    QVariant backgroundBrush = index.data(Qt::BackgroundRole);
    if (backgroundBrush.canConvert(QMetaType::QBrush)) {
        opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
        opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
    }
}

QSize HierarchicalHeaderView::Implementation::cellSize(const QModelIndex& leafIndex,
                                                       const QHeaderView* hv,
                                                       QStyleOptionHeader styleOptions) const
{
    QSize res;
    QVariant variant(leafIndex.data(Qt::SizeHintRole));
    if (variant.isValid()) {
        res = qvariant_cast<QSize>(variant);
    }
    QFont fnt(hv->font());
    QVariant var(leafIndex.data(Qt::FontRole));
    if (var.isValid() && var.canConvert(QMetaType::QFont)) {
        fnt = qvariant_cast<QFont>(var);
    }
    fnt.setBold(true);
    QFontMetrics fm(fnt);
    QSize size(fm.size(0, leafIndex.data(Qt::DisplayRole).toString()));
    if (leafIndex.data(Qt::UserRole).isValid()) {
        size.transpose();
    }
    QSize decorationsSize(
        hv->style()->sizeFromContents(QStyle::CT_HeaderSection, &styleOptions, QSize(), hv));
    return res.expandedTo({ size.width() + decorationsSize.width(),
                            static_cast<int>(Ui::DesignSystem::treeOneLineItem().height()) });
}

int HierarchicalHeaderView::Implementation::currentCellWidth(const QModelIndex& searchedIndex,
                                                             const QModelIndex& leafIndex,
                                                             int sectionIndex,
                                                             const QHeaderView* hv) const
{
    QModelIndexList leafsList(leafs(searchedIndex));
    if (leafsList.empty())
        return hv->sectionSize(sectionIndex);
    int width = 0;
    int firstLeafSectionIndex = sectionIndex - leafsList.indexOf(leafIndex);
    for (int i = 0; i < leafsList.size(); ++i)
        width += hv->sectionSize(firstLeafSectionIndex + i);
    return width;
}

int HierarchicalHeaderView::Implementation::currentCellLeft(const QModelIndex& searchedIndex,
                                                            const QModelIndex& leafIndex,
                                                            int sectionIndex, int left,
                                                            const QHeaderView* hv) const
{
    QModelIndexList leafsList(leafs(searchedIndex));
    if (!leafsList.empty()) {
        int n = leafsList.indexOf(leafIndex);
        int firstLeafSectionIndex = sectionIndex - n;
        --n;
        for (; n >= 0; --n) {
            if (QLocale().textDirection() == Qt::LeftToRight) {
                left -= hv->sectionSize(firstLeafSectionIndex + n);
            } else {
                left += hv->sectionSize(firstLeafSectionIndex + n);
            }
        }
    }
    return left;
}

int HierarchicalHeaderView::Implementation::paintHorizontalCell(
    QPainter* _painter, const QHeaderView* hv, const QModelIndex& cellIndex,
    const QModelIndex& leafIndex, int logicalLeafIndex, const QStyleOptionHeader& styleOptions,
    const QRect& sectionRect, int top) const
{
    int height = cellSize(cellIndex, hv, styleOptions).height();
    if (cellIndex == leafIndex) {
        height = sectionRect.height() - top;
    }
    int width = currentCellWidth(cellIndex, leafIndex, logicalLeafIndex, hv);
    int left = currentCellLeft(cellIndex, leafIndex, logicalLeafIndex, sectionRect.left(), hv);
    if (QLocale().textDirection() == Qt::RightToLeft && width > sectionRect.width()) {
        left -= width - sectionRect.width();
    }

    QRect rect(left, top, width, height);

    const auto backgroundColor = q->palette().color(QPalette::Base);
    _painter->fillRect(rect, backgroundColor);

    auto textColor = q->palette().color(QPalette::Text);
    textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    _painter->setPen(textColor);
    auto textRect = rect.adjusted(logicalLeafIndex == 0
                                      ? Ui::DesignSystem::tree().indicatorWidth()
                                          + Ui::DesignSystem::treeOneLineItem().spacing()
                                      : Ui::DesignSystem::treeOneLineItem().margins().left(),
                                  0, 0, 0);
    if (q->model() && cellIndex.data(Qt::DecorationRole).isValid()) {
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        QRectF iconRect(
            textRect.topLeft(),
            QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(), textRect.height()));
        _painter->drawText(iconRect, Qt::AlignCenter,
                           cellIndex.data(Qt::DecorationRole).toString());

        textRect.adjust(Ui::DesignSystem::treeOneLineItem().iconSize().width()
                            + Ui::DesignSystem::treeOneLineItem().spacing(),
                        0, 0, 0);
    }
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, cellIndex.data().toString());

    return top + height;
}

void HierarchicalHeaderView::Implementation::paintHorizontalSection(
    QPainter* painter, const QRect& sectionRect, int logicalLeafIndex, const QHeaderView* hv,
    const QStyleOptionHeader& styleOptions, const QModelIndex& leafIndex) const
{
    QPointF oldBO(painter->brushOrigin());
    int top = sectionRect.y();
    QModelIndexList indexes(parentIndexes(leafIndex));
    for (int i = 0; i < indexes.size(); ++i) {
        QStyleOptionHeader realStyleOptions(styleOptions);
        if (i < indexes.size() - 1
            && (realStyleOptions.state.testFlag(QStyle::State_Sunken)
                || realStyleOptions.state.testFlag(QStyle::State_On))) {
            QStyle::State t(QStyle::State_Sunken | QStyle::State_On);
            realStyleOptions.state &= (~t);
        }
        top = paintHorizontalCell(painter, hv, indexes[i], leafIndex, logicalLeafIndex,
                                  realStyleOptions, sectionRect, top);
    }
    painter->setBrushOrigin(oldBO);
}

int HierarchicalHeaderView::Implementation::paintVerticalCell(
    QPainter* painter, const QHeaderView* hv, const QModelIndex& cellIndex,
    const QModelIndex& leafIndex, int logicalLeafIndex, const QStyleOptionHeader& styleOptions,
    const QRect& sectionRect, int left) const
{
    QStyleOptionHeader uniopt(styleOptions);
    setForegroundBrush(uniopt, cellIndex);
    setBackgroundBrush(uniopt, cellIndex);

    int width = cellSize(cellIndex, hv, uniopt).width();
    if (cellIndex == leafIndex)
        width = sectionRect.width() - left;
    int top = currentCellLeft(cellIndex, leafIndex, logicalLeafIndex, sectionRect.top(), hv);
    int height = currentCellWidth(cellIndex, leafIndex, logicalLeafIndex, hv);

    QRect r(left, top, width, height);

    uniopt.text = cellIndex.data(Qt::DisplayRole).toString();
    painter->save();
    uniopt.rect = r;
    if (cellIndex.data(Qt::UserRole).isValid()) {
        hv->style()->drawControl(QStyle::CE_HeaderSection, &uniopt, painter, hv);
        QTransform matrix;
        matrix.rotate(-90);
        painter->setWorldTransform(matrix, true);
        QRect new_r(0, 0, r.height(), r.width());
        new_r.moveCenter(QPoint(-r.center().y(), r.center().x()));
        uniopt.rect = new_r;
        hv->style()->drawControl(QStyle::CE_HeaderLabel, &uniopt, painter, hv);
    } else {
        hv->style()->drawControl(QStyle::CE_Header, &uniopt, painter, hv);
    }
    painter->restore();
    return left + width;
}

void HierarchicalHeaderView::Implementation::paintVerticalSection(
    QPainter* painter, const QRect& sectionRect, int logicalLeafIndex, const QHeaderView* hv,
    const QStyleOptionHeader& styleOptions, const QModelIndex& leafIndex) const
{
    QPointF oldBO(painter->brushOrigin());
    int left = sectionRect.x();
    QModelIndexList indexes(parentIndexes(leafIndex));
    for (int i = 0; i < indexes.size(); ++i) {
        QStyleOptionHeader realStyleOptions(styleOptions);
        if (i < indexes.size() - 1
            && (realStyleOptions.state.testFlag(QStyle::State_Sunken)
                || realStyleOptions.state.testFlag(QStyle::State_On))) {
            QStyle::State t(QStyle::State_Sunken | QStyle::State_On);
            realStyleOptions.state &= (~t);
        }
        left = paintVerticalCell(painter, hv, indexes[i], leafIndex, logicalLeafIndex,
                                 realStyleOptions, sectionRect, left);
    }
    painter->setBrushOrigin(oldBO);
}

QStyleOptionHeader HierarchicalHeaderView::Implementation::styleOptionForCell(int logicalInd) const
{
    QStyleOptionHeader opt;
    q->initStyleOption(&opt);
    if (q->window()->isActiveWindow()) {
        opt.state |= QStyle::State_Active;
    }
    opt.textAlignment = Qt::AlignCenter;
    opt.iconAlignment = Qt::AlignVCenter;
    opt.section = logicalInd;

    int visual = q->visualIndex(logicalInd);

    const int count = q->count();
    if (count == 1) {
        opt.position = QStyleOptionHeader::OnlyOneSection;
    } else {
        if (visual == 0) {
            opt.position = QStyleOptionHeader::Beginning;
        } else {
            opt.position
                = (visual == count - 1 ? QStyleOptionHeader::End : QStyleOptionHeader::Middle);
        }
    }

    const auto orientation = q->orientation();
    const auto selectionModel = q->selectionModel();
    const auto rootIndex = q->rootIndex();
    if (q->sectionsClickable()) {
        /*
        if (logicalIndex == d->hover)
            state |= QStyle::State_MouseOver;
        if (logicalIndex == d->pressed)
        {
            state |= QStyle::State_Sunken;
        }
        else*/
        {
            if (q->highlightSections() && selectionModel) {
                if (orientation == Qt::Horizontal) {
                    if (selectionModel->columnIntersectsSelection(logicalInd, rootIndex)) {
                        opt.state |= QStyle::State_On;
                    }
                    if (selectionModel->isColumnSelected(logicalInd, rootIndex)) {
                        opt.state |= QStyle::State_Sunken;
                    }
                } else {
                    if (selectionModel->rowIntersectsSelection(logicalInd, rootIndex)) {
                        opt.state |= QStyle::State_On;
                    }
                    if (selectionModel->isRowSelected(logicalInd, rootIndex)) {
                        opt.state |= QStyle::State_Sunken;
                    }
                }
            }
        }
    }
    if (selectionModel) {
        bool previousSelected = false;
        if (orientation == Qt::Horizontal) {
            previousSelected
                = selectionModel->isColumnSelected(q->logicalIndex(visual - 1), rootIndex);
        } else {
            previousSelected
                = selectionModel->isRowSelected(q->logicalIndex(visual - 1), rootIndex);
        }
        bool nextSelected = false;
        if (orientation == Qt::Horizontal) {
            nextSelected = selectionModel->isColumnSelected(q->logicalIndex(visual + 1), rootIndex);
        } else {
            nextSelected = selectionModel->isRowSelected(q->logicalIndex(visual + 1), rootIndex);
        }
        if (previousSelected && nextSelected) {
            opt.selectedPosition = QStyleOptionHeader::NextAndPreviousAreSelected;
        } else {
            if (previousSelected) {
                opt.selectedPosition = QStyleOptionHeader::PreviousIsSelected;
            } else {
                if (nextSelected) {
                    opt.selectedPosition = QStyleOptionHeader::NextIsSelected;
                } else {
                    opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
                }
            }
        }
    }
    return opt;
}

// **

HierarchicalHeaderView::HierarchicalHeaderView(QWidget* _parent)
    : QHeaderView(Qt::Horizontal, _parent)
    , d(new Implementation(this))
{
    connect(this, &HierarchicalHeaderView::sectionResized, this, [this](int logicalIndex) {
        if (isSectionHidden(logicalIndex)) {
            return;
        }

        QModelIndex leafIndex(d->leafIndex(logicalIndex));
        if (!leafIndex.isValid()) {
            return;
        }

        QModelIndexList leafsList(d->leafs(d->findRootIndex(leafIndex)));
        for (int n = leafsList.indexOf(leafIndex); n > 0; --n) {
            --logicalIndex;

            int w = viewport()->width();
            int h = viewport()->height();
            int pos = sectionViewportPosition(logicalIndex);
            QRect r(pos, 0, w - pos, h);
            if (orientation() == Qt::Vertical) {
                r.setRect(0, pos, w, h - pos);
            }

            viewport()->update(r.normalized());
        }
    });
}

HierarchicalHeaderView::~HierarchicalHeaderView() = default;

QSize HierarchicalHeaderView::sectionSizeFromContents(int _logicalIndex) const
{
    if (d->headerModel) {
        QModelIndex curLeafIndex(d->leafIndex(_logicalIndex));
        if (curLeafIndex.isValid()) {
            QStyleOptionHeader styleOption(d->styleOptionForCell(_logicalIndex));
            QSize s(d->cellSize(curLeafIndex, this, styleOption));
            curLeafIndex = curLeafIndex.parent();
            while (curLeafIndex.isValid()) {
                if (orientation() == Qt::Horizontal) {
                    s.rheight() += d->cellSize(curLeafIndex, this, styleOption).height();
                } else {
                    s.rwidth() += d->cellSize(curLeafIndex, this, styleOption).width();
                }
                curLeafIndex = curLeafIndex.parent();
            }
            return s;
        }
    }

    return QHeaderView::sectionSizeFromContents(_logicalIndex);
}

void HierarchicalHeaderView::paintSection(QPainter* _painter, const QRect& _rect,
                                          int _logicalIndex) const
{
    if (_rect.isValid()) {
        QModelIndex leafIndex(d->leafIndex(_logicalIndex));
        if (leafIndex.isValid()) {
            if (orientation() == Qt::Horizontal) {
                d->paintHorizontalSection(_painter, _rect, _logicalIndex, this,
                                          d->styleOptionForCell(_logicalIndex), leafIndex);
            } else {
                d->paintVerticalSection(_painter, _rect, _logicalIndex, this,
                                        d->styleOptionForCell(_logicalIndex), leafIndex);
            }
            return;
        }
    }

    QHeaderView::paintSection(_painter, _rect, _logicalIndex);
}

void HierarchicalHeaderView::setModel(QAbstractItemModel* _model)
{
    d->initFromNewModel(orientation(), _model);

    QHeaderView::setModel(_model);

    const auto count
        = (orientation() == Qt::Horizontal ? _model->columnCount() : _model->rowCount());
    if (count > 0) {
        initializeSections(0, count - 1);
    }
}


// ****


HierarchicalModel::HierarchicalModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
{
}

void HierarchicalModel::setHeaderModel(QAbstractItemModel* _model)
{
    if (m_headerModel == _model) {
        return;
    }

    m_headerModel = _model;
    if (sourceModel()->columnCount() > 0) {
        emit headerDataChanged(Qt::Horizontal, 0, sourceModel()->columnCount() - 1);
    }
}

QAbstractItemModel* HierarchicalModel::headerModel() const
{
    return m_headerModel;
}

QVariant HierarchicalModel::data(const QModelIndex& _index, int _role) const
{
    if (m_headerModel && _role == HierarchicalHeaderView::HorizontalHeaderDataRole) {
        return QVariant::fromValue(m_headerModel);
    }

    return QSortFilterProxyModel::data(_index, _role);
}

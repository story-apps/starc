#include "scalable_wrapper.h"

#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_edit/completer/completer_text_edit.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>

#include <QAbstractItemView>
#include <QGestureEvent>
#include <QGraphicsProxyWidget>
#include <QScrollBar>
#include <QShortcut>

namespace {
    const qreal kDefaultZoomRange = 1.;
    const qreal kMinimumZoomRange = 0.1;
    const qreal kMaximumZoomRange = 10.;
}


class ScalableWrapper::Implementation
{
public:
    explicit Implementation(PageTextEdit* _editor);


    /**
     * @brief Сцена в которой будет позиционироваться редактор
     */
    QGraphicsScene* scene = nullptr;

    /**
     * @brief Указатель на сам редактор
     */
    QScopedPointer<PageTextEdit> editor;

    /**
     * @brief Графическое представление редактора
     */
    QGraphicsProxyWidget* editorProxy = nullptr;

    /**
     * @brief Коэффициент масштабирования
     */
    qreal zoomRange = kDefaultZoomRange;

    /**
     * @brief Инерционный тормоз масштабирования при помощи жестов
     */
    int gestureZoomInertionBreak = 0;

    /**
     * @brief Вспомогательный элемент, посредством которого настраивается размер полос прокрутки
     */
    QGraphicsRectItem* rect = nullptr;

    /**
     * @brief Включена ли синхронизация полос прокрутки с редактором текста
     */
    bool isScrollingSynchronizationActive = false;

};

ScalableWrapper::Implementation::Implementation(PageTextEdit* _editor)
    : scene(new QGraphicsScene),
      editor(_editor)
{
    Q_ASSERT(editor);
}


// ****


ScalableWrapper::ScalableWrapper(PageTextEdit* _editor, QWidget* _parent)
    : QGraphicsView(_parent),
      d(new Implementation(_editor))
{
    setFrameShape(QFrame::NoFrame);

    //
    // Настраиваем лучшее опции прорисовки
    //
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(AnchorUnderMouse);

    //
    // Отслеживаем жесты
    //
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);

    //
    // Предварительная настройка редактора текста
    //
    d->editor->setParent(nullptr);
    d->editor->installEventFilter(this);

    //
    // Настраиваем само представление
    //
    d->rect = d->scene->addRect(0, 0, 1, 1, QPen(), Qt::transparent);
    d->editorProxy = d->scene->addWidget(d->editor.data());
    d->editorProxy->setPos(0, 0);
    setScene(d->scene);

    //
    // Добавляем возможность масштабирования при помощи комбинаций Ctrl +/-
    //
    auto zoomInShortcut1 = new QShortcut(QKeySequence("Ctrl++"), this, 0, 0, Qt::WidgetShortcut);
    connect(zoomInShortcut1, &QShortcut::activated, this, &ScalableWrapper::zoomIn);
    auto zoomInShortcut2 = new QShortcut(QKeySequence("Ctrl+="), this, 0, 0, Qt::WidgetShortcut);
    connect(zoomInShortcut2, &QShortcut::activated, this, &ScalableWrapper::zoomIn);
    auto zoomOutShortcut = new QShortcut(QKeySequence("Ctrl+-"), this, 0, 0, Qt::WidgetShortcut);
    connect(zoomOutShortcut, &QShortcut::activated, this, &ScalableWrapper::zoomOut);

    if (auto editor = qobject_cast<CompleterTextEdit*>(d->editor.data())) {
        connect(editor, &CompleterTextEdit::popupShowed, [=] {
            const QPointF point = d->editorProxy->mapToScene(editor->completer()->popup()->pos());
            editor->completer()->popup()->move(mapToGlobal(mapFromScene(point)));
        });
    }
    connect(d->editor.data(), &PageTextEdit::cursorPositionChanged, this, &ScalableWrapper::cursorPositionChanged);

    //
    // Настроим синхронизацию полос прокрутки
    //
    initScrollBarsSyncing();
}

void ScalableWrapper::initScrollBarsSyncing()
{
    //
    // Отключаем действия полос прокрутки, чтобы в дальнейшем проксировать ими
    // полосы прокрутки самого редактора текста
    //
    horizontalScrollBar()->disconnect();
    verticalScrollBar()->disconnect();

    //
    // Синхронизация значения ролика в обе стороны
    //
    setupScrollingSynchronization(true);
}

qreal ScalableWrapper::zoomRange() const
{
    return d->zoomRange;
}

ScalableWrapper::~ScalableWrapper() = default;

void ScalableWrapper::setZoomRange(qreal _zoomRange)
{
    if (d->zoomRange != _zoomRange
        && _zoomRange >= kMinimumZoomRange
        && _zoomRange <= kMaximumZoomRange) {
        d->zoomRange = _zoomRange;
        emit zoomRangeChanged(d->zoomRange);

        scaleTextEdit();
        updateTextEditSize();
    }
}

void ScalableWrapper::zoomIn()
{
    setZoomRange(d->zoomRange + 0.1);
}

void ScalableWrapper::zoomOut()
{
    setZoomRange(d->zoomRange - 0.1);
}

QPoint ScalableWrapper::mapFromEditor(const QPoint& _position) const
{
    const QPointF point = d->editorProxy->mapToScene(_position);
    return mapFromScene(point);
}

bool ScalableWrapper::event(QEvent* _event)
{
    bool result = true;
    switch (_event->type()) {
        //
        // Определяем особый обработчик для жестов
        //
        case QEvent::Gesture: {
            gestureEvent(static_cast<QGestureEvent*>(_event));
            break;
        }

        //
        // Во время события paint корректируем размер встроенного редактора
        //
        case QEvent::Paint: {
            updateTextEditSize();
            result = QGraphicsView::event(_event);
            break;
        }

        //
        // После события обновления компоновки, полностью перенастраиваем полосы прокрутки
        //
        case QEvent::LayoutRequest: {
            setupScrollingSynchronization(false);

            result = QGraphicsView::event(_event);

            syncScrollBarWithTextEdit();
            setupScrollingSynchronization(true);
            break;
        }

        //
        // Прочие стандартные обработчики событий
        //
        default: {
            result = QGraphicsView::event(_event);

            //
            // Переустанавливаем фокус в редактор, иначе в нём пропадает курсор
            //
            if (_event->type() == QEvent::FocusIn) {
                d->editor->clearFocus();
                d->editor->setFocus();
            } else if (_event->type() == QEvent::FocusOut) {
                d->editor->clearFocus();
            }

            break;
        }
    }

    return result;
}

void ScalableWrapper::wheelEvent(QWheelEvent* _event)
{
    //
    // Собственно масштабирование
    //
    if (_event->modifiers() & Qt::ControlModifier) {
        if (_event->orientation() == Qt::Vertical) {
#ifdef Q_OS_MAC
            const qreal angleDivider = 2.;
#else
            const qreal angleDivider = 120.;
#endif

            //
            // zoomRange > 0 - масштаб увеличивается
            // zoomRange < 0 - масштаб уменьшается
            //
            const qreal zoom = _event->angleDelta().y() / angleDivider;
            const qreal zoomCoefficientDivider = 10.;
            setZoomRange(d->zoomRange + zoom / zoomCoefficientDivider);

            _event->accept();
        }
    }
    //
    // В противном случае прокручиваем редактор
    //
    else {
        QGraphicsView::wheelEvent(_event);
    }
}

void ScalableWrapper::gestureEvent(QGestureEvent* _event)
{
    //
    // Жест масштабирования
    //
    QGesture* gesture = _event->gesture(Qt::PinchGesture);
    if (gesture == nullptr) {
        return;
    }

    auto pinch = qobject_cast<QPinchGesture *>(gesture);
    if (pinch == nullptr) {
        return;
    }

    //
    // При масштабировании за счёт жестов приходится немного притормаживать
    // т.к. события приходят слишком часто и при обработке каждого события
    // пользователю просто невозможно корректно настроить масштаб
    //

    const int kInertionBreak = 8;
    const qreal kZoomStep = 0.1;
    qreal zoomDelta = 0;
    if (pinch->scaleFactor() > 1) {
        if (d->gestureZoomInertionBreak < 0) {
            d->gestureZoomInertionBreak = 0;
        } else if (d->gestureZoomInertionBreak >= kInertionBreak) {
            d->gestureZoomInertionBreak = 0;
            zoomDelta = kZoomStep;
        } else {
            ++d->gestureZoomInertionBreak;
        }
    } else if (pinch->scaleFactor() < 1) {
        if (d->gestureZoomInertionBreak > 0) {
            d->gestureZoomInertionBreak = 0;
        } else if (d->gestureZoomInertionBreak <= -kInertionBreak) {
            d->gestureZoomInertionBreak = 0;
            zoomDelta = -1 * kZoomStep;
        } else {
            --d->gestureZoomInertionBreak;
        }
    } else {
        //
        // При обычной прокрутке часто приходит событие с scaledFactor == 1,
        // так что просто игнорируем их
        //
    }

    //
    // Если необходимо масштабируем и перерисовываем представление
    //
    if (zoomDelta != 0) {
        setZoomRange(d->zoomRange + zoomDelta);
    }

    _event->accept();
}

bool ScalableWrapper::eventFilter(QObject* _object, QEvent* _event)
{
    bool needShowMenu = false;
    QPoint cursorGlobalPos = QCursor::pos();
    switch (_event->type()) {
        case QEvent::ContextMenu: {
            QContextMenuEvent* contextMenuEvent = static_cast<QContextMenuEvent*>(_event);
            cursorGlobalPos = contextMenuEvent->globalPos();
            needShowMenu = true;
            break;
        }

        case QEvent::MouseButtonPress: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(_event);
            if (mouseEvent->button() == Qt::RightButton) {
                cursorGlobalPos = mouseEvent->globalPos();
                needShowMenu = true;
            }
            break;
        }

        default: {
            break;
        }
    }

    bool result = false;

    //
    // Если необходимо, то показываем контекстное меню
    //
    if (needShowMenu) {
        //
        // TODO: реализовать работу с новым контекстным меню
        //
        auto menu = d->editor->createContextMenu(d->editor->viewport()->mapFromGlobal(cursorGlobalPos), this);
        menu->showContextMenu(QCursor::pos());

        //
        // Событие перехвачено
        //
        result = true;
    }
    //
    // Если нет, то стандартная обработка события
    //
    else {
        //
        // Возвращаем фокус редактору, если он его потерял
        //
        if (_object == d->editor.data()
            && _event->type() == QEvent::FocusOut) {
            d->editor->clearFocus();
            d->editor->setFocus();
        }

        result = QGraphicsView::eventFilter(_object, _event);
    }

    return result;
}

void ScalableWrapper::showEvent(QShowEvent* _event)
{
    callEventWithScrollbarsTweak([this, _event] { QGraphicsView::showEvent(_event); });
}

void ScalableWrapper::resizeEvent(QResizeEvent* _event)
{
    callEventWithScrollbarsTweak([this, _event] { QGraphicsView::resizeEvent(_event); });
}

void ScalableWrapper::callEventWithScrollbarsTweak(std::function<void()> _callback)
{
    //
    // Перед событием отключаем синхронизацию полос прокрутки
    //
    setupScrollingSynchronization(false);

    //
    // Запускаем событие
    //
    _callback();

    //
    // ОБновляем размер редактора сценария
    //
    updateTextEditSize();

    //
    // Корректируем размер сцены, чтобы исключить внезапные смещения редактора на ней
    //
    if (d->scene->sceneRect() != viewport()->rect()) {
        setSceneRect(viewport()->rect());
        ensureVisible(d->editorProxy);
        syncScrollBarWithTextEdit();
    }

    //
    // А после события включаем синхронизацию
    //
    setupScrollingSynchronization(true);
}

void ScalableWrapper::setupScrollingSynchronization(bool _needSync)
{
    d->isScrollingSynchronizationActive = _needSync;

    if (_needSync) {
        connect(d->editor->verticalScrollBar(), &QScrollBar::rangeChanged, this, &ScalableWrapper::updateTextEditSize);
        connect(d->editor->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &ScalableWrapper::updateTextEditSize);
        //
        connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
                d->editor->verticalScrollBar(), SLOT(setValue(int)));
        connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
                d->editor->horizontalScrollBar(), SLOT(setValue(int)));
        // --
        connect(d->editor->verticalScrollBar(), SIGNAL(valueChanged(int)),
                verticalScrollBar(), SLOT(setValue(int)));
        connect(d->editor->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                horizontalScrollBar(), SLOT(setValue(int)));
    } else {
        disconnect(d->editor->verticalScrollBar(), &QScrollBar::rangeChanged, this, &ScalableWrapper::updateTextEditSize);
        disconnect(d->editor->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &ScalableWrapper::updateTextEditSize);
        //
        disconnect(verticalScrollBar(), SIGNAL(valueChanged(int)),
                d->editor->verticalScrollBar(), SLOT(setValue(int)));
        disconnect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
                d->editor->horizontalScrollBar(), SLOT(setValue(int)));
        // --
        disconnect(d->editor->verticalScrollBar(), SIGNAL(valueChanged(int)),
                verticalScrollBar(), SLOT(setValue(int)));
        disconnect(d->editor->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                horizontalScrollBar(), SLOT(setValue(int)));
    }
}

void ScalableWrapper::syncScrollBarWithTextEdit(bool _syncPosition)
{
    //
    // Скорректируем размерность полос прокрутки
    //
    const int rectHeight = d->editor->verticalScrollBar()->maximum();
    const int rectWidth = d->editor->horizontalScrollBar()->maximum();
    if (verticalScrollBar()->maximum() != rectHeight) {
        d->rect->setRect(0, 0, rectWidth, rectHeight);
        verticalScrollBar()->setMaximum(rectHeight);
        horizontalScrollBar()->setMaximum(rectWidth);
    }

    //
    // Скорректируем пользовательское взаимодействие с полосами прокрутки
    //
    verticalScrollBar()->setSingleStep(d->editor->verticalScrollBar()->singleStep());
    verticalScrollBar()->setPageStep(d->editor->verticalScrollBar()->pageStep());
    horizontalScrollBar()->setSingleStep(d->editor->horizontalScrollBar()->singleStep());
    horizontalScrollBar()->setPageStep(d->editor->horizontalScrollBar()->pageStep());

    //
    // Скорректируем положение полос прокрутки
    //
    if (_syncPosition) {
        verticalScrollBar()->setValue(d->editor->verticalScrollBar()->value());
        horizontalScrollBar()->setValue(d->editor->horizontalScrollBar()->value());
    }
}

void ScalableWrapper::updateTextEditSize()
{
    //
    // Задаём политику отображения полосы прокрутки.
    // При смене необходимо отключать синхронизацию, если она была активирована,
    // чтобы не происходило отбрасывание в нулевую позицию соседнего скролбара
    //
    auto setScrollBarVisibility = [this] (bool _isVerticalScrollBar, Qt::ScrollBarPolicy _policy) {
        const bool needSync = d->isScrollingSynchronizationActive;
        if (needSync) {
            setupScrollingSynchronization(false);
        }

        if (_isVerticalScrollBar
            && verticalScrollBarPolicy() != _policy) {
            setVerticalScrollBarPolicy(_policy);
        } else if (!_isVerticalScrollBar
                   && horizontalScrollBarPolicy() != _policy) {
            setHorizontalScrollBarPolicy(_policy);
        }

        if (needSync) {
            setupScrollingSynchronization(true);
        }
    };

    int vbarWidth = 0;
    const bool verticalScrollbar = true;
    const bool horizontalScrollbar = false;
    if (d->editor->verticalScrollBar()->isVisible()) {
        vbarWidth = d->editor->verticalScrollBar()->width();
        setScrollBarVisibility(verticalScrollbar, Qt::ScrollBarAlwaysOn);
    } else {
        setScrollBarVisibility(verticalScrollbar, Qt::ScrollBarAlwaysOff);
    }
    //
    int hbarHeight = 0;
    if (d->editor->horizontalScrollBar()->isVisible()) {
        hbarHeight = d->editor->horizontalScrollBar()->height();
        setScrollBarVisibility(horizontalScrollbar, Qt::ScrollBarAlwaysOn);
    } else {
        setScrollBarVisibility(horizontalScrollbar, Qt::ScrollBarAlwaysOff);
    }

    //
    // Размер редактора устанавливается таким образом, чтобы спрятать масштабированные полосы
    // прокрутки (скрывать их нельзя, т.к. тогда теряются значения, которые необходимо проксировать)
    //
    const int scrollBarsDiff = 2; // Считаем, что скролбар редактора на 2 пикселя шире скролбара обёртки
    const int editorWidth = width() / d->zoomRange + vbarWidth + d->zoomRange - scrollBarsDiff;
    const int editorHeight = height() / d->zoomRange + hbarHeight + d->zoomRange - scrollBarsDiff;
    const QSize editorSize(editorWidth, editorHeight);
    if (d->editorProxy->size() != editorSize) {
        d->editorProxy->resize(editorSize);
        if (QLocale().textDirection() == Qt::RightToLeft) {
            const QPointF delta(vbarWidth * d->zoomRange,  0);
            d->editorProxy->setPos(-delta);
        }
    }

    //
    // Необходимые действия для корректировки значений на полосах прокрутки
    //
    const bool dontSyncScrollPosition = false;
    syncScrollBarWithTextEdit(dontSyncScrollPosition);
}

void ScalableWrapper::scaleTextEdit()
{
    if (d->zoomRange == 0) {
        d->zoomRange = kDefaultZoomRange;
    } else if (d->zoomRange < kMinimumZoomRange) {
        d->zoomRange = kMinimumZoomRange;
    } else if (d->zoomRange > kMaximumZoomRange) {
        d->zoomRange = kMaximumZoomRange;
    }
    d->editorProxy->setScale(d->zoomRange);
}

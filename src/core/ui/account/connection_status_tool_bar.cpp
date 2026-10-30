#include "connection_status_tool_bar.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/avatar_generator/avatar_generator.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card_popup.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/logging.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHash>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSet>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QVariantAnimation>

#include <algorithm>

namespace Ui {

namespace {
constexpr qint64 kConfirmationTimeout = 30000;
}

class ConnectionStatusToolBar::Implementation
{
public:
    explicit Implementation(ConnectionStatusToolBar* _q);
    void refresh();
    void correctPosition();
    void showDetails();
    void positionDetails(bool _invalidate = true);
    void updateCollaborators();
    void expand(bool _expanded);
    void updateGeometry();
    void showProblem();
    void updateActivityColor();
    int slotCount() const;
    QRect statusRect() const;
    QRect avatarRect(int _index) const;
    int hitTest(const QPoint& _position) const;
    bool isOverdue(const QHash<QString, qint64>& _operations) const;
    void begin(QHash<QString, qint64>& _operations, const QString& _key);

    ConnectionStatusToolBar* q = nullptr;
    QAction* statusAction = nullptr;
    CardPopup* details = nullptr;
    H6Label* title = nullptr;
    Body2Label* connection = nullptr;
    Body2Label* outgoing = nullptr;
    Body2Label* incoming = nullptr;
    Body2Label* error = nullptr;
    Button* reconnect = nullptr;
    QVBoxLayout* layout = nullptr;
    H6Label* collaboratorsTitle = nullptr;
    QScrollArea* collaboratorsScroll = nullptr;
    QWidget* collaboratorsContent = nullptr;
    QVBoxLayout* collaboratorsLayout = nullptr;
    QVector<Domain::CursorInfo> collaborators;
    QLabel* notification = nullptr;
    QTimer notificationTimer;
    QTimer collapseTimer;
    QVariantAnimation expansion;
    qreal expansionProgress = 0;
    QColor stateColor;
    QString summary;
    QString problem;
    int pressedSlot = -2;

    bool connectionKnown = false;
    bool connected = false;
    bool cloudProjectOpen = false;
    QString syncError;
    QString uploadError;
    QHash<QString, qint64> pending;
    QSet<QString> uploading;
    QHash<QString, qint64> downloading;
    QSet<QUuid> subscribedDocuments;
    QDateTime lastUpload;
    QDateTime lastDownload;
    QElapsedTimer clock;
    QTimer refreshTimer;
    QTimer receivedTimer;
    QVariantAnimation activity;
};

ConnectionStatusToolBar::Implementation::Implementation(ConnectionStatusToolBar* _q)
    : q(_q)
    , statusAction(new QAction(q))
    , details(new CardPopup(q))
    , title(new H6Label(details))
    , connection(new Body2Label(details))
    , outgoing(new Body2Label(details))
    , incoming(new Body2Label(details))
    , error(new Body2Label(details))
    , reconnect(new Button(details))
    , layout(new QVBoxLayout)
    , collaboratorsTitle(new H6Label(details))
    , collaboratorsScroll(new QScrollArea(details))
    , collaboratorsContent(new QWidget)
    , collaboratorsLayout(new QVBoxLayout(collaboratorsContent))
    , notification(
          new QLabel(q, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint))
{
    clock.start();
    layout->setContentsMargins({ });
    collaboratorsTitle->setObjectName("collaboratorsTitle");
    collaboratorsScroll->setObjectName("collaboratorsList");
    collaboratorsScroll->setWidgetResizable(true);
    collaboratorsScroll->setFrameShape(QFrame::NoFrame);
    collaboratorsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    collaboratorsScroll->setVerticalScrollBar(new ScrollBar);
    collaboratorsScroll->setWidget(collaboratorsContent);
    collaboratorsLayout->setContentsMargins({ });
    collaboratorsLayout->setSpacing(0);
    layout->addWidget(collaboratorsTitle);
    layout->addWidget(collaboratorsScroll);
    for (auto label : QVector<AbstractLabel*>{ title, connection, outgoing, incoming, error }) {
        layout->addWidget(label);
    }
    layout->addWidget(reconnect);
    details->setContentLayout(layout);
    details->installEventFilter(q);
    details->setFocusPolicy(Qt::StrongFocus);
    reconnect->setFlat(true);
    QObject::connect(reconnect, &Button::clicked, q, [this] { emit q->checkConnectionPressed(); });

    refreshTimer.setInterval(1000);
    QObject::connect(&refreshTimer, &QTimer::timeout, q, [this] { refresh(); });
    receivedTimer.setSingleShot(true);
    receivedTimer.setInterval(2000);
    QObject::connect(&receivedTimer, &QTimer::timeout, q, [this] { refresh(); });
    activity.setObjectName("syncColorPulse");
    activity.setStartValue(0.0);
    activity.setKeyValueAt(0.5, 1.0);
    activity.setEndValue(0.0);
    activity.setEasingCurve(QEasingCurve::InOutSine);
    activity.setDuration(1600);
    activity.setLoopCount(-1);
    QObject::connect(&activity, &QVariantAnimation::valueChanged, q,
                     [this] { updateActivityColor(); });
    expansion.setDuration(180);
    expansion.setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(&expansion, &QVariantAnimation::valueChanged, q,
                     [this](const QVariant& value) {
                         expansionProgress = value.toReal();
                         updateGeometry();
                     });
    collapseTimer.setSingleShot(true);
    collapseTimer.setInterval(700);
    QObject::connect(&collapseTimer, &QTimer::timeout, q, [this] {
        if (!details->isVisible()) {
            expand(false);
        }
    });
    notification->setObjectName("syncNotification");
    notification->setAttribute(Qt::WA_ShowWithoutActivating);
    notification->setAttribute(Qt::WA_TransparentForMouseEvents);
    notification->setTextFormat(Qt::PlainText);
    notification->setWordWrap(true);
    notification->hide();
    notificationTimer.setSingleShot(true);
    notificationTimer.setInterval(5000);
    QObject::connect(&notificationTimer, &QTimer::timeout, notification, &QWidget::hide);
    QObject::connect(AvatarGenerator::instance(), &AvatarGenerator::avatarUpdated, q,
                     [this](const QString& email) {
                         for (const auto& collaborator : std::as_const(collaborators)) {
                             if (collaborator.email == email) {
                                 updateCollaborators();
                                 q->update();
                                 break;
                             }
                         }
                     });
}

void ConnectionStatusToolBar::Implementation::begin(QHash<QString, qint64>& _operations,
                                                    const QString& _key)
{
    if (!cloudProjectOpen) {
        return;
    }
    // Repeated edits must not postpone the warning about an unconfirmed request.
    if (!_operations.contains(_key)) {
        _operations.insert(_key, clock.elapsed());
    }
    refresh();
}

bool ConnectionStatusToolBar::Implementation::isOverdue(
    const QHash<QString, qint64>& _operations) const
{
    for (auto started : _operations) {
        if (clock.elapsed() - started >= kConfirmationTimeout) {
            return true;
        }
    }
    return false;
}

void ConnectionStatusToolBar::Implementation::refresh()
{
    const bool sendingDelayed = isOverdue(pending);
    const bool receivingDelayed = isOverdue(downloading);
    const bool hasError
        = !syncError.isEmpty() || !uploadError.isEmpty() || sendingDelayed || receivingDelayed;
    const bool busy = connected && !hasError
        && (!pending.isEmpty() || !downloading.isEmpty() || receivedTimer.isActive());

    // Material Design Icons: cloud-check / cloud-off-outline / cloud-alert /
    // cloud-upload / cloud-download / cloud-sync.
    QString icon = u8"\U000F0160";
    if (!connected) {
        icon = u8"\U000F0164";
    } else if (hasError) {
        icon = u8"\U000F09E0";
    } else if (!pending.isEmpty() && !downloading.isEmpty()) {
        icon = u8"\U000F063F";
    } else if (!pending.isEmpty()) {
        icon = u8"\U000F0167";
    } else if (!downloading.isEmpty() || receivedTimer.isActive()) {
        icon = u8"\U000F0162";
    }
    statusAction->setIconText(icon);
    q->setAccessibleName(ConnectionStatusToolBar::tr("Cloud synchronization"));
    title->setText(ConnectionStatusToolBar::tr("Cloud synchronization"));
    connection->setText(connected
                            ? ConnectionStatusToolBar::tr("Connection: connected")
                            : ConnectionStatusToolBar::tr(
                                  "Connection: disconnected. Automatic reconnection enabled."));

    QString uploadText;
    if (!pending.isEmpty()) {
        if (!connected) {
            uploadText = ConnectionStatusToolBar::tr("Your changes: waiting for connection");
        } else if (!uploadError.isEmpty()) {
            uploadText = ConnectionStatusToolBar::tr("Your changes: %1").arg(uploadError);
        } else if (sendingDelayed) {
            uploadText = ConnectionStatusToolBar::tr(
                "Your changes: server confirmation has not arrived yet");
        } else if (!uploading.isEmpty()) {
            uploadText = ConnectionStatusToolBar::tr("Your changes: saving to the cloud…");
        } else {
            uploadText = ConnectionStatusToolBar::tr("Your changes: waiting to be sent");
        }
    } else {
        uploadText = lastUpload.isValid()
            ? ConnectionStatusToolBar::tr("Your changes: saved to the cloud at %1")
                  .arg(QLocale().toString(lastUpload.toLocalTime().time(), QLocale::ShortFormat))
            : ConnectionStatusToolBar::tr("Your changes: no pending changes");
    }
    outgoing->setText(uploadText);
    QString downloadText;
    if (!connected) {
        downloadText = ConnectionStatusToolBar::tr("Incoming changes: waiting for connection");
    } else if (receivingDelayed) {
        downloadText
            = ConnectionStatusToolBar::tr("Incoming changes: the server has not responded yet");
    } else if (!downloading.isEmpty()) {
        downloadText = ConnectionStatusToolBar::tr("Incoming changes: receiving…");
    } else {
        downloadText = lastDownload.isValid()
            ? ConnectionStatusToolBar::tr("Incoming changes: received at %1")
                  .arg(QLocale().toString(lastDownload.toLocalTime().time(), QLocale::ShortFormat))
            : ConnectionStatusToolBar::tr("Incoming changes: none received in this session");
    }
    incoming->setText(downloadText);
    outgoing->setVisible(cloudProjectOpen);
    incoming->setVisible(cloudProjectOpen);
    error->setText(syncError);
    error->setVisible(cloudProjectOpen && !syncError.isEmpty());
    reconnect->setText(ConnectionStatusToolBar::tr("Reconnect"));
    reconnect->setVisible(!connected);
    q->setAccessibleDescription(connection->text()
                                + (cloudProjectOpen
                                       ? "\n" + uploadText + "\n" + downloadText + "\n" + syncError
                                       : QString()));

    summary = !connected ? ConnectionStatusToolBar::tr("No connection")
        : hasError       ? ConnectionStatusToolBar::tr("Synchronization needs attention")
        : busy           ? ConnectionStatusToolBar::tr("Synchronizing…")
                         : ConnectionStatusToolBar::tr("Synchronized");
    QString tooltip = summary;
    const auto latest = qMax(lastUpload, lastDownload);
    if (latest.isValid()) {
        tooltip += "\n"
            + ConnectionStatusToolBar::tr("Last synchronization: %1")
                  .arg(QLocale().toString(latest.toLocalTime().time(), QLocale::ShortFormat));
    }
    statusAction->setToolTip(tooltip);
    stateColor = !connected ? DesignSystem::color().warning()
        : hasError          ? DesignSystem::color().error()
                            : QColor("#2196f3");
    auto neutral = DesignSystem::color().onSurface();
    neutral.setAlphaF(0.65);
    q->setTextColor(neutral);
    q->setBackgroundColor(DesignSystem::color().surface());
    const bool visible = cloudProjectOpen || (connectionKnown && !connected);
    const bool pulse = visible && (busy || hasError || !connected);
    if (pulse && activity.state() != QAbstractAnimation::Running) {
        activity.start();
    } else if (!pulse) {
        activity.stop();
    }
    updateActivityColor();
    if (cloudProjectOpen && (!pending.isEmpty() || !downloading.isEmpty())) {
        if (!refreshTimer.isActive()) {
            refreshTimer.start();
        }
    } else {
        refreshTimer.stop();
    }
    q->setVisible(visible);
    if (q->isHidden()) {
        details->hide();
    }
    if (details->isVisible()) {
        positionDetails();
    }
    const QString nextProblem = !connected && connectionKnown ? QStringLiteral("offline")
        : connected && hasError ? syncError + uploadError + QString::number(sendingDelayed)
            + QString::number(receivingDelayed)
                                : QString();
    if (problem != nextProblem) {
        problem = nextProblem;
        notification->hide();
        if (!problem.isEmpty() && visible && q->isVisible() && !details->isVisible()) {
            showProblem();
        }
    }
    q->update();
}

int ConnectionStatusToolBar::Implementation::slotCount() const
{
    const int available
        = qMax(0,
               (q->parentWidget()->height()
                - int(DesignSystem::layout().px48() + DesignSystem::floatingToolBar().height()))
                   / int(DesignSystem::layout().px(36)));
    return qMin(available, qMin(int(collaborators.size()), 4));
}

QRect ConnectionStatusToolBar::Implementation::statusRect() const
{
    const auto& style = DesignSystem::floatingToolBar();
    const int size = style.height();
    return QRect(style.shadowMargins().left(), q->height() - style.shadowMargins().bottom() - size,
                 size, size);
}

QRect ConnectionStatusToolBar::Implementation::avatarRect(int _index) const
{
    const int step = DesignSystem::layout().px(36);
    const int size = DesignSystem::layout().px(28);
    return QRect((q->width() - size) / 2,
                 statusRect().top() - slotCount() * step + _index * step + (step - size) / 2, size,
                 size);
}

int ConnectionStatusToolBar::Implementation::hitTest(const QPoint& _position) const
{
    if (!q->rect().contains(_position)) {
        return -2;
    }
    if (statusRect().contains(_position)) {
        return -1;
    }
    for (int i = 0; i < slotCount(); ++i) {
        if (avatarRect(i).adjusted(-4, -4, 4, 4).contains(_position)) {
            return i;
        }
    }
    return -2;
}

void ConnectionStatusToolBar::Implementation::updateGeometry()
{
    q->setFixedSize(q->sizeHint());
    const auto& style = DesignSystem::floatingToolBar();
    const qreal horizontal = (style.height() - style.margins().left() - style.iconSize().width()
                              - style.margins().right())
        / 2.0;
    q->setActionsOffset(QPointF(q->isLeftToRight() ? horizontal : -horizontal,
                                statusRect().top() - style.shadowMargins().top()));
    correctPosition();
    q->update();
}

void ConnectionStatusToolBar::Implementation::expand(bool _expanded)
{
    collapseTimer.stop();
    const qreal target = _expanded && !collaborators.isEmpty() ? 1.0 : 0.0;
    if (expansion.endValue().toReal() == target
        && expansion.state() == QAbstractAnimation::Running) {
        return;
    }
    expansion.stop();
    expansion.setStartValue(expansionProgress);
    expansion.setEndValue(target);
    expansion.start();
}

void ConnectionStatusToolBar::Implementation::updateCollaborators()
{
    // Update existing rows in place: cursor broadcasts must not steal keyboard focus.
    while (collaboratorsLayout->count() > collaborators.size()) {
        auto item = collaboratorsLayout->takeAt(collaboratorsLayout->count() - 1);
        delete item->widget();
        delete item;
    }
    for (int i = 0; i < collaborators.size(); ++i) {
        QPushButton* row = nullptr;
        if (i < collaboratorsLayout->count()) {
            row = qobject_cast<QPushButton*>(collaboratorsLayout->itemAt(i)->widget());
        } else {
            row = new QPushButton(collaboratorsContent);
            row->setFlat(true);
            row->setCursor(Qt::PointingHandCursor);
            collaboratorsLayout->addWidget(row);
            QObject::connect(row, &QPushButton::clicked, q, [this, row] {
                const auto id = row->property("cursorId").toString();
                details->hide();
                emit q->collaboratorClicked(id);
            });
        }
        const auto& collaborator = collaborators[i];
        row->setProperty("cursorId", collaborator.cursorId);
        auto name = collaborator.name;
        name.replace('&', "&&"); // Names are literal text, not button mnemonics.
        row->setText(name + "\n" + ConnectionStatusToolBar::tr("Active in this project"));
        row->setToolTip(ConnectionStatusToolBar::tr("Go to collaborator’s position"));
        row->setIcon(QIcon(AvatarGenerator::avatar(collaborator.name, collaborator.email)));
        row->setIconSize(QSize(DesignSystem::layout().px(28), DesignSystem::layout().px(28)));
        row->setMinimumHeight(DesignSystem::layout().px(56));
        row->setFont(DesignSystem::font().body2());
    }
    collaboratorsTitle->setText(
        ConnectionStatusToolBar::tr("Collaborators (%1)").arg(collaborators.size()));
    collaboratorsTitle->setVisible(!collaborators.isEmpty());
    collaboratorsScroll->setVisible(!collaborators.isEmpty());
    collaboratorsScroll->setFixedHeight(qMin(int(collaborators.size()), 4)
                                        * int(DesignSystem::layout().px(56)));
    collaboratorsScroll->setStyleSheet(
        QString("QScrollArea, QWidget { background: %1; color: %2; } "
                "QPushButton { text-align: left; border: none; padding: 4px; border-radius: 4px; } "
                "QPushButton:hover, QPushButton:focus { background: %3; }")
            .arg(DesignSystem::color().background().name(),
                 DesignSystem::color().onBackground().name(),
                 DesignSystem::color().surface().name()));
    if (details->isVisible()) {
        positionDetails();
    }
}

void ConnectionStatusToolBar::Implementation::showProblem()
{
    if (!q->isVisible() || details->isVisible()) {
        return;
    }
    const QString text = summary + "\n"
        + (!connected                 ? ConnectionStatusToolBar::tr("Changes are saved locally")
               : !syncError.isEmpty() ? syncError
               : !uploadError.isEmpty()
               ? uploadError
               : ConnectionStatusToolBar::tr("Waiting for the server to confirm changes"));
    notification->setFont(DesignSystem::font().body2());
    notification->setStyleSheet(QString("QLabel { background: %1; color: %2; border: 1px solid %3; "
                                        "border-radius: 6px; padding: 10px; }")
                                    .arg(DesignSystem::color().background().name(),
                                         DesignSystem::color().onBackground().name(),
                                         stateColor.name()));
    notification->setText(text);
    const auto anchor = q->mapToGlobal(QPoint(0, 0));
    const auto anchorScreen = QApplication::screenAt(anchor);
    const auto screen = anchorScreen != nullptr ? anchorScreen : q->screen();
    if (!screen) {
        return;
    }
    const auto available = screen->availableGeometry();
    notification->setFixedWidth(qMin(int(DesignSystem::layout().px(300)), available.width()));
    notification->adjustSize();
    notification->move(qBound(available.left(), anchor.x() + q->width() - notification->width(),
                              available.right() - notification->width() + 1),
                       qMax(available.top(), anchor.y() - notification->height() - 8));
    notification->show();
    notificationTimer.start();
}

void ConnectionStatusToolBar::Implementation::correctPosition()
{
    q->move(q->isLeftToRight()
                ? q->parentWidget()->width() - q->width() - DesignSystem::layout().px24()
                    + DesignSystem::floatingToolBar().shadowMargins().right()
                : DesignSystem::layout().px24()
                    - DesignSystem::floatingToolBar().shadowMargins().left(),
            q->parentWidget()->height() - q->height() - DesignSystem::layout().px24()
                + DesignSystem::floatingToolBar().shadowMargins().bottom());
}

void ConnectionStatusToolBar::Implementation::showDetails()
{
    if (details->isVisible()) {
        details->hide();
        return;
    }
    QToolTip::hideText();
    notification->hide();
    expand(true);
    refresh();
    positionDetails();
    details->show();
    positionDetails();
    details->setFocus(Qt::PopupFocusReason);
}

void ConnectionStatusToolBar::Implementation::positionDetails(bool _invalidate)
{
    const auto anchor = q->mapToGlobal(statusRect().topLeft());
    const auto anchorScreen = QApplication::screenAt(anchor);
    const auto screen = anchorScreen != nullptr ? anchorScreen : q->screen();
    if (screen == nullptr) {
        return;
    }
    const auto available = screen->availableGeometry();
    const int width = qMin(static_cast<int>(DesignSystem::layout().px(360)), available.width());
    // Initialize the card's shadow margins before measuring its wrapped content.
    details->resize(width, details->height());
    if (_invalidate) {
        layout->invalidate();
        details->layout()->invalidate();
    }
    auto contentHeight = [&] {
        return qMax(details->layout()->totalHeightForWidth(width),
                    details->layout()->minimumSize().height());
    };
    if (_invalidate && !collaborators.isEmpty()) {
        // Reserve space for the status text; long errors and larger fonts must
        // not push the reconnect action out of the screen.
        const int preferred
            = qMin(int(collaborators.size()), 4) * int(DesignSystem::layout().px(56));
        collaboratorsScroll->setFixedHeight(preferred);
        layout->invalidate();
        details->layout()->invalidate();
        const int overflow = qMax(0, contentHeight() - available.height());
        if (overflow > 0) {
            collaboratorsScroll->setFixedHeight(
                qMax(int(DesignSystem::layout().px(28)), preferred - overflow));
            layout->invalidate();
            details->layout()->invalidate();
        }
    }
    const int height = qMin(available.height(), contentHeight());
    const int x = qBound(available.left(),
                         q->isLeftToRight() ? anchor.x() - width : anchor.x() + q->width(),
                         available.right() - width + 1);
    const int y = qBound(available.top(), anchor.y() + statusRect().height() - height,
                         available.bottom() - height + 1);
    details->setGeometry(x, y, width, height);
}

ConnectionStatusToolBar::ConnectionStatusToolBar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    Q_ASSERT(_parent);
    _parent->installEventFilter(this);
    setFlat(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    d->updateCollaborators();
    d->updateGeometry();
    addAction(d->statusAction);
    connect(d->statusAction, &QAction::triggered, this, [this] { d->showDetails(); });
    hide();
}

ConnectionStatusToolBar::~ConnectionStatusToolBar() = default;

void ConnectionStatusToolBar::setConnectionAvailable(bool _available)
{
    if (_available != d->connected) {
        // Subscriptions and their outstanding replies belong to one connection.
        // Login will request the current documents again on the new connection.
        d->subscribedDocuments.clear();
        d->downloading.clear();
    }
    if (_available && !d->connected) {
        // Offline time is not a server timeout. Keep the queue, but give the new
        // connection time to authenticate and retry the outstanding operations.
        const auto now = d->clock.elapsed();
        for (auto it = d->pending.begin(); it != d->pending.end(); ++it) {
            it.value() = now;
        }
        d->uploading.clear();
    }
    d->connectionKnown = true;
    d->connected = _available;
    d->refresh();
}

void ConnectionStatusToolBar::setCloudProjectOpen(bool _open)
{
    d->details->hide();
    d->notification->hide();
    d->collapseTimer.stop();
    d->expansion.stop();
    d->expansionProgress = 0;
    d->collaborators.clear();
    d->updateCollaborators();
    d->updateGeometry();
    d->problem.clear();
    d->cloudProjectOpen = _open;
    d->pending.clear();
    d->uploading.clear();
    d->downloading.clear();
    d->subscribedDocuments.clear();
    d->syncError.clear();
    d->uploadError.clear();
    d->lastUpload = { };
    d->lastDownload = { };
    d->receivedTimer.stop();
    d->refresh();
}

void ConnectionStatusToolBar::setSyncError(const QString& _error)
{
    d->syncError = _error;
    d->refresh();
}

void ConnectionStatusToolBar::setUploadError(const QString& _error)
{
    if (!d->cloudProjectOpen || d->pending.isEmpty()) {
        return;
    }
    d->uploadError = _error;
    d->refresh();
}

void ConnectionStatusToolBar::setDocumentPending(const QUuid& _uuid)
{
    d->begin(d->pending, _uuid.toString());
}

void ConnectionStatusToolBar::setUploadStarted(const QUuid& _uuid)
{
    if (!d->cloudProjectOpen) {
        return;
    }
    d->uploading.insert(_uuid.toString());
    d->begin(d->pending, _uuid.toString());
}

void ConnectionStatusToolBar::setUploadFinished(const QUuid& _uuid, bool _hasPendingChanges)
{
    const auto key = _uuid.toString();
    if (!d->cloudProjectOpen || !d->pending.contains(key)) {
        return;
    }
    d->lastUpload = QDateTime::currentDateTimeUtc();
    d->uploading.remove(key);
    if (_hasPendingChanges) {
        d->pending[key] = d->clock.elapsed();
    } else {
        d->pending.remove(key);
    }
    if (d->pending.isEmpty()) {
        d->uploadError.clear();
    }
    d->refresh();
}

void ConnectionStatusToolBar::removeDocument(const QUuid& _uuid)
{
    const auto key = _uuid.toString();
    d->pending.remove(key);
    d->uploading.remove(key);
    d->downloading.remove(key);
    d->subscribedDocuments.remove(_uuid);
    if (d->pending.isEmpty()) {
        d->uploadError.clear();
    }
    d->refresh();
}

void ConnectionStatusToolBar::setDocumentClosed(const QUuid& _uuid)
{
    d->subscribedDocuments.remove(_uuid);
    d->downloading.remove(_uuid.toString());
    d->refresh();
}

void ConnectionStatusToolBar::setDownloadStarted(const QUuid& _uuid)
{
    if (d->subscribedDocuments.contains(_uuid)) {
        // OpenDocuments can include an already opened bundle. The server deliberately
        // sends no reply for its existing subscriptions, so there is nothing to await.
        return;
    }
    d->begin(d->downloading, _uuid.toString());
}

void ConnectionStatusToolBar::setDownloadStarted(int _documentType)
{
    d->begin(d->downloading, QString::number(_documentType));
}

void ConnectionStatusToolBar::setDownloadFinished(const QUuid& _uuid, int _documentType,
                                                  bool _receivedChanges)
{
    if (!d->cloudProjectOpen) {
        return;
    }
    d->downloading.remove(_uuid.toString());
    d->downloading.remove(QString::number(_documentType));
    d->subscribedDocuments.insert(_uuid);
    Log::trace("[CloudSync] Document %1 received. Pending downloads: %2", _uuid.toString(),
               QStringList(d->downloading.keys()).join(", "));
    if (_receivedChanges) {
        d->lastDownload = QDateTime::currentDateTimeUtc();
        d->receivedTimer.start();
    }
    d->refresh();
}

bool ConnectionStatusToolBar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parentWidget()) {
        if (_event->type() == QEvent::Resize || _event->type() == QEvent::LayoutDirectionChange) {
            d->updateGeometry();
            d->details->hide();
            d->notification->hide();
        } else if (_event->type() == QEvent::ChildAdded) {
            raise();
        }
    } else if (_watched == d->details && _event->type() == QEvent::LayoutRequest
               && d->details->isVisible()) {
        d->positionDetails(false);
    } else if (_watched == d->details && _event->type() == QEvent::Hide) {
        d->collapseTimer.start();
    } else if (_watched == d->details && _event->type() == QEvent::KeyPress
               && static_cast<QKeyEvent*>(_event)->key() == Qt::Key_Escape) {
        d->details->hide();
        setFocus(Qt::PopupFocusReason);
        return true;
    }
    return FloatingToolBar::eventFilter(_watched, _event);
}

void ConnectionStatusToolBar::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter
        || _event->key() == Qt::Key_Space) {
        d->showDetails();
        _event->accept();
        return;
    }
    FloatingToolBar::keyPressEvent(_event);
}

bool ConnectionStatusToolBar::event(QEvent* _event)
{
    switch (_event->type()) {
    case QEvent::Enter:
        d->expand(true);
        break;
    case QEvent::Leave:
        d->collapseTimer.start();
        break;
    case QEvent::Show:
        QTimer::singleShot(0, this, [this] { d->refresh(); });
        break;
    case QEvent::Hide:
        d->activity.stop();
        d->notification->hide();
        d->details->hide();
        break;
    case QEvent::ToolTip: {
        const auto help = static_cast<QHelpEvent*>(_event);
        const int slot = d->hitTest(help->pos());
        if (slot == -1 && !d->details->isVisible()) {
            return FloatingToolBar::event(_event);
        }
        if (slot >= 0 && !d->details->isVisible()) {
            const bool overflow = slot == d->slotCount() - 1
                && (d->collaborators.size() > 3 || d->collaborators.size() > d->slotCount());
            const auto text = overflow
                ? tr("Show all collaborators")
                : d->collaborators[slot].name + "\n" + tr("Active in this project");
            // Use Qt's hover event and cursor position, as FloatingToolBar does.
            // Anchoring to the avatar's top-left can put the tooltip under the cursor.
            QToolTip::showText(help->globalPos(), text, this);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    default:
        break;
    }
    return FloatingToolBar::event(_event);
}

void ConnectionStatusToolBar::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton) {
        d->pressedSlot = d->hitTest(_event->pos());
        if (d->pressedSlot == -1) {
            FloatingToolBar::mousePressEvent(_event);
        }
        _event->accept();
    }
}

void ConnectionStatusToolBar::mouseReleaseEvent(QMouseEvent* _event)
{
    const int slot = d->hitTest(_event->pos());
    if (_event->button() == Qt::LeftButton && slot != -2 && slot == d->pressedSlot) {
        if (slot == -1) {
            FloatingToolBar::mouseReleaseEvent(_event);
        } else {
            d->showDetails();
        }
    }
    d->pressedSlot = -2;
}

void ConnectionStatusToolBar::paintEventPostprocess(QPainter& painter)
{
    painter.save();
    painter.setClipRect(QRect(0, 0, width(), d->statusRect().top()));
    painter.setOpacity(d->expansionProgress);
    for (int i = 0; i < d->slotCount(); ++i) {
        const auto avatarRect = d->avatarRect(i);
        if (i == d->slotCount() - 1
            && (d->collaborators.size() > 3 || d->collaborators.size() > d->slotCount())) {
            painter.setBrush(DesignSystem::color().background());
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(avatarRect);
            painter.setFont(DesignSystem::font().caption());
            painter.setPen(DesignSystem::color().onSurface());
            painter.drawText(avatarRect, Qt::AlignCenter,
                             QString("+%1").arg(d->collaborators.size() - i));
        } else {
            const auto& collaborator = d->collaborators[i];
            painter.setPen(
                QPen(ColorHelper::forText(collaborator.name), DesignSystem::layout().px2()));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(avatarRect);
            const auto avatar = AvatarGenerator::avatar(collaborator.name, collaborator.email);
            painter.drawPixmap(avatarRect, avatar);
        }
    }
    painter.restore();
    if (hasFocus()) {
        QPen focusPen(DesignSystem::color().onSurface());
        focusPen.setStyle(Qt::DotLine);
        painter.setPen(focusPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(d->statusRect().adjusted(2, 2, -2, -2));
    }
}

void ConnectionStatusToolBar::Implementation::updateActivityColor()
{
    auto color = q->textColor();
    if (activity.state() == QAbstractAnimation::Running) {
        const auto progress = activity.currentValue().toReal();
        color = QColor::fromRgbF(color.redF() + (stateColor.redF() - color.redF()) * progress,
                                 color.greenF() + (stateColor.greenF() - color.greenF()) * progress,
                                 color.blueF() + (stateColor.blueF() - color.blueF()) * progress,
                                 color.alphaF() + (1.0 - color.alphaF()) * progress);
    }
    q->setActionColor(statusAction, color);
}

QSize ConnectionStatusToolBar::sizeHint() const
{
    const auto& style = DesignSystem::floatingToolBar();
    return QSize(
        style.height() + style.shadowMargins().left() + style.shadowMargins().right(),
        style.height() + style.shadowMargins().top() + style.shadowMargins().bottom()
            + qRound(d->slotCount() * DesignSystem::layout().px(36) * d->expansionProgress));
}

void ConnectionStatusToolBar::setCollaborators(const QVector<Domain::CursorInfo>& _collaborators)
{
    if (!d->cloudProjectOpen) {
        return;
    }
    // Preserve the visual order even when the cursor map arrives in a different order.
    for (int i = d->collaborators.size() - 1; i >= 0; --i) {
        const auto found = std::find_if(_collaborators.begin(), _collaborators.end(),
                                        [this, i](const Domain::CursorInfo& cursor) {
                                            return cursor.cursorId == d->collaborators[i].cursorId;
                                        });
        if (found == _collaborators.end()) {
            d->collaborators.removeAt(i);
        } else {
            d->collaborators[i] = *found;
        }
    }
    for (const auto& cursor : _collaborators) {
        const bool exists = std::any_of(d->collaborators.begin(), d->collaborators.end(),
                                        [&cursor](const Domain::CursorInfo& existing) {
                                            return existing.cursorId == cursor.cursorId;
                                        });
        if (!exists) {
            d->collaborators.append(cursor);
        }
    }
    d->updateCollaborators();
    if (d->collaborators.isEmpty()) {
        d->expand(false);
    } else if (underMouse() || d->details->isVisible()) {
        d->expand(true);
    }
    d->updateGeometry();
}

void ConnectionStatusToolBar::updateTranslations()
{
    d->updateCollaborators();
    d->refresh();
}

void ConnectionStatusToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);
    d->details->setBackgroundColor(DesignSystem::color().background());
    d->layout->setSpacing(DesignSystem::layout().px12());
    const int padding = DesignSystem::layout().px16();
    d->layout->setContentsMargins(padding, padding, padding, padding);
    for (auto label : QVector<AbstractLabel*>{ d->title, d->connection, d->outgoing, d->incoming,
                                               d->collaboratorsTitle }) {
        label->setBackgroundColor(Qt::transparent);
        label->setTextColor(DesignSystem::color().onBackground());
    }
    d->error->setTextColor(DesignSystem::color().error());
    d->error->setBackgroundColor(Qt::transparent);
    d->reconnect->setBackgroundColor(DesignSystem::color().background());
    d->reconnect->setTextColor(DesignSystem::color().accent());
    d->updateCollaborators();
    d->updateGeometry();
    d->refresh();
    raise();
}

} // namespace Ui

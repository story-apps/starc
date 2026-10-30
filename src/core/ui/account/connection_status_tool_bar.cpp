#include "connection_status_tool_bar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card_popup.h>
#include <ui/widgets/label/label.h>
#include <utils/logging.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHash>
#include <QKeyEvent>
#include <QLocale>
#include <QPainter>
#include <QScreen>
#include <QSet>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariantAnimation>

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
    void positionDetails();
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
{
    clock.start();
    layout->setContentsMargins({ });
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
    activity.setStartValue(0);
    activity.setEndValue(360);
    activity.setDuration(1600);
    activity.setLoopCount(-1);
    QObject::connect(&activity, &QVariantAnimation::valueChanged, q,
                     qOverload<>(&ConnectionStatusToolBar::update));
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
    const bool busy = connected && !hasError && (!pending.isEmpty() || !downloading.isEmpty());

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
    // The visible control is icon-only. Details are available on click or via accessibility.
    statusAction->setToolTip(icon);
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

    q->setTextColor(hasError ? DesignSystem::color().error() : DesignSystem::color().onPrimary());
    q->setBackgroundColor(DesignSystem::color().primary());
    if (busy && activity.state() != QAbstractAnimation::Running) {
        activity.start();
    } else if (!busy) {
        activity.stop();
    }
    if (cloudProjectOpen && (!pending.isEmpty() || !downloading.isEmpty())) {
        if (!refreshTimer.isActive()) {
            refreshTimer.start();
        }
    } else {
        refreshTimer.stop();
    }
    q->setVisible(cloudProjectOpen || (connectionKnown && !connected));
    if (q->isHidden()) {
        details->hide();
    }
    if (details->isVisible()) {
        positionDetails();
    }
    q->update();
}

void ConnectionStatusToolBar::Implementation::correctPosition()
{
    q->move(q->isLeftToRight()
                ? q->parentWidget()->width() - q->width() - DesignSystem::layout().px24()
                : DesignSystem::layout().px24(),
            q->parentWidget()->height() - q->height() - DesignSystem::layout().px24());
}

void ConnectionStatusToolBar::Implementation::showDetails()
{
    if (details->isVisible()) {
        details->hide();
        return;
    }
    refresh();
    positionDetails();
    details->show();
    details->setFocus(Qt::PopupFocusReason);
}

void ConnectionStatusToolBar::Implementation::positionDetails()
{
    const auto anchor = q->mapToGlobal(q->rect().topLeft());
    const auto screen = QApplication::screenAt(anchor);
    if (screen == nullptr) {
        return;
    }
    const auto available = screen->availableGeometry();
    const int width = qMin(static_cast<int>(DesignSystem::layout().px(360)), available.width());
    // Initialize the card's shadow margins before measuring its wrapped content.
    details->resize(width, details->height());
    details->layout()->invalidate();
    const int height = qMin(available.height(),
                            qMax(details->layout()->totalHeightForWidth(width),
                                 details->layout()->minimumSize().height()));
    const int x = qBound(available.left(),
                         q->isLeftToRight() ? anchor.x() + q->width() - width : anchor.x(),
                         available.right() - width + 1);
    const int y = qBound(available.top(), anchor.y() - height, available.bottom() - height + 1);
    details->setGeometry(x, y, width, height);
}

ConnectionStatusToolBar::ConnectionStatusToolBar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    Q_ASSERT(_parent);
    _parent->installEventFilter(this);
    setFocusPolicy(Qt::StrongFocus);
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
            d->correctPosition();
            d->details->hide();
        } else if (_event->type() == QEvent::ChildAdded) {
            raise();
        }
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

void ConnectionStatusToolBar::paintEventPostprocess(QPainter& _painter)
{
    if (hasFocus()) {
        QPen focusPen(DesignSystem::color().onPrimary());
        focusPen.setStyle(Qt::DotLine);
        _painter.setPen(focusPen);
        _painter.setBrush(Qt::NoBrush);
        _painter.drawRoundedRect(
            QRectF(rect()).marginsRemoved(DesignSystem::floatingToolBar().shadowMargins()), 6, 6);
    }
    if (d->activity.state() != QAbstractAnimation::Running) {
        return;
    }
    QPen pen(DesignSystem::color().onPrimary());
    pen.setWidthF(DesignSystem::layout().px2());
    pen.setCapStyle(Qt::RoundCap);
    _painter.setPen(pen);
    const auto circle
        = QRectF(rect()).marginsRemoved(DesignSystem::floatingToolBar().shadowMargins()
                                        + DesignSystem::floatingToolBar().margins());
    _painter.drawArc(circle, d->activity.currentValue().toInt() * 16, 90 * 16);
}

void ConnectionStatusToolBar::updateTranslations()
{
    d->refresh();
}

void ConnectionStatusToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);
    d->details->setBackgroundColor(DesignSystem::color().background());
    d->layout->setSpacing(DesignSystem::layout().px12());
    const int padding = DesignSystem::layout().px16();
    d->layout->setContentsMargins(padding, padding, padding, padding);
    for (auto label :
         QVector<AbstractLabel*>{ d->title, d->connection, d->outgoing, d->incoming }) {
        label->setBackgroundColor(Qt::transparent);
        label->setTextColor(DesignSystem::color().onBackground());
    }
    d->error->setTextColor(DesignSystem::color().error());
    d->error->setBackgroundColor(Qt::transparent);
    d->reconnect->setBackgroundColor(DesignSystem::color().background());
    d->reconnect->setTextColor(DesignSystem::color().accent());
    resize(sizeHint());
    d->correctPosition();
    d->refresh();
    raise();
}

} // namespace Ui

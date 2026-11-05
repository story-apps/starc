#include <domain/starcloud_api.h>
#include <ui/account/connection_status_tool_bar.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card_popup.h>
#include <ui/widgets/label/label.h>

#include <QAction>
#include <QFontDatabase>
#include <QLabel>
#include <QPushButton>
#include <QRawFont>
#include <QScrollArea>
#include <QSignalSpy>
#include <QtTest>

class CloudSyncStatusTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        for (auto name : { "materialdesignicons", "roboto-regular", "roboto-medium" }) {
            QVERIFY(QFontDatabase::addApplicationFont(QString(":/fonts/%1").arg(name)) >= 0);
        }
        const auto icons = QRawFont::fromFont(Ui::DesignSystem::font().iconsMid());
        for (uint codepoint : { 0xF0160, 0xF0164, 0xF09E0, 0xF063F, 0xF0167, 0xF0162 }) {
            QVERIFY(icons.supportsCharacter(codepoint));
        }
    }

    void visibilityAndProjectSwitch()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        QVERIFY(status.isHidden());
        status.setConnectionAvailable(true);
        QVERIFY(status.isHidden());
        status.setCloudProjectOpen(true);
        QVERIFY(!status.isHidden());
        const auto document = QUuid::createUuid();
        status.setUploadStarted(document);
        status.setSyncError("Subscription expired");
        status.setCloudProjectOpen(false);
        QVERIFY(status.isHidden());
        status.setCloudProjectOpen(true);
        QVERIFY(status.accessibleDescription().contains("no pending changes"));
        QVERIFY(!status.accessibleDescription().contains("Subscription expired"));
        status.setUploadFinished(document, false); // A late confirmation from the previous project.
        QVERIFY(!status.accessibleDescription().contains("saved to the cloud at"));
        status.setCloudProjectOpen(false);
        status.setConnectionAvailable(false);
        QVERIFY(!status.isHidden()); // Preserve the global offline indicator for
                                     // local projects.
    }

    void confirmationsDoNotLoseNewerEdits()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setConnectionAvailable(true);
        status.setCloudProjectOpen(true);
        const auto first = QUuid::createUuid();
        const auto second = QUuid::createUuid();
        status.setUploadStarted(first);
        status.setUploadStarted(second);
        status.setUploadFinished(first, false);
        QVERIFY(status.accessibleDescription().contains("saving to the cloud"));
        QVERIFY(!status.accessibleDescription().contains("saved to the cloud at"));
        status.setUploadFinished(second,
                                 true); // More edits were made while the batch was in flight.
        QVERIFY(status.accessibleDescription().contains("waiting to be sent"));
        status.setUploadStarted(second);
        status.setUploadFinished(second, false);
        QVERIFY(status.accessibleDescription().contains("saved to the cloud at"));
    }

    void reconnectDoesNotConfirmPendingChanges()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        const auto document = QUuid::createUuid();
        status.setDocumentPending(document);
        QVERIFY(status.accessibleDescription().contains("waiting for connection"));
        status.setConnectionAvailable(true);
        QVERIFY(status.accessibleDescription().contains("waiting to be sent"));
        status.setUploadStarted(document);
        status.setConnectionAvailable(false);
        status.setConnectionAvailable(true);
        QVERIFY(!status.accessibleDescription().contains("saved to the cloud at"));
        status.setUploadFinished(document, false);
        QVERIFY(status.accessibleDescription().contains("saved to the cloud at"));
    }

    void downloadsAreIndependentOfUploads()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        const auto document = QUuid::createUuid();
        status.setUploadStarted(document);
        status.setDownloadStarted(1);
        status.setDownloadStarted(2);
        status.setDownloadFinished(QUuid::createUuid(), 1);
        QVERIFY(status.accessibleDescription().contains("receiving"));
        QVERIFY(status.accessibleDescription().contains("saving to the cloud"));
        status.setDownloadFinished(QUuid::createUuid(), 2);
        QVERIFY(status.accessibleDescription().contains("received at"));
        QVERIFY(status.accessibleDescription().contains("saving to the cloud"));
        status.setUploadFinished(document, false);
        QVERIFY(status.accessibleDescription().contains("saved to the cloud at"));
    }

    void repeatedBundleSubscriptionsDoNotWaitForSilentReplies()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        const auto information = QUuid::createUuid();
        const auto text = QUuid::createUuid();
        const auto statistics = QUuid::createUuid();
        const auto unrelated = QUuid::createUuid();
        const QVector<QUuid> bundle{ information, text, statistics };
        for (const auto& uuid : bundle) {
            status.setDownloadStarted(uuid);
        }
        status.setDownloadFinished(information, 10100);
        status.setDownloadFinished(text, 10104);

        // Applying the text opens its view and requests the same bundle again.
        // The server silently skips documents to which this session is subscribed.
        for (const auto& uuid : bundle) {
            status.setDownloadStarted(uuid);
        }
        status.setDownloadFinished(statistics, 10106);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: received at"));

        // A collaborator's patch must update the receipt status without leaving
        // the already received information/statistics documents waiting forever.
        status.setDownloadStarted(text);
        status.setDownloadFinished(text, 10104);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: received at"));

        // Successful traffic must not hide a genuinely outstanding document.
        status.setDownloadStarted(unrelated);
        status.setDownloadFinished(text, 10104);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: receiving"));
        status.setDownloadFinished(unrelated, 10104);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: received at"));
    }

    void subscriptionLifetimeMatchesConnectionAndProject()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        const auto document = QUuid::createUuid();
        const auto obsoleteRequest = QUuid::createUuid();
        status.setDownloadStarted(document);
        status.setDownloadFinished(document, 10104);
        status.setDownloadStarted(document);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: received at"));

        status.setDownloadStarted(obsoleteRequest);
        status.setDocumentPending(document);
        status.setConnectionAvailable(false);
        status.setConnectionAvailable(true);
        // Old requests are cancelled; login will request only the current documents.
        QVERIFY(!status.accessibleDescription().contains("Incoming changes: receiving"));
        QVERIFY(status.accessibleDescription().contains("Your changes: waiting to be sent"));
        status.setDownloadStarted(document);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: receiving"));
        status.setDownloadFinished(document, 10104);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: received at"));

        status.setCloudProjectOpen(false);
        status.setCloudProjectOpen(true);
        status.setDownloadStarted(document);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: receiving"));
    }

    void closingDocumentAllowsResubscriptionWithoutConfirmingUpload()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        const auto document = QUuid::createUuid();
        status.setDownloadStarted(document);
        status.setDownloadFinished(document, 10104);
        status.setDocumentPending(document);
        status.setDocumentClosed(document);
        QVERIFY(status.accessibleDescription().contains("Your changes: waiting to be sent"));
        status.setDownloadStarted(document);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: receiving"));
        status.setDocumentClosed(document); // Cancel a still outstanding subscription.
        QVERIFY(!status.accessibleDescription().contains("Incoming changes: receiving"));
        QVERIFY(status.accessibleDescription().contains("Your changes: waiting to be sent"));

        status.setDownloadStarted(document);
        status.setDownloadFinished(document, 0, false); // An acknowledgement without local data.
        status.setDownloadStarted(document);
        QVERIFY(!status.accessibleDescription().contains("Incoming changes: receiving"));
        status.removeDocument(document);
        status.setDownloadStarted(document);
        QVERIFY(status.accessibleDescription().contains("Incoming changes: receiving"));
    }

    void ownDocumentAcknowledgementIsNotIncomingChange()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        const auto document = QUuid::createUuid();
        status.setUploadStarted(document);
        status.setDownloadStarted(document);
        status.setDownloadFinished(document, 1, false);
        status.setUploadFinished(document, false);
        QVERIFY(status.accessibleDescription().contains("saved to the cloud at"));
        QVERIFY(status.accessibleDescription().contains("none received"));
    }

    void detailsOpenOnClickAndUpdateLive()
    {
        QWidget window;
        window.resize(900, 600);
        window.show();
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        QSignalSpy reconnect(&status, &Ui::ConnectionStatusToolBar::checkConnectionPressed);
        status.actions().constFirst()->trigger();
        auto popup = status.findChild<CardPopup*>();
        QVERIFY(popup != nullptr);
        QVERIFY(popup->isVisible());
        QCOMPARE(reconnect.count(), 0); // Clicking the icon only opens details.
        QCOMPARE(status.actions().constFirst()->toolTip(),
                 status.actions().constFirst()->iconText());
        status.setConnectionAvailable(false);
        auto button = popup->findChild<Button*>();
        QVERIFY(button != nullptr);
        QVERIFY(button->isVisible());
        QCoreApplication::processEvents();
        for (auto label : popup->findChildren<AbstractLabel*>()) {
            if (label->isVisible()) {
                QVERIFY2(label->height() >= label->heightForWidth(label->width()),
                         qPrintable(QString("%1: %2x%3, needs %4")
                                        .arg(label->text())
                                        .arg(label->width())
                                        .arg(label->height())
                                        .arg(label->heightForWidth(label->width()))));
            }
        }
        button->click();
        QCOMPARE(reconnect.count(), 1);
        QTest::keyClick(popup, Qt::Key_Escape);
        QVERIFY(!popup->isVisible());
        QTest::keyClick(&status, Qt::Key_Space);
        QVERIFY(popup->isVisible());
        status.setCloudProjectOpen(false);
        QVERIFY(!popup->isVisible());
    }

    void serverErrorClearsOnlyAfterAllConfirmations()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        const auto first = QUuid::createUuid();
        const auto second = QUuid::createUuid();
        status.setUploadStarted(first);
        status.setUploadStarted(second);
        status.setUploadError("Not enough cloud storage");
        QVERIFY(status.accessibleDescription().contains("Not enough cloud storage"));
        status.setUploadFinished(first, false);
        QVERIFY(status.accessibleDescription().contains("Not enough cloud storage"));
        status.setUploadFinished(second, false);
        QVERIFY(!status.accessibleDescription().contains("Not enough cloud storage"));
        QVERIFY(status.accessibleDescription().contains("saved to the cloud at"));
    }

    void collaboratorsExpandAndCollapseWithoutMovingTheIcon()
    {
        QWidget window;
        window.resize(900, 700);
        window.show();
        Ui::ConnectionStatusToolBar status(&window);
        status.setConnectionAvailable(true);
        status.setCloudProjectOpen(true);
        QVector<Domain::CursorInfo> collaborators;
        for (int i = 0; i < 8; ++i) {
            Domain::CursorInfo cursor;
            cursor.cursorId = QString::number(i);
            cursor.name = QString("Writer %1").arg(i);
            collaborators.append(cursor);
        }
        status.setCollaborators(collaborators);
        const int collapsed = status.height();
        const auto bottomRight = status.geometry().bottomRight();
        QEvent enter(QEvent::Enter);
        QApplication::sendEvent(&status, &enter);
        QTest::qWait(220);
        QVERIFY(status.height() > collapsed);
        QCOMPARE(status.geometry().bottomRight(), bottomRight);
        const int expanded = status.height();
        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(&status, &leave);
        QTest::qWait(350);
        QCOMPARE(status.height(), expanded);
        QApplication::sendEvent(&status, &enter); // Returning cancels the delayed collapse.
        QTest::qWait(500);
        QCOMPARE(status.height(), expanded);
        QApplication::sendEvent(&status, &leave);
        QTest::qWait(950);
        QCOMPARE(status.height(), collapsed);
        QCOMPARE(status.geometry().bottomRight(), bottomRight);
    }

    void combinedPopupPreservesOrderAndNavigation()
    {
        QWidget window;
        window.resize(900, 700);
        window.show();
        Ui::ConnectionStatusToolBar status(&window);
        status.setConnectionAvailable(true);
        status.setCloudProjectOpen(true);
        Domain::CursorInfo first, second;
        first.cursorId = "first";
        first.name = "First author";
        second.cursorId = "second";
        second.name = "Second author";
        status.setCollaborators({ first, second });
        QTest::mouseClick(&status, Qt::RightButton, Qt::NoModifier, status.rect().center());
        auto popup = status.findChild<CardPopup*>();
        QVERIFY(!popup->isVisible());
        QTest::mouseClick(&status, Qt::LeftButton, Qt::NoModifier, status.rect().center());
        QVERIFY(popup->isVisible());
        auto rows = popup->findChildren<QPushButton*>();
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows[0]->property("cursorId").toString(), first.cursorId);
        rows[0]->setFocus();
        status.setCollaborators({ second, first });
        QCOMPARE(popup->findChildren<QPushButton*>()[0], rows[0]);
        QCOMPARE(rows[0]->property("cursorId").toString(), first.cursorId);
        QSignalSpy follow(&status, &Ui::ConnectionStatusToolBar::collaboratorClicked);
        rows[1]->click();
        QCOMPARE(follow.count(), 1);
        QCOMPARE(follow.takeFirst().first().toString(), second.cursorId);
        QVERIFY(!popup->isVisible());
        status.setCloudProjectOpen(false);
        status.setCloudProjectOpen(true);
        status.actions().constFirst()->trigger();
        QCOMPARE(popup->findChildren<QPushButton*>().size(), 0);
    }

    void largeCollaboratorListScrollsAndAdaptsToSmallWindows()
    {
        QWidget window;
        window.resize(600, 700);
        window.show();
        Ui::ConnectionStatusToolBar status(&window);
        status.setConnectionAvailable(true);
        status.setCloudProjectOpen(true);
        QVector<Domain::CursorInfo> collaborators;
        for (int i = 0; i < 30; ++i) {
            Domain::CursorInfo cursor;
            cursor.cursorId = QString::number(i);
            cursor.name = QString("Writer %1").arg(i);
            collaborators.append(cursor);
        }
        status.setCollaborators(collaborators);
        status.actions().constFirst()->trigger();
        QTest::qWait(220);
        auto scroll = status.findChild<QScrollArea*>();
        QVERIFY(scroll->isVisible());
        QCOMPARE(scroll->findChildren<QPushButton*>().size(), 30);
        QVERIFY(scroll->widget()->height() > scroll->height());
        status.setSyncError(
            "Synchronization is temporarily unavailable. Please check your "
            "subscription and reconnect to continue collaborating on this project.");
        status.setConnectionAvailable(false);
        QTest::qWait(50);
        auto popup = status.findChild<CardPopup*>();
        for (auto label : popup->findChildren<AbstractLabel*>()) {
            if (label->isVisible()) {
                QVERIFY(label->height() >= label->heightForWidth(label->width()));
                QVERIFY(popup->rect().contains(label->geometry()));
            }
        }
        auto reconnect = popup->findChild<Button*>();
        QVERIFY(reconnect->isVisible());
        QVERIFY(popup->rect().contains(reconnect->geometry()));
        window.resize(600, 160);
        QCoreApplication::processEvents();
        QVERIFY(status.y() >= 0);
        QVERIFY(status.geometry().bottom() < window.height());
        window.setLayoutDirection(Qt::RightToLeft);
        QCoreApplication::processEvents();
        QVERIFY(status.x() < window.width() / 2);
    }

    void iconPulsesAndProblemNotificationDoesNotRepeat()
    {
        QWidget window;
        window.resize(900, 700);
        window.show();
        Ui::ConnectionStatusToolBar status(&window);
        status.setConnectionAvailable(true);
        status.setCloudProjectOpen(true);
        status.clearFocus();
        QTest::qWait(50); // Apply the queued design-system event before comparing frames.
        const auto idle = status.grab().toImage();
        QTest::qWait(400);
        QCOMPARE(status.grab().toImage(), idle);
        status.setConnectionAvailable(false);
        auto notification = status.findChild<QLabel*>("syncNotification");
        QVERIFY(notification->isVisible());
        QVERIFY(notification->text().contains("saved locally"));
        const auto neutral = status.grab().toImage();
        QTest::qWait(800);
        QVERIFY(status.grab().toImage() != neutral);
        notification->hide();
        status.setConnectionAvailable(false);
        QVERIFY(!notification->isVisible());
        status.setConnectionAvailable(true);
        QTest::qWait(50);
        QCOMPARE(status.grab().toImage(), idle);
        status.setUploadStarted(QUuid::createUuid());
        status.setUploadError("Rejected");
        QVERIFY(notification->isVisible());
        QVERIFY(notification->text().contains("Rejected"));
        status.setCloudProjectOpen(false);
        QVERIFY(!notification->isVisible());
    }

    void unconfirmedOperationsStayVisible()
    {
        QWidget window;
        Ui::ConnectionStatusToolBar status(&window);
        status.setCloudProjectOpen(true);
        status.setConnectionAvailable(true);
        const auto first = QUuid::createUuid();
        const auto second = QUuid::createUuid();
        status.setUploadStarted(first);
        status.setUploadStarted(second);
        status.setDownloadStarted(1);
        QTest::qWait(31000);
        QVERIFY(status.accessibleDescription().contains("confirmation has not arrived"));
        QVERIFY(status.accessibleDescription().contains("has not responded"));
        status.setUploadFinished(first, false);
        QVERIFY(status.accessibleDescription().contains("confirmation has not arrived"));
        status.setConnectionAvailable(false);
        status.setConnectionAvailable(true);
        QVERIFY(!status.accessibleDescription().contains("confirmation has not arrived"));
        QVERIFY(!status.accessibleDescription().contains("has not responded"));
        QVERIFY(!status.accessibleDescription().contains("saved to the cloud at"));
        status.setUploadStarted(second);
        status.setUploadFinished(second, false);
        status.setDownloadFinished(QUuid::createUuid(), 1);
        QVERIFY(status.accessibleDescription().contains("saved to the cloud at"));
        QVERIFY(!status.accessibleDescription().contains("has not responded"));
    }
};

QTEST_MAIN(CloudSyncStatusTest)
#include "tst_cloud_sync_status.moc"

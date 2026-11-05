#include <ui/account/connection_status_tool_bar.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card_popup.h>
#include <ui/widgets/label/label.h>

#include <QAction>
#include <QFontDatabase>
#include <QRawFont>
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
                QVERIFY(label->height() >= label->heightForWidth(label->width()));
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

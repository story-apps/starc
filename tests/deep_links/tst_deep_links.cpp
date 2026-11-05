#include <include/deep_link.h>
#include <app/application.h>
#include <interfaces/management_layer/i_application_manager.h>

#include <QFileOpenEvent>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <QStandardPaths>
#include <QtTest>

class FakeManager : public QObject, public ManagementLayer::IApplicationManager
{
    Q_OBJECT
    Q_INTERFACES(ManagementLayer::IApplicationManager)
public:
    QString last;
    QString logFilePath() const override { return {}; }
    void exec(const QString& value) override { last = value; }
    bool openProject(const QString& value) override { last = value; return true; }
};

class DeepLinkTest : public QObject
{
    Q_OBJECT
    FakeManager* receiver = nullptr;
private slots:
    void parse_data()
    {
        QTest::addColumn<QString>("url");
        QTest::addColumn<bool>("valid");
        QTest::newRow("app") << "starc://open" << true;
        QTest::newRow("team") << "starc://open?teamId=12" << true;
        QTest::newRow("personal-project") << "starc://open?projectId=34" << true;
        QTest::newRow("web") << "https://starc.app/open/?teamId=12&projectId=34&documentGuid=7a46aa60-9fc3-4d43-a883-b3131f25a4d3" << true;
        QTest::newRow("duplicate") << "starc://open?projectId=1&projectId=2" << false;
        QTest::newRow("negative") << "starc://open?teamId=-1" << false;
        QTest::newRow("zero") << "starc://open?projectId=0" << false;
        QTest::newRow("overflow") << "starc://open?projectId=2147483648" << false;
        QTest::newRow("empty") << "starc://open?projectId=" << false;
        QTest::newRow("leading-zero") << "starc://open?projectId=01" << false;
        QTest::newRow("orphan-document") << "starc://open?documentGuid=7a46aa60-9fc3-4d43-a883-b3131f25a4d3" << false;
        QTest::newRow("bad-document") << "starc://open?projectId=1&documentGuid=invalid" << false;
        QTest::newRow("null-document") << "starc://open?projectId=1&documentGuid=00000000-0000-0000-0000-000000000000" << false;
        QTest::newRow("foreign-host") << "https://evil.example/open?projectId=1" << false;
        QTest::newRow("credentials") << "starc://user@open?projectId=1" << false;
        QTest::newRow("port") << "starc://open:123?projectId=1" << false;
        QTest::newRow("fragment") << "starc://open?projectId=1#x" << false;
        QTest::newRow("unknown-parameter") << "starc://open?path=/tmp/test.starc" << false;
        QTest::newRow("http") << "http://starc.app/open?projectId=1" << false;
        QTest::newRow("trailing-newline") << "starc://open?projectId=1%0A" << false;
        QTest::newRow("bad-escape") << "starc://open?projectId=%ZZ" << false;
        QTest::newRow("oversized") << ("starc://open?projectId=" + QString(5000, '1')) << false;
        QTest::newRow("local-file") << "/tmp/my story.starc" << false;
    }
    void parse()
    {
        QFETCH(QString, url);
        QFETCH(bool, valid);
        QCOMPARE(DeepLink::parse(url).has_value(), valid);
    }
    void values()
    {
        auto link = DeepLink::parse("starc://open?teamId=12&projectId=34&documentGuid=7A46AA60-9FC3-4D43-A883-B3131F25A4D3");
        QVERIFY(link);
        QCOMPARE(link->teamId, 12);
        QCOMPARE(link->projectId, 34);
        QCOMPARE(link->documentGuid, QUuid("7a46aa60-9fc3-4d43-a883-b3131f25a4d3"));
    }
    void startupAndOsEvents()
    {
        auto app = static_cast<Application*>(qApp);
        receiver = new FakeManager;
        auto manager = receiver;
        app->setApplicationManager(manager);
        const QUrl url("starc://open?projectId=34");
        QFileOpenEvent early(url);
        QCoreApplication::sendEvent(app, &early);
        app->startUp();
        QCOMPARE(manager->last, url.toString());
        QFileOpenEvent running(QUrl("starc://open?teamId=12"));
        QCoreApplication::sendEvent(app, &running);
        QCOMPARE(manager->last, QString("starc://open?teamId=12"));
        QFileOpenEvent file(QUrl::fromLocalFile("/tmp/a story.starc"));
        QCoreApplication::sendEvent(app, &file);
        QCOMPARE(manager->last, QString("/tmp/a story.starc"));
    }
    void runningInstance()
    {
        auto app = static_cast<Application*>(qApp);
        QVERIFY(!app->forwardDeepLink());
        QProcess child;
        child.start(QCoreApplication::applicationFilePath(), {"starc://open?projectId=75"});
        QVERIFY(child.waitForStarted());
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 6000);
        QVERIFY2(child.exitCode() == 0, child.readAllStandardError().constData());
        QCOMPARE(receiver->last, QString("starc://open?projectId=75"));
        child.start(QCoreApplication::applicationFilePath(), {"starc://open?projectId=76"});
        QVERIFY(child.waitForStarted());
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 6000);
        QVERIFY2(child.exitCode() == 0, child.readAllStandardError().constData());
        QCOMPARE(receiver->last, QString("starc://open?projectId=76"));
    }
};

int main(int argc, char** argv)
{
    // Child processes inherit a private data directory, so tests cannot contact a real app.
    QTemporaryDir data;
    if (qEnvironmentVariableIsEmpty("STARC_LINK_TEST_CHILD")) {
        qputenv("XDG_DATA_HOME", data.path().toUtf8());
        qputenv("STARC_LINK_TEST_CHILD", "1");
        qputenv("STARC_LINK_TEST_ID", QUuid::createUuid().toByteArray(QUuid::WithoutBraces));
    }
    qunsetenv("APPIMAGE");
    Application app(argc, argv);
    QCoreApplication::setOrganizationName("Story Architect Tests");
    QCoreApplication::setApplicationName("DeepLinks-" + qEnvironmentVariable("STARC_LINK_TEST_ID"));
    if (argc == 2 && DeepLink::isLink(QString::fromLocal8Bit(argv[1]))) {
        return app.forwardDeepLink() ? 0 : 2;
    }
    DeepLinkTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_deep_links.moc"

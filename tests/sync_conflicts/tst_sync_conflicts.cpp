#include <business_layer/model/abstract_model.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>

#include <QtTest>

#include <memory>

// Exercise AbstractModel's actual merge/reset/save machinery without editor dependencies.
class XmlModel : public BusinessLayer::AbstractModel
{
public:
    XmlModel() : AbstractModel({ "doc", "p" }) { }
    QByteArray xml;
    void initDocument() override { xml = document()->content(); }
    void clearDocument() override { xml.clear(); }
    QByteArray toXml() const override { return xml; }
};

class SyncConflictsTest : public QObject
{
    Q_OBJECT
private slots:
    void merge_data()
    {
        QTest::addColumn<QByteArray>("base");
        QTest::addColumn<QByteArray>("local");
        QTest::addColumn<QByteArray>("remote");
        QTest::addColumn<QByteArray>("expected");
        auto row = [](const char* name, const char* base, const char* local,
                      const char* remote, const char* expected) {
            QTest::newRow(name) << QByteArray(base) << QByteArray(local)
                                << QByteArray(remote) << QByteArray(expected);
        };
        row("independent-paragraphs", "<doc><p>one</p><p>two</p></doc>",
            "<doc><p>ONE</p><p>two</p></doc>", "<doc><p>one</p><p>TWO</p></doc>",
            "<doc><p>ONE</p><p>TWO</p></doc>");
        row("same-paragraph", "<doc><p>Hello Alice, welcome home.</p></doc>",
            "<doc><p>Hi Alice, welcome home.</p></doc>",
            "<doc><p>Hello Alice, welcome back.</p></doc>",
            "<doc><p>Hi Alice, welcome back.</p></doc>");
        row("identical", "<doc>old</doc>", "<doc>new</doc>", "<doc>new</doc>", "<doc>new</doc>");
        row("local-only", "<doc>old</doc>", "<doc>new</doc>", "<doc>old</doc>", "<doc>new</doc>");
        row("remote-only", "<doc>old</doc>", "<doc>old</doc>", "<doc>new</doc>", "<doc>new</doc>");
        row("overlapping-replacement", "<doc>Hello Alice</doc>", "<doc>Hello Bob</doc>",
            "<doc>Hello Carol</doc>", nullptr);
        row("same-insertion-point", "<doc>Hello</doc>", "<doc>Hello Alice</doc>",
            "<doc>Hello Bob</doc>", nullptr);
        row("delete-versus-edit", "<doc><p>one</p><p>two</p></doc>", "<doc><p>one</p></doc>",
            "<doc><p>one</p><p>TWO</p></doc>", nullptr);
        row("invalid-local", "<doc>old</doc>", "<doc>bad", "<doc>old</doc>", nullptr);
        row("invalid-remote", "<doc>old</doc>", "<doc>old</doc>", "<doc>bad", nullptr);
        row("invalid-base", "bad", "<doc>local</doc>", "<doc>remote</doc>", nullptr);
        row("root-type", "<doc>old</doc>", "<other>old</other>", "<doc>new</doc>", nullptr);
        row("unicode", u8"<doc><p>Привет 🌍</p><p>東京</p></doc>",
            u8"<doc><p>Привет 🌞</p><p>東京</p></doc>",
            u8"<doc><p>Привет 🌍</p><p>京都</p></doc>",
            u8"<doc><p>Привет 🌞</p><p>京都</p></doc>");
        row("delete-and-edit-independent", "<doc><p>first</p><p>second</p><p>third</p></doc>",
            "<doc><p>first</p><p>third</p></doc>",
            "<doc><p>FIRST</p><p>second</p><p>third</p></doc>",
            "<doc><p>FIRST</p><p>third</p></doc>");
    }

    void merge()
    {
        QFETCH(QByteArray, base);
        QFETCH(QByteArray, local);
        QFETCH(QByteArray, remote);
        QFETCH(QByteArray, expected);
        DiffMatchPatchController controller({ "doc", "p" });
        QByteArray result("untouched");
        QCOMPARE(controller.mergeXml(base, local, remote, result), !expected.isNull());
        QCOMPARE(result, expected.isNull() ? QByteArray("untouched") : expected);
        // The merge must not favor one participant.
        result = "untouched";
        QCOMPARE(controller.mergeXml(base, remote, local, result), !expected.isNull());
        QCOMPARE(result, expected.isNull() ? QByteArray("untouched") : expected);
    }

    void exactPatchIsAtomic()
    {
        DiffMatchPatchController controller({ "doc", "p" });
        const QByteArray base = "<doc><p>first</p><p>" + QByteArray(100, 'x') + "</p><p>last</p></doc>";
        auto changed = base;
        changed.replace("first", "FIRST").replace("last", "LAST");
        const auto patch = controller.makePatch(base, changed);
        QByteArray result;
        QVERIFY(controller.applyPatchExact(base, patch, result));
        QCOMPARE(result, changed);
        QVERIFY(controller.applyPatchExact(changed, controller.makePatch(changed, base), result));
        QCOMPARE(result, base);
        auto divergent = base;
        divergent.replace("last", "remote");
        result = "untouched";
        QVERIFY(!controller.applyPatchExact(divergent, patch, result));
        QCOMPARE(result, QByteArray("untouched"));
        QVERIFY(!controller.applyPatchExact(base, "not a patch", result));
        QCOMPARE(result, QByteArray("untouched"));
    }

    void offlineHistoryAndRebasedPatch()
    {
        DiffMatchPatchController controller({ "doc", "p" });
        const QByteArray base = "<doc><p>one</p><p>two</p></doc>";
        const QByteArray local1 = "<doc><p>ONE</p><p>two</p></doc>";
        const QByteArray local2 = "<doc><p>ONE!</p><p>two</p></doc>";
        const QByteArray remote = "<doc><p>one</p><p>TWO</p></doc>";
        const QByteArray merged = "<doc><p>ONE!</p><p>TWO</p></doc>";
        std::unique_ptr<Domain::DocumentObject> document(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::SimpleText, remote, {}));
        XmlModel model;
        model.setDocument(document.get());
        QSignalSpy changed(&model, &BusinessLayer::AbstractModel::contentsChanged);
        QVERIFY(model.mergeOfflineChanges(local2,
            { controller.makePatch(local1, base), controller.makePatch(local2, local1) }));
        QCOMPARE(document->content(), merged);
        const auto rebased = model.adoptDocumentChanges(remote);
        QByteArray result;
        QVERIFY(controller.applyPatchExact(remote, rebased.second, result));
        QCOMPARE(result, merged);
        QVERIFY(controller.applyPatchExact(merged, rebased.first, result));
        QCOMPARE(result, remote);
        model.saveChanges();
        QCOMPARE(changed.count(), 0);
        // A retry with identical online content is idempotent.
        QVERIFY(model.mergeOfflineChanges(merged, { rebased.first }));
        QCOMPARE(document->content(), merged);
    }

    void failedModelMergeKeepsOnlineVersion()
    {
        DiffMatchPatchController controller({ "doc", "p" });
        const QByteArray base = "<doc>Hello Alice</doc>";
        const QByteArray local = "<doc>Hello Bob</doc>";
        const QByteArray remote = "<doc>Hello Carol</doc>";
        std::unique_ptr<Domain::DocumentObject> document(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::SimpleText, remote, {}));
        XmlModel model;
        model.setDocument(document.get());
        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        QVERIFY(!model.mergeOfflineChanges(local, { controller.makePatch(local, base) }));
        QCOMPARE(document->content(), remote);
        QCOMPARE(model.xml, remote);
        QCOMPARE(reset.count(), 0);
        QVERIFY(!model.mergeOfflineChanges(local, { QByteArray("broken history") }));
        QVERIFY(!model.mergeDocumentChanges(base, { QByteArray("broken online patch") }));
        QVERIFY(!model.mergeDocumentChanges("<doc>broken", {}));
        QCOMPARE(document->content(), remote);
        QCOMPARE(model.xml, remote);
        QCOMPARE(reset.count(), 0);
    }

    void mergedSnapshotClearsObsoleteRedo()
    {
        DiffMatchPatchController controller({ "doc", "p" });
        const QByteArray before = "<doc>old</doc>";
        const QByteArray local = "<doc>local</doc>";
        const QByteArray online = "<doc>online</doc>";
        std::unique_ptr<Domain::DocumentObject> document(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::SimpleText, local, {}));
        XmlModel model;
        model.setDocument(document.get());
        model.undoChange(controller.makePatch(local, before), controller.makePatch(before, local));
        QCOMPARE(document->content(), before);
        QVERIFY(model.mergeDocumentChanges(online, {}));
        model.redo();
        QCOMPARE(document->content(), online);
        QSignalSpy undo(&model, &BusinessLayer::AbstractModel::undoRequested);
        model.undo();
        QCOMPARE(undo.first().first().toInt(), 0);
    }
};

QTEST_MAIN(SyncConflictsTest)
#include "tst_sync_conflicts.moc"

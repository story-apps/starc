#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/database.h>
#include <data_layer/storage/document_change_storage.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <domain/starcloud_api.h>
#include <management_layer/content/project/project_manager.h>
#include <management_layer/content/projects/projects_model_project_item.h>
#include <management_layer/plugins_builder.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <ui/widgets/dialog/dialog.h>

#include <QDirIterator>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

class TestTextModel : public BusinessLayer::SimpleTextModel
{
public:
    QByteArray makePatch(const QByteArray& before, const QByteArray& after) const
    {
        return dmpController().makePatch(QString::fromUtf8(before), QString::fromUtf8(after));
    }
};

class ProjectSyncConflictsTest : public QObject
{
    Q_OBJECT
private slots:
    void cleanup()
    {
        DataStorageLayer::StorageFacade::clearStorages();
        DatabaseLayer::Database::closeCurrentFile();
        DatabaseLayer::Database::clearLastError();
    }

    void singleEditConflict_data()
    {
        QTest::addColumn<bool>("editDraft");
        QTest::addColumn<bool>("blockRecovery");
        QTest::addColumn<bool>("disjoint");
        QTest::newRow("document-with-existing-draft") << false << false << false;
        QTest::newRow("conflict-in-existing-draft") << true << false << false;
        QTest::newRow("recovery-save-failure") << false << true << false;
        QTest::newRow("automatic-merge") << false << false << true;
    }

    void singleEditConflict()
    {
        QFETCH(bool, editDraft);
        QFETCH(bool, blockRecovery);
        QFETCH(bool, disjoint);
        using namespace BusinessLayer;
        using namespace DataStorageLayer;
        using namespace Domain;
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("test.starc");
        DatabaseLayer::Database::setCurrentFile(path);
        QUuid recoveryUuid;
        {
            QWidget window;
            ManagementLayer::PluginsBuilder plugins;
            ManagementLayer::ProjectManager manager(nullptr, &window, plugins);
            StructureModel* structure = nullptr;
            connect(&manager, &ManagementLayer::ProjectManager::structureModelChanged,
                    [&](AbstractModel* model) { structure = qobject_cast<StructureModel*>(model); });
            ProjectsModelProjectItem project;
            project.setPath(path);
            project.setName("Conflict regression");
            project.setProjectType(ProjectType::Local);
            manager.loadCurrentProject(&project);
            QVERIFY(structure);

            std::unique_ptr<DocumentObject> seed(ObjectsBuilder::createDocument(
                {}, QUuid::createUuid(), DocumentObjectType::SimpleText, {}, {}));
            TestTextModel model;
            model.setDocument(seed.get());
            auto paragraph = model.createTextItem();
            paragraph->setParagraphType(TextParagraphType::Text);
            paragraph->setText("Hello Alice");
            model.appendItem(paragraph);
            auto second = model.createTextItem();
            second->setParagraphType(TextParagraphType::Text);
            second->setText("Second paragraph");
            model.appendItem(second);
            model.reassignContent();
            const auto base = seed->content();
            QVERIFY(base.contains("Hello Alice"));
            auto local = base;
            local.replace("Hello Alice", "Hello Bob");
            auto remote = base;
            if (disjoint) {
                remote.replace("Second paragraph", "Online paragraph");
            } else {
                remote.replace("Hello Alice", "Hello Carol");
            }

            manager.storeSimpleText("Unrelated document", QString::fromUtf8(base));
            auto unrelated = structure->itemForIndex(structure->index(structure->rowCount() - 1, 0));
            auto item = new StructureModelItem(QUuid::createUuid(), DocumentObjectType::SimpleText,
                                               "Affected document", "Current draft", {}, true,
                                               false, false);
            structure->appendItem(item, structure->itemForIndex({}), base);
            structure->addItemDraft(item, "Existing draft", {}, false, base, false);
            auto existingDraft = item->drafts().first();
            const auto targetUuid = editDraft ? existingDraft->uuid() : item->uuid();
            // Leave the earlier, unrelated document selected while the target synchronizes.
            manager.saveChanges();
            manager.clearChangesHistory();

            DocumentInfo info;
            info.uuid = targetUuid;
            info.type = static_cast<int>(DocumentObjectType::SimpleText);
            info.content = local;
            manager.mergeDocumentInfo(info);
            manager.clearChangesHistory();
            const auto savedLocal = StorageFacade::documentStorage()->document(targetUuid)->content();
            // Use the model's canonical XML as the offline history's endpoint.
            auto savedBase = savedLocal;
            savedBase.replace("Hello Bob", "Hello Alice");
            StorageFacade::documentChangeStorage()->appendDocumentChange(
                targetUuid, QUuid::createUuid(), model.makePatch(savedLocal, savedBase),
                model.makePatch(savedBase, savedLocal), "Test Author", "test@example.com");
            if (blockRecovery) {
                QFile blocker(directory.filePath("conflicts"));
                QVERIFY(blocker.open(QIODevice::WriteOnly));
            }
            int uploadedHistorySize = 0;
            connect(&manager, &ManagementLayer::ProjectManager::contentsChanged,
                    [&](AbstractModel* changed) {
                        if (changed->document()->uuid() == targetUuid) {
                            uploadedHistorySize = StorageFacade::documentChangeStorage()
                                ->unsyncedDocumentChanges(targetUuid).size();
                        }
                    });
            info.content = remote;
            manager.mergeDocumentInfo(info);
            if (blockRecovery || disjoint) {
                QCOMPARE(item->drafts().size(), 1);
                QCOMPARE(StorageFacade::documentChangeStorage()
                             ->unsyncedDocumentChanges(targetUuid).size(), 1);
                const auto current = StorageFacade::documentStorage()->document(targetUuid)->content();
                QVERIFY(current.contains("Hello Bob"));
                if (disjoint) {
                    QVERIFY(current.contains("Online paragraph"));
                    QCOMPARE(uploadedHistorySize, 1);
                } else {
                    QCOMPARE(current, savedLocal);
                    QCOMPARE(uploadedHistorySize, 0);
                }
            } else {
                QCOMPARE(item->drafts().size(), 2);
                QCOMPARE(item->drafts().last(), existingDraft);
                QCOMPARE(existingDraft->drafts().size(), 0);
                QVERIFY(unrelated->drafts().isEmpty());
                auto recovery = StorageFacade::documentStorage()->document(item->drafts().first()->uuid());
                QVERIFY(recovery);
                recoveryUuid = recovery->uuid();
                QVERIFY(recovery->content().contains("Hello Bob"));
                QVERIFY(StorageFacade::documentStorage()->document(targetUuid)->content().contains("Hello Carol"));
                QVERIFY(StorageFacade::documentChangeStorage()->unsyncedDocumentChanges(targetUuid).isEmpty());
                QStringList copies;
                QDirIterator it(directory.path(), { "offline.xml", "online.xml" }, QDir::Files,
                                QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    copies.append(it.next());
                }
                QCOMPARE(copies.size(), 2);
                for (const auto& copy : copies) {
                    QFile file(copy);
                    QVERIFY(file.open(QIODevice::ReadOnly));
                    QVERIFY(file.readAll().contains(copy.endsWith("offline.xml") ? "Hello Bob" : "Hello Carol"));
                }
            }
            if (!blockRecovery && !disjoint) {
                QCoreApplication::processEvents();
                const auto dialogs = window.findChildren<Dialog*>();
                QVERIFY(!dialogs.isEmpty());
                dialogs.last()->finished({ 2, "Compare versions", Dialog::AcceptButton });
                QCOMPARE(item->drafts().size(), 3);
                QVERIFY(item->drafts().last()->isComparison());
                QVERIFY(unrelated->drafts().isEmpty());
            }
            manager.saveChanges();
            QVERIFY2(!DatabaseLayer::Database::hasError(),
                     qPrintable(DatabaseLayer::Database::lastError()));
            manager.closeCurrentProject(path);
        }
        StorageFacade::clearStorages();
        DatabaseLayer::Database::setCurrentFile(path);
        if (!recoveryUuid.isNull()) {
            const auto recovery = StorageFacade::documentStorage()->document(recoveryUuid);
            QVERIFY(recovery);
            QVERIFY(recovery->content().contains("Hello Bob"));
        }
    }
};

QTEST_MAIN(ProjectSyncConflictsTest)
#include "tst_project_sync_conflicts.moc"

#include <business_layer/model/audioplay/audioplay_statistics_model.h>
#include <business_layer/model/comic_book/comic_book_statistics_model.h>
#include <business_layer/model/novel/novel_statistics_model.h>
#include <business_layer/model/screenplay/screenplay_statistics_model.h>
#include <business_layer/model/stageplay/stageplay_statistics_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>
#include <business_layer/model/text/text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/reports/audioplay/audioplay_summary_report.h>
#include <business_layer/reports/comic_book/comic_book_summary_report.h>
#include <business_layer/reports/novel/novel_summary_report.h>
#include <business_layer/reports/screenplay/screenplay_summary_report.h>
#include <business_layer/reports/stageplay/stageplay_summary_report.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/database.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <management_layer/content/project/project_models_facade.h>

#include <QPointer>
#include <QTemporaryDir>
#include <QtTest>

#ifdef TEST_STATISTICS_PLUGINS
#include <interfaces/management_layer/i_document_manager.h>
#include <interfaces/ui/i_document_view.h>
#include <ui/widgets/tree/tree.h>

#include <QPluginLoader>
#include <QStandardItemModel>
#endif

using namespace BusinessLayer;
using Domain::DocumentObjectType;

namespace {
int wordsCount(AbstractModel* _model)
{
    if (auto model = qobject_cast<ScreenplayStatisticsModel*>(_model)) {
        model->updateReports();
        return model->summaryReport().wordsCount();
    }
    if (auto model = qobject_cast<NovelStatisticsModel*>(_model)) {
        model->updateReports();
        return model->summaryReport().wordsCount();
    }
    if (auto model = qobject_cast<AudioplayStatisticsModel*>(_model)) {
        model->updateReports();
        return model->summaryReport().wordsCount();
    }
    if (auto model = qobject_cast<StageplayStatisticsModel*>(_model)) {
        model->updateReports();
        return model->summaryReport().wordsCount();
    }
    if (auto model = qobject_cast<ComicBookStatisticsModel*>(_model)) {
        model->updateReports();
        return model->summaryReport().wordsCount();
    }
    return -1;
}

#ifdef TEST_STATISTICS_PLUGINS
QAbstractItemModel* summaryTable(AbstractModel* _model)
{
    if (auto model = qobject_cast<ScreenplayStatisticsModel*>(_model)) {
        return model->summaryReport().textInfoModel();
    }
    if (auto model = qobject_cast<NovelStatisticsModel*>(_model)) {
        return model->summaryReport().textInfoModel();
    }
    if (auto model = qobject_cast<AudioplayStatisticsModel*>(_model)) {
        return model->summaryReport().textInfoModel();
    }
    if (auto model = qobject_cast<StageplayStatisticsModel*>(_model)) {
        return model->summaryReport().textInfoModel();
    }
    if (auto model = qobject_cast<ComicBookStatisticsModel*>(_model)) {
        return model->summaryReport().textInfoModel();
    }
    return nullptr;
}

bool displaysTable(QWidget* _view, QAbstractItemModel* _model)
{
    for (auto tree : _view->findChildren<Tree*>()) {
        if (tree->model() == _model) {
            return true;
        }
    }
    return false;
}
#endif
} // namespace

class StatisticsDraftsTest : public QObject
{
    Q_OBJECT

private slots:
    void separateReports_data()
    {
        QTest::addColumn<int>("documentType");
        QTest::addColumn<int>("statisticsType");
        QTest::addColumn<int>("paragraphType");
        QTest::newRow("screenplay") << int(DocumentObjectType::Screenplay)
                                   << int(DocumentObjectType::ScreenplayStatistics)
                                   << int(TextParagraphType::Action);
        QTest::newRow("novel") << int(DocumentObjectType::Novel)
                               << int(DocumentObjectType::NovelStatistics)
                               << int(TextParagraphType::Text);
        QTest::newRow("audioplay") << int(DocumentObjectType::Audioplay)
                                   << int(DocumentObjectType::AudioplayStatistics)
                                   << int(TextParagraphType::Sound);
        QTest::newRow("stageplay") << int(DocumentObjectType::Stageplay)
                                   << int(DocumentObjectType::StageplayStatistics)
                                   << int(TextParagraphType::Action);
        QTest::newRow("comic-book") << int(DocumentObjectType::ComicBook)
                                    << int(DocumentObjectType::ComicBookStatistics)
                                    << int(TextParagraphType::Description);
    }

    void separateReports()
    {
        QFETCH(int, documentType);
        QFETCH(int, statisticsType);
        QFETCH(int, paragraphType);

        QTemporaryDir temporaryDirectory;
        QVERIFY(temporaryDirectory.isValid());
        DatabaseLayer::Database::setCurrentFile(temporaryDirectory.filePath("project.starc"));
        {
            StructureModel structure;
            auto storage = DataStorageLayer::StorageFacade::documentStorage();
            connect(&structure, &StructureModel::documentAdded, &structure,
                    [storage](const QUuid& uuid, const QUuid&, DocumentObjectType type,
                              const QString&, const QByteArray& content) {
                        storage->createDocument(uuid, type)->setContent(content);
                    });
            structure.setDocument(storage->createDocument(QUuid::createUuid(),
                                                           DocumentObjectType::Structure));
            const auto documentIndex
                = structure.addDocument(static_cast<DocumentObjectType>(documentType));
            ManagementLayer::ProjectModelsFacade facade(&structure, nullptr, nullptr);
            const auto documentItem = structure.itemForIndex(documentIndex);
            QVERIFY(documentItem != nullptr);
            StructureModelItem* statistics = nullptr;
            for (int index = 0; index < documentItem->childCount(); ++index) {
                if (documentItem->childAt(index)->type()
                    == static_cast<DocumentObjectType>(statisticsType)) {
                    statistics = documentItem->childAt(index);
                }
            }
            QVERIFY(statistics != nullptr);
            auto source = facade.draftsItemFor(statistics);
            QVERIFY(source != nullptr);
            QVERIFY(source != statistics);
            QCOMPARE(facade.draftsItemFor(source), source);
            QVERIFY(facade.draftsItemFor(nullptr) == nullptr);

            auto currentText = qobject_cast<TextModel*>(facade.modelFor(source->uuid()));
            QVERIFY(currentText != nullptr);
            auto appendText = [paragraphType](TextModel* model, const QString& text) {
                auto item = model->createTextItem();
                item->setParagraphType(static_cast<TextParagraphType>(paragraphType));
                item->setText(text);
                model->appendItem(item, model->itemForIndex(model->index(0, 0)));
            };
            appendText(currentText, "one two three");
            currentText->reassignContent();
            const auto originalContent = currentText->document()->content();
            structure.addItemDraft(source, "First draft", Qt::red, true, originalContent, false);
            auto firstDraft = source->drafts().first();
            auto firstText = qobject_cast<TextModel*>(facade.modelFor(firstDraft->uuid()));
            QVERIFY(firstText != nullptr);
            appendText(currentText, "four five");

            auto currentReport = facade.modelForDraft(statistics->uuid(), source->uuid());
            auto firstReport = facade.modelForDraft(statistics->uuid(), firstDraft->uuid());
            QVERIFY(currentReport != nullptr);
            QVERIFY(firstReport != nullptr);
            QVERIFY(firstReport != currentReport);
            QCOMPARE(firstReport->document()->uuid(), statistics->uuid());
            QCOMPARE(wordsCount(currentReport), wordsCount(firstReport) + 2);
            QCOMPARE(facade.modelForDraft(source->uuid(), firstDraft->uuid()), firstText);

            // Opening a second draft must not rebind the first pane's report.
            structure.addItemDraft(source, "Second draft", Qt::blue, false, originalContent, false);
            auto secondDraft = source->drafts().first();
            auto secondText = qobject_cast<TextModel*>(facade.modelFor(secondDraft->uuid()));
            QVERIFY(secondText != nullptr);
            appendText(secondText, "six seven eight nine");
            auto secondReport = facade.modelForDraft(statistics->uuid(), secondDraft->uuid());
            QVERIFY(secondReport != nullptr);
            QVERIFY(secondReport != firstReport);
            QCOMPARE(wordsCount(secondReport), wordsCount(firstReport) + 4);
            QCOMPARE(wordsCount(currentReport), wordsCount(firstReport) + 2);
            QCOMPARE(facade.modelForDraft(statistics->uuid(), firstDraft->uuid()), firstReport);

#ifdef TEST_STATISTICS_PLUGINS
            // Load the actual private UI plugin through the production interface.
#ifdef Q_OS_MACOS
            const QDir plugins(QFINDTESTDATA("../../src/_build/starcapp.app/Contents/PlugIns"));
#else
            const QDir plugins(QFINDTESTDATA("../../src/_build/plugins"));
#endif
            const auto pluginName
                = QString::fromLatin1(QTest::currentDataTag()).remove('-') + "statisticsplugin";
            QString pluginPath;
            for (const auto& file : plugins.entryList({ "*" + pluginName + "*" }, QDir::Files)) {
                if (QLibrary::isLibrary(file)) {
                    pluginPath = plugins.filePath(file);
                    break;
                }
            }
            QVERIFY2(!pluginPath.isEmpty(), qPrintable(pluginName));
            QPluginLoader loader(pluginPath);
            auto manager = qobject_cast<ManagementLayer::IDocumentManager*>(loader.instance());
            QVERIFY2(manager != nullptr, qPrintable(loader.errorString()));
            auto left = manager->view(firstReport)->asQWidget();
            auto right = manager->secondaryView(secondReport)->asQWidget();
            QVERIFY(left != nullptr);
            QVERIFY(right != nullptr);
            QVERIFY(left != right);
            left->resize(1000, 700);
            right->resize(1000, 700);
            left->show();
            right->show();
            QCoreApplication::processEvents();
            QVERIFY(displaysTable(left, summaryTable(firstReport)));
            QVERIFY(displaysTable(right, summaryTable(secondReport)));

            QCOMPARE(manager->view(currentReport)->asQWidget(), left);
            QVERIFY(displaysTable(left, summaryTable(currentReport)));
            QVERIFY(displaysTable(right, summaryTable(secondReport)));
            QVERIFY(!displaysTable(left, summaryTable(firstReport)));
            QCOMPARE(manager->view(firstReport)->asQWidget(), left);
            QVERIFY(displaysTable(left, summaryTable(firstReport)));

            // Exercise the same report/plot slots used by the statistics navigator.
            if (statistics->type() == DocumentObjectType::ScreenplayStatistics
                || statistics->type() == DocumentObjectType::AudioplayStatistics) {
                QStandardItemModel navigation(6, 1);
                QVERIFY(QMetaObject::invokeMethod(loader.instance(), "showReport",
                                                  Q_ARG(QModelIndex, navigation.index(1, 0))));
                manager->view(currentReport);
                QVERIFY(QMetaObject::invokeMethod(loader.instance(), "showPlot",
                                                  Q_ARG(QModelIndex, navigation.index(0, 0))));
                manager->view(firstReport);
                QVERIFY(QMetaObject::invokeMethod(loader.instance(), "showReport",
                                                  Q_ARG(QModelIndex, navigation.index(0, 0))));
                QVERIFY(displaysTable(left, summaryTable(firstReport)));
                QVERIFY(displaysTable(right, summaryTable(secondReport)));
            }
            manager->resetModels();
            QVERIFY(!displaysTable(left, summaryTable(firstReport)));
            QVERIFY(!displaysTable(right, summaryTable(secondReport)));
            delete left;
            delete right;
#endif

            // A draft's rename and lock state do not change its report identity.
            structure.updateItemDraft(source, 1, "Renamed draft", Qt::green, false);
            QCOMPARE(facade.modelForDraft(statistics->uuid(), firstDraft->uuid()), firstReport);
            QVERIFY(facade.modelForDraft(statistics->uuid(), QUuid::createUuid()) == nullptr);
            QVERIFY(facade.modelForDraft(statistics->uuid(), statistics->uuid()) == nullptr);

            // A remote draft may be listed before its document has arrived.
            auto remoteDraft = source->addDraft("Remote draft", Qt::yellow, true, false);
            QVERIFY(facade.modelForDraft(statistics->uuid(), remoteDraft->uuid()) == nullptr);
            storage->createDocument(remoteDraft->uuid(), remoteDraft->type())
                ->setContent(originalContent);
            auto remoteReport = facade.modelForDraft(statistics->uuid(), remoteDraft->uuid());
            QVERIFY(remoteReport != nullptr);
            QCOMPARE(wordsCount(remoteReport), wordsCount(firstReport));

            QPointer<AbstractModel> removedReport = firstReport;
            const auto removedUuid = firstDraft->uuid();
            facade.removeModelFor(firstText->document());
            structure.removeItemDraft(source, 2);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QVERIFY(removedReport.isNull());
            QVERIFY(facade.modelForDraft(statistics->uuid(), removedUuid) == nullptr);
            QCOMPARE(wordsCount(secondReport), wordsCount(currentReport) + 2);

            QPointer<AbstractModel> remainingReport = secondReport;
            facade.clear();
            QVERIFY(remainingReport.isNull());
        }
        DataStorageLayer::StorageFacade::clearStorages();
        DatabaseLayer::Database::closeCurrentFile();
    }
};

QTEST_MAIN(StatisticsDraftsTest)
#include "tst_statistics_drafts.moc"

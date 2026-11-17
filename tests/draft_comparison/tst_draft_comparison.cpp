#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <management_layer/content/project/project_models_facade.h>
#include <management_layer/plugins/simple_text/simple_text_view.h>
#include <management_layer/plugins/simple_text/text/simple_text_edit.h>
#include <ui/modules/comments/comments_model.h>
#include <ui/modules/comments/comments_view.h>
#include <ui/project/project_view.h>
#include <ui/widgets/floating_tool_bar/comparison_toolbar.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>

#include <QAction>
#include <QGraphicsProxyWidget>
#include <QLoggingCategory>
#include <QtTest>

#include <memory>

using namespace BusinessLayer;

namespace {
std::unique_ptr<Domain::DocumentObject> makeDocument()
{
    return std::unique_ptr<Domain::DocumentObject>(Domain::ObjectsBuilder::createDocument(
        {}, QUuid::createUuid(), Domain::DocumentObjectType::SimpleText, QByteArray(""), {}));
}

Ui::SimpleTextEdit* textEdit(Ui::SimpleTextView* view)
{
    const auto wrapper = view->findChild<ScalableWrapper*>();
    for (auto item : wrapper->scene()->items()) {
        if (auto proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item)) {
            if (auto editor = qobject_cast<Ui::SimpleTextEdit*>(proxy->widget())) {
                return editor;
            }
        }
    }
    return nullptr;
}

void appendText(SimpleTextModel& model, const QString& text)
{
    auto item = model.createTextItem();
    item->setParagraphType(TextParagraphType::Text);
    item->setText(text);
    model.appendItem(item);
    model.saveChanges();
}
} // namespace

class DraftComparisonTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QLoggingCategory::setFilterRules("*.debug=false");
        qRegisterMetaType<BusinessLayer::AbstractModel*>("BusinessLayer::AbstractModel*");
    }

    void temporaryModelDoesNotChangeProject()
    {
        auto data = makeDocument();
        SimpleTextModel source;
        source.setDocument(data.get());
        appendText(source, QStringLiteral("Original draft"));
        const auto original = data->content();
        StructureModel structure;
        ManagementLayer::ProjectModelsFacade facade(&structure, nullptr, nullptr);
        QSignalSpy changes(&facade, &ManagementLayer::ProjectModelsFacade::modelContentChanged);
        QSignalSpy rows(&structure, &StructureModel::rowsInserted);
        const auto sourceRows = source.rowCount();
        std::unique_ptr<TextModel> comparison(facade.createComparisonModel(&source));
        QVERIFY(comparison != nullptr);
        QVERIFY(comparison->document()->uuid() != data->uuid());
        comparison->compareWith(QByteArray(original).replace("Original draft", "Changed draft"),
                                "Draft A", "Draft B");
        CommentsModel comments;
        comments.setTextModel(comparison.get());
        bool addition = false;
        bool removal = false;
        for (int row = 0; row < comments.rowCount(); ++row) {
            const auto index = comments.index(row, 0);
            addition |= index.data(CommentsModel::ReviewMarkIsAdditionRole).toBool();
            removal |= index.data(CommentsModel::ReviewMarkIsRemovalRole).toBool();
        }
        QVERIFY(addition);
        QVERIFY(removal);
        comparison->saveChanges();
        QCOMPARE(data->content(), original);
        QCOMPARE(source.rowCount(), sourceRows);
        QVERIFY(facade.loadedModels().isEmpty());
        QVERIFY(facade.loadedDocuments().isEmpty());
        QCOMPARE(changes.count(), 0);
        QCOMPARE(rows.count(), 0);
    }

    void savedComparisonIsAnEditableLastDraft()
    {
        auto sourceData = makeDocument();
        SimpleTextModel source;
        source.setDocument(sourceData.get());
        appendText(source, "Original draft");
        const auto original = sourceData->content();
        auto structureData = makeDocument();
        structureData->setType(Domain::DocumentObjectType::Structure);
        StructureModel structure;
        structure.setDocument(structureData.get());
        const auto hostIndex = structure.addDocument(Domain::DocumentObjectType::SimpleText,
                                                     "Document", {}, original);
        auto host = structure.itemForIndex(hostIndex);
        structure.addItemDraft(host, "Older draft", {}, false, original, false);
        structure.addItemDraft(host, "Locked draft", {}, true, original, false);
        structure.addItemDraft(host, "Legacy comparison", {}, true, original, true);
        const auto previousDrafts = host->drafts();
        const auto mainDraftName = host->draftName();
        const auto hostUuid = host->uuid();

        ManagementLayer::ProjectModelsFacade facade(&structure, nullptr, nullptr);
        std::unique_ptr<TextModel> comparison(facade.createComparisonModel(&source));
        comparison->compareWith(QByteArray(original).replace("Original draft", "Changed draft"),
                                "Draft A", "Draft B");
        comparison->saveChanges();
        const auto content = comparison->document()->content();
        QByteArray savedContent;
        QUuid savedUuid;
        int documentsAdded = 0;
        connect(&structure, &StructureModel::documentAdded, this,
                [&](const QUuid& uuid, const QUuid&, Domain::DocumentObjectType, const QString&,
                    const QByteArray& data) {
                    savedUuid = uuid;
                    savedContent = data;
                    ++documentsAdded;
                });
        structure.appendItemDraft(host, "Draft A vs Draft B", {}, content);
        QCOMPARE(documentsAdded, 1);
        QCOMPARE(savedContent, content);
        QCOMPARE(host->drafts().size(), previousDrafts.size() + 1);
        for (int index = 0; index < previousDrafts.size(); ++index) {
            QCOMPARE(host->drafts().at(index), previousDrafts.at(index));
        }
        const auto saved = host->drafts().constLast();
        QCOMPARE(saved->uuid(), savedUuid);
        QVERIFY(savedUuid != comparison->document()->uuid());
        QVERIFY(!saved->isReadOnly());
        QVERIFY(!saved->isComparison());
        QCOMPARE(saved->name(), QString("Draft A vs Draft B"));
        QCOMPARE(host->draftName(), mainDraftName);

        // The ordering and ordinary-draft flags must survive reopening the
        // project.
        structure.saveChanges();
        StructureModel reopened;
        reopened.setDocument(structureData.get());
        const auto restored = reopened.itemForUuid(hostUuid)->drafts().constLast();
        QCOMPARE(restored->uuid(), savedUuid);
        QVERIFY(!restored->isReadOnly());
        QVERIFY(!restored->isComparison());

        auto savedData = makeDocument();
        savedData->setUuid(savedUuid);
        savedData->setContent(savedContent);
        SimpleTextModel savedModel;
        savedModel.setDocument(savedData.get());
        CommentsModel savedComments;
        savedComments.setTextModel(&savedModel);
        QVERIFY(savedComments.rowCount() > 0);
        Ui::SimpleTextView savedView;
        savedView.setModel(&savedModel);
        savedView.setEditingMode(ManagementLayer::DocumentEditingMode::Edit);
        auto editor = textEdit(&savedView);
        QVERIFY(editor != nullptr);
        QVERIFY(!editor->isReadOnly());
        const auto before = editor->toPlainText();
        QTest::keyClicks(editor, "Editable ");
        QVERIFY(editor->toPlainText() != before);
        savedModel.saveChanges();
        QVERIFY(savedData->content() != savedContent);
        QCOMPARE(sourceData->content(), original);
    }

    void comparisonIsReadOnlyAndBackRestoresDraft()
    {
        auto data = makeDocument();
        SimpleTextModel source;
        source.setDocument(data.get());
        for (int i = 0; i < 60; ++i) {
            appendText(source, QString("Original paragraph %1").arg(i));
        }
        StructureModel structure;
        ManagementLayer::ProjectModelsFacade facade(&structure, nullptr, nullptr);
        Ui::ProjectView project;
        project.resize(1100, 800);
        auto original = new Ui::SimpleTextView;
        original->setModel(&source);
        project.showEditor(original);
        project.show();
        QTest::qWait(350);
        original->setCursorPosition(500);
        original->setVerticalScroll(200);
        const auto cursor = original->cursorPosition();
        const auto scroll = original->verticalScroll();
        const auto content = data->content();

        auto model = facade.createComparisonModel(&source);
        model->compareWith(
            QByteArray(content).replace("Original paragraph 10", "Changed paragraph 10"), "Draft A",
            "Draft B");
        auto comparison = new Ui::SimpleTextView;
        comparison->setModel(qobject_cast<SimpleTextModel*>(model));
        model->setParent(comparison);
        QPointer<TextModel> guardedModel(model);
        QPointer<Ui::SimpleTextView> guardedView(comparison);
        comparison->showComparison("Draft A vs Draft B", [&project] { project.closeComparison(); });
        project.showComparisonEditor(comparison);
        QTest::qWait(350);
        QCOMPARE(project.currentEditor(), comparison);
        auto editor = textEdit(comparison);
        QVERIFY(editor != nullptr);
        QVERIFY(editor->isReadOnly());
        comparison->setEditingMode(ManagementLayer::DocumentEditingMode::Edit);
        QVERIFY(editor->isReadOnly());
        auto comments = comparison->findChild<Ui::CommentsView*>();
        QVERIFY(comments != nullptr);
        QVERIFY(comments->isReadOnly());
        QTRY_VERIFY(comments->isVisible());
        const auto commentModel = comparison->findChild<CommentsModel*>();
        QVERIFY(commentModel != nullptr);
        QVERIFY(commentModel->rowCount() > 0);
        const auto beforeTyping = editor->toPlainText();
        QTest::keyClicks(editor, "must not be inserted");
        QCOMPARE(editor->toPlainText(), beforeTyping);
        auto toolbar = comparison->findChild<Ui::ComparisonToolbar*>();
        QVERIFY(toolbar != nullptr);
        QVERIFY(toolbar->isVisible());
        QCOMPARE(toolbar->actions().at(1)->toolTip(), QString("Draft A vs Draft B"));
        QSignalSpy saveRequested(toolbar, &Ui::ComparisonToolbar::saveRequested);
        toolbar->setSavingEnabled(false);
        QVERIFY(!toolbar->actions().last()->isEnabled());
        toolbar->setSavingEnabled(true);
        toolbar->actions().last()->trigger();
        QCOMPARE(saveRequested.count(), 1);
        QCOMPARE(project.currentEditor(), comparison);
        QVERIFY(editor->isReadOnly());
        toolbar->actions().first()->trigger();
        QCOMPARE(project.currentEditor(), original);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(guardedModel.isNull());
        QVERIFY(guardedView.isNull());
        // Exercise resizing during the outgoing comparison animation after its deletion.
        project.resize(1000, 750);
        QTest::qWait(350);
        QCOMPARE(original->cursorPosition(), cursor);
        QCOMPARE(original->verticalScroll(), scroll);
        QCOMPARE(data->content(), content);
    }

    void navigationAndRepeatedComparisonReleaseTemporaryViews()
    {
        Ui::ProjectView left;
        Ui::ProjectView right;
        auto original = new QWidget;
        auto otherOriginal = new QWidget;
        left.showEditor(original);
        right.showEditor(otherOriginal);
        QPointer<QWidget> first = new QWidget;
        QPointer<QWidget> second = new QWidget;
        QPointer<QWidget> other = new QWidget;
        left.showComparisonEditor(first);
        right.showComparisonEditor(other);
        left.showComparisonEditor(second);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(first.isNull());
        QCOMPARE(left.comparisonEditor(), second.data());
        QCOMPARE(right.comparisonEditor(), other.data());
        left.closeComparison();
        QCOMPARE(left.currentEditor(), original);
        QCOMPARE(right.currentEditor(), other.data());
        right.showDefaultPage();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(second.isNull());
        QVERIFY(other.isNull());
        QVERIFY(right.comparisonEditor() == nullptr);
        right.showEditor(otherOriginal);
        QPointer<QWidget> temporary = new QWidget;
        right.showComparisonEditor(temporary);
        auto nextDocument = new QWidget;
        right.showEditor(nextDocument);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(temporary.isNull());
        QCOMPARE(right.currentEditor(), nextDocument);
    }
};

QTEST_MAIN(DraftComparisonTest)
#include "tst_draft_comparison.moc"

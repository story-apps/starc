#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <management_layer/plugins/screenplay_text/screenplay_text_view.h>
#include <management_layer/plugins/screenplay_text/text/screenplay_text_edit.h>
#include <ui/widgets/text_edit/scalable_wrapper/scalable_wrapper.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>

#include <QGraphicsProxyWidget>
#include <QLoggingCategory>
#include <QTextBlock>
#include <QtTest>

#include <memory>

using namespace BusinessLayer;

class TestModel : public ScreenplayTextModel
{
public:
    using ScreenplayTextModel::toXml;
    QByteArray makePatch(const QByteArray& before, const QByteArray& after) const
    {
        return dmpController().makePatch(QString::fromUtf8(before), QString::fromUtf8(after));
    }
};

class RemoteTextChangesTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QLoggingCategory::setFilterRules("*.debug=false");
    }

    void queuedResetDoesNotOverridePosition_data()
    {
        QTest::addColumn<qreal>("zoom");
        QTest::addColumn<bool>("correctBreaks");
        for (bool correctBreaks : { false, true }) {
            for (qreal zoom : { 0.75, 1.0, 1.5 }) {
                const auto name = QString("zoom-%1-correct-breaks-%2").arg(zoom).arg(correctBreaks);
                QTest::newRow(qPrintable(name)) << zoom << correctBreaks;
            }
        }
    }

    void queuedResetDoesNotOverridePosition()
    {
        QFETCH(qreal, zoom);
        QFETCH(bool, correctBreaks);
        std::unique_ptr<Domain::DocumentObject> data(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::ScreenplayText, {}, {}));
        ScreenplayInformationModel information;
        ScreenplayDictionariesModel dictionaries;
        TestModel model;
        model.setInformationModel(&information);
        model.setDictionariesModel(&dictionaries);
        model.setDocument(data.get());
        auto scene = model.itemForIndex(model.index(0, 0));
        for (int i = 0; i < 70; ++i) {
            auto item = model.createTextItem();
            item->setParagraphType(TextParagraphType::Action);
            const auto action = QString("Some screenplay action for the regression test. ")
                                    .repeated(4)
                                    .trimmed();
            item->setText(QString("Paragraph %1: ").arg(i) + action
                          + (i == 40 ? QString(" TARGET-END") : QString()));
            model.appendItem(item, scene);
        }

        Ui::ScreenplayTextView view;
        view.resize(1000, 800);
        view.setModel(&model);
        auto wrapper = view.findChild<ScalableWrapper*>();
        QVERIFY(wrapper != nullptr);
        Ui::ScreenplayTextEdit* editor = nullptr;
        for (auto item : wrapper->scene()->items()) {
            if (auto proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item)) {
                editor = qobject_cast<Ui::ScreenplayTextEdit*>(proxy->widget());
                if (editor != nullptr) {
                    break;
                }
            }
        }
        QVERIFY(editor != nullptr);
        editor->setUsePageMode(true);
        editor->setCorrectionOptions(false, correctBreaks);
        wrapper->setZoomRange(zoom);
        view.show();
        QTest::qWait(50);
        auto cursor = editor->document()->find("TARGET-END");
        QVERIFY(!cursor.isNull());
        cursor.setPosition(cursor.selectionEnd());
        editor->setTextCursor(cursor);
        const int lowerViewportPosition = editor->viewport()->height() - 100;
        editor->setVerticalScroll(editor->verticalScroll() + editor->cursorRect().top()
                                  - lowerViewportPosition);
        QTest::qWait(50);
        const auto oldTop = editor->cursorRect().top();
        QVERIFY(oldTop > editor->viewport()->height() / 2);

        // An empty paragraph is the only text change. Distant formatting changes make
        // TextModel rebuild the document, as in a large incoming collaboration patch.
        const auto before = model.toXml();
        std::unique_ptr<Domain::DocumentObject> remoteData(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::ScreenplayText, before, {}));
        TestModel remote;
        remote.setInformationModel(&information);
        remote.setDocument(remoteData.get());
        auto remoteScene = remote.itemForIndex(remote.index(0, 0));
        auto empty = remote.createTextItem();
        empty->setParagraphType(TextParagraphType::Action);
        remote.insertItem(empty, remoteScene->childAt(2));
        auto last = static_cast<TextModelTextItem*>(
            remoteScene->childAt(remoteScene->childCount() - 1));
        QTextLayout::FormatRange format;
        format.start = 0;
        format.length = 10;
        format.format.setFontWeight(QFont::Bold);
        last->setFormats({ format });
        remote.updateItem(last);
        const auto after = remote.toXml();
        QSignalSpy resets(&model, &ScreenplayTextModel::modelReset);
        model.applyDocumentChanges({ model.makePatch(before, after) });
        QVERIFY(!resets.isEmpty());
        QCOMPARE(model.toXml(), after);
        QCOMPARE(editor->textCursor().position(),
                 editor->document()->find("TARGET-END").selectionEnd());
        QCOMPARE(editor->cursorRect().top(), oldTop);

        // ScreenplayTextView has a queued modelReset handler in addition to the editor.
        // It must not overwrite the position restored at the end of the cloud batch.
        QTest::qWait(100);
        QCOMPARE(editor->textCursor().position(),
                 editor->document()->find("TARGET-END").selectionEnd());
        QCOMPARE(editor->cursorRect().top(), oldTop);
    }

    void preservePosition_data()
    {
        QTest::addColumn<bool>("pageMode");
        QTest::addColumn<QString>("change");
        const QStringList changes = {
            "grow-above", "shrink-above", "same-paragraph-before", "same-paragraph-after",
            "disjoint-edits", "batch", "forward-selection", "backward-selection",
            "offscreen-cursor", "insert-paragraph", "delete-paragraph", "delete-current",
            "empty-batch", "unicode", "wide-patch", "wide-batch", "wide-selection"
        };
        for (bool pageMode : { false, true }) {
            for (const auto& change : changes) {
                QTest::newRow(qPrintable((pageMode ? "paged-" : "continuous-") + change))
                    << pageMode << change;
            }
        }
    }

    void preservePosition()
    {
        QFETCH(bool, pageMode);
        QFETCH(QString, change);

        std::unique_ptr<Domain::DocumentObject> data(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::ScreenplayText, {}, {}));
        ScreenplayInformationModel information;
        ScreenplayDictionariesModel dictionaries;
        TestModel model;
        model.setInformationModel(&information);
        model.setDictionariesModel(&dictionaries);
        model.setDocument(data.get());
        auto scene = model.itemForIndex(model.index(0, 0));
        const QString above = "ABOVE " + QString("old words ").repeated(60) + " end above";
        const QString target = "Beginning of paragraph. TARGET anchor tail of paragraph.";
        for (int i = 0; i < 100; ++i) {
            auto item = model.createTextItem();
            item->setParagraphType(TextParagraphType::Action);
            item->setText(i == 10 ? above : i == 60 ? target
                : QString("Paragraph %1: a line of screenplay action for layout testing.").arg(i));
            model.appendItem(item, scene);
        }

        Ui::ScreenplayTextEdit editor;
        editor.resize(900, 600);
        editor.setUsePageMode(pageMode);
        editor.setCorrectionOptions(false, pageMode);
        editor.initWithModel(&model);
        editor.show();
        QTest::qWait(20);
        auto find = [&](const QString& text) {
            auto result = editor.document()->find(text);
            result.setPosition(result.selectionStart());
            return result;
        };
        auto cursor = find("TARGET");
        QVERIFY(!cursor.isNull());
        if (change == "forward-selection" || change == "wide-selection") {
            cursor.setPosition(cursor.position() + 13, QTextCursor::KeepAnchor);
        } else if (change == "backward-selection") {
            cursor.setPosition(cursor.position() + 13);
            cursor.setPosition(cursor.position() - 13, QTextCursor::KeepAnchor);
        }
        editor.setTextCursor(cursor);
        const auto selected = cursor.selectedText();
        const auto backward = cursor.position() < cursor.anchor();
        auto viewAnchor = change == "offscreen-cursor" ? find("Paragraph 35:") : cursor;
        viewAnchor.clearSelection();
        editor.setVerticalScroll(editor.verticalScroll() + editor.cursorRect(viewAnchor).top() - 180);
        QTest::qWait(20);
        const auto oldTop = editor.cursorRect(viewAnchor).top();
        const auto oldHorizontal = editor.horizontalScroll();

        auto xml = model.toXml();
        QVector<QByteArray> patches;
        auto patch = [&](const QByteArray& next) {
            patches.append(model.makePatch(xml, next));
            xml = next;
        };
        auto next = xml;
        const auto larger = QString("new remote words ").repeated(100).toUtf8();
        if (change == "grow-above" || change.endsWith("selection")
            || change == "offscreen-cursor" || change == "batch") {
            next.replace(above.toUtf8(), above.toUtf8() + larger);
        } else if (change == "shrink-above") {
            next.replace(above.toUtf8(), "ABOVE end above");
        } else if (change == "same-paragraph-before") {
            next.replace("TARGET", "remote insertion TARGET");
        } else if (change == "same-paragraph-after") {
            next.replace("tail of paragraph", "remotely edited tail of paragraph");
        } else if (change == "disjoint-edits") {
            next.replace("Beginning of paragraph.", "New beginning.");
            next.replace("tail of paragraph.", "a different ending.");
        } else if (change == "unicode") {
            next.replace("TARGET", QString::fromUtf8("😀 е́ TARGET").toUtf8());
        } else if (change == "insert-paragraph" || change == "delete-paragraph") {
            const auto start = next.lastIndexOf("<action", next.indexOf("Paragraph 20:"));
            const auto end = next.indexOf("</action>", start) + QByteArray("</action>\n").size();
            QVERIFY(start >= 0 && end > start);
            if (change == "insert-paragraph") {
                next.insert(start, next.mid(start, end - start));
            } else {
                next.remove(start, end - start);
            }
        } else if (change == "delete-current") {
            next.replace(target.toUtf8(), "Replacement paragraph.");
        }
        if (change == "forward-selection" || change == "backward-selection") {
            patch(next);
            next = xml;
            next.replace("Beginning of paragraph.", "Remote prefix. Beginning of paragraph.");
            next.replace("tail of paragraph.", "tail of remote ending.");
        }
        if (change.startsWith("wide-")) {
            next.replace("Paragraph 15:", "Inserted words in Paragraph 15:");
            next.replace("Paragraph 50:", "Inserted words in Paragraph 50:");
        }
        if (change != "empty-batch") {
            patch(next);
        }
        if (change == "batch" || change == "wide-batch") {
            next = xml;
            next.replace("Beginning of paragraph.", "Remote prefix. Beginning of paragraph.");
            patch(next);
            next = xml;
            next.replace("Paragraph 80:", "A later change in Paragraph 80:");
            patch(next);
        }

        model.applyDocumentChanges(patches);
        QCOMPARE(model.toXml(), xml);
        // Check immediately and again after deferred layout/correction work.
        for (int pass = 0; pass < 2; ++pass) {
            if (pass != 0) {
                QTest::qWait(30);
            }
            if (change == "delete-current") {
                QVERIFY(editor.textCursor().position() >= 0);
                QVERIFY(editor.textCursor().position() < editor.document()->characterCount());
                QVERIFY(editor.toPlainText().contains("Replacement paragraph."));
                continue;
            }
            const auto expectedStart = find("TARGET").position();
            const auto expectedPosition = expectedStart + ((change == "forward-selection" || change == "wide-selection") ? 13 : 0);
            QCOMPARE(editor.textCursor().position(), expectedPosition);
            QCOMPARE(editor.textCursor().selectedText(), selected);
            QCOMPARE(editor.textCursor().position() < editor.textCursor().anchor(), backward);
            const auto expectedAnchor = change == "offscreen-cursor" ? find("Paragraph 35:")
                                                                       : editor.textCursor();
            QCOMPARE(editor.cursorRect(expectedAnchor).top(), oldTop);
            QCOMPARE(editor.horizontalScroll(), oldHorizontal);
        }
    }
};

QTEST_MAIN(RemoteTextChangesTest)
#include "tst_remote_text_changes.moc"

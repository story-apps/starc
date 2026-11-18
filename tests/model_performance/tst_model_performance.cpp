#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/abstract_model_item.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

#include <QRandomGenerator>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QtTest>

#include <memory>

using namespace BusinessLayer;

namespace {
class Item : public AbstractModelItem
{
public:
    QVariant data(int) const override
    {
        return {};
    }
};

class CountingParagraph : public ScreenplayTextModelTextItem
{
public:
    using ScreenplayTextModelTextItem::ScreenplayTextModelTextItem;
    int updates = 0;

    void updateCounters(bool force = false) override
    {
        ++updates;
        ScreenplayTextModelTextItem::updateCounters(force);
    }
};

class CountingScene : public ScreenplayTextModelSceneItem
{
public:
    using ScreenplayTextModelSceneItem::ScreenplayTextModelSceneItem;
    int updates = 0;

protected:
    void handleChange() override
    {
        ++updates;
        ScreenplayTextModelSceneItem::handleChange();
    }
};
}

class ModelPerformanceTest : public QObject
{
    Q_OBJECT

private slots:
    void paragraphChangesAreCoalesced()
    {
        ScreenplayInformationModel information;
        ScreenplayTextModel model;
        model.setInformationModel(&information);
        CountingScene scene(&model);
        auto paragraph = new CountingParagraph(&model);
        paragraph->setParagraphType(TextParagraphType::Action);
        paragraph->setText(QStringLiteral("One word."));
        scene.appendItem(paragraph);
        paragraph->updates = scene.updates = 0;
        paragraph->setText(QStringLiteral("One more word."));
        QCOMPARE(paragraph->updates, 1);
        QCOMPARE(scene.updates, 1);
        QCOMPARE(scene.wordsCount(), 3);
        QCOMPARE(scene.text(), paragraph->text());
        QCOMPARE(scene.duration(), paragraph->duration());
        QCOMPARE(scene.eighths(), paragraph->eighths());

        paragraph->updates = scene.updates = 0;
        paragraph->setAlignment(Qt::AlignRight);
        paragraph->setNumber(12);
        paragraph->setBookmark({ Qt::red, QStringLiteral("Bookmark") });
        QCOMPARE(paragraph->updates, 0);
        QCOMPARE(scene.updates, 3);
        QCOMPARE(scene.wordsCount(), 3);
        QVERIFY(paragraph->isChanged());
        QVERIFY(paragraph->toXml().contains("Bookmark"));

        TextModelTextItem::ReviewMark review;
        review.length = 3;
        review.comments.append({ {}, {}, {}, QStringLiteral("Review this") });
        paragraph->setReviewMarks(QVector<TextModelTextItem::ReviewMark> { review });
        QCOMPARE(paragraph->updates, 0);
        QCOMPARE(scene.reviewMarksSize(), 1);
        review.isDone = true;
        paragraph->setReviewMarks(QVector<TextModelTextItem::ReviewMark> { review });
        QCOMPARE(paragraph->updates, 0);
        QCOMPARE(scene.reviewMarksSize(), 0);
        QVERIFY(paragraph->toXml().contains("Review this"));

        paragraph->updates = 0;
        paragraph->setParagraphType(TextParagraphType::Dialogue);
        QCOMPARE(paragraph->updates, 1);
        paragraph->updates = 0;
        paragraph->updateCounters(true);
        QCOMPARE(paragraph->updates, 1);
        QCOMPARE(scene.duration(), paragraph->duration());
    }

    void countersSurviveTextMutations()
    {
        ScreenplayInformationModel screenplayInformation;
        AudioplayInformationModel audioplayInformation;
        ScreenplayTextModel screenplay;
        screenplay.setInformationModel(&screenplayInformation);
        AudioplayTextModel audioplay;
        audioplay.setInformationModel(&audioplayInformation);
        SimpleTextModel simple;
        NovelTextModel novel;
        ComicBookTextModel comic;
        StageplayTextModel stageplay;
        const QVector<TextModel*> models { &screenplay, &audioplay, &simple, &novel, &comic,
                                          &stageplay };
        for (auto model : models) {
            std::unique_ptr<TextModelTextItem> item(model->createTextItem());
            item->setParagraphType(TextParagraphType::Action);
            auto verify = [&] {
                QCOMPARE(item->wordsCount(), TextHelper::wordsCount(item->text()));
                const int length = item->text().size();
                QCOMPARE(item->charactersCount(),
                         qMakePair(length - int(item->text().count(' ')), length + 1));
            };
            verify(); // Empty paragraphs also have valid counters.
            item->beginChangeTransaction();
            item->setText(QStringLiteral("Привет, мир! One two."));
            item->setAlignment(Qt::AlignRight);
            item->finishChangeTransaction();
            verify();
            item->removeText(0, 8);
            verify();
            std::unique_ptr<TextModelTextItem> other(model->createTextItem());
            other->setParagraphType(TextParagraphType::Action);
            other->setText(QStringLiteral(" Another paragraph."));
            item->mergeWith(other.get());
            verify();
            item->copyFrom(other.get());
            verify();
            QXmlStreamReader reader("<action><value><![CDATA[Loaded text.]]></value></action>");
            QVERIFY(reader.readNextStartElement());
            item->readContent(reader);
            verify();
            item->setCorrection(true);
            item->setText(QStringLiteral("Changed during page correction."));
            item->setCorrection(false);
            verify();
            item->setText({});
            verify();
        }
    }

    void forcedChronometryRefresh()
    {
        AudioplayInformationModel information;
        information.setOverrideCommonSettings(true);
        AudioplayTextModel model;
        model.setInformationModel(&information);
        std::unique_ptr<AudioplayTextModelTextItem> item(
            static_cast<AudioplayTextModelTextItem*>(model.createTextItem()));
        item->setParagraphType(TextParagraphType::Dialogue);
        item->setText(QStringLiteral("One two three four."));
        const auto before = item->duration();
        auto options = information.chronometerOptions();
        options.words.seconds *= 2;
        information.setChronometerOptions(options);
        item->updateCounters(true);
        QCOMPARE(item->duration(), AudioplayChronometer::duration(
                                      item->paragraphType(), item->text(),
                                      information.templateId(), options));
        QVERIFY(item->duration() > before);
    }

    void rangeRemovalPreservesReparentedItems()
    {
        QVector<int> deleted;
        Item source;
        Item destination;
        class TrackedItem : public Item
        {
        public:
            TrackedItem(int id, QVector<int>& deleted)
                : id(id)
                , deleted(deleted)
            {
            }
            ~TrackedItem() override
            {
                deleted.append(id);
            }
            int id;
            QVector<int>& deleted;
        };
        QVector<AbstractModelItem*> items;
        for (int i = 0; i < 6; ++i) {
            items.append(new TrackedItem(i, deleted));
        }
        source.appendItems(items);
        source.removeItems(4, 2);
        source.takeItems(4, 2);
        QCOMPARE(source.childCount(), 6);
        destination.appendItems({ items[1], items[3] });
        source.removeItems(0, 4);
        QCOMPARE(deleted, QVector<int>({ 4, 2, 0 }));
        QCOMPARE(source.childCount(), 3);
        QCOMPARE(source.childAt(0), items[1]);
        QCOMPARE(source.childAt(1), items[3]);
        QCOMPARE(source.childAt(2), items[5]);
        QCOMPARE(source.rowOfChild(items[5]), 2);
        source.takeItems(0, 2);
        QCOMPARE(source.childCount(), 2);
        QCOMPARE(items[5]->parent(), nullptr);
        delete items[5];
        for (auto item : { items[1], items[3] }) {
            QCOMPARE(item->parent(), &destination);
            item->setParent(&source);
        }
        source.takeItems(0, 1);
        items[1]->setParent(&destination);
        items[3]->setParent(&destination);
        // Leave neither tree with shared ownership at destruction.
        destination.removeItems(0, 1);
        QCOMPARE(deleted, QVector<int>({ 4, 2, 0, 5, 3, 1 }));
    }

    void treeMutations()
    {
        Item root;
        QVector<AbstractModelItem*> expected;
        QRandomGenerator random(42);
        for (int step = 0; step < 1000; ++step) {
            const int operation = random.bounded(5);
            if (expected.isEmpty() || operation < 3) {
                QVector<AbstractModelItem*> items { new Item, new Item, new Item };
                const int row = operation == 0 ? 0
                    : operation == 1 ? expected.size() : random.bounded(expected.size() + 1);
                if (row == 0) {
                    root.prependItems(items);
                } else if (row == expected.size()) {
                    root.appendItems(items);
                } else {
                    root.insertItems(row, items);
                }
                for (int i = 0; i < items.size(); ++i) {
                    expected.insert(row + i, items[i]);
                }
            } else {
                const int from = random.bounded(expected.size());
                const int count = random.bounded(expected.size() - from) + 1;
                if (operation == 3) {
                    root.removeItems(from, from + count - 1);
                    expected.remove(from, count);
                } else {
                    const auto moved = expected.mid(from, count);
                    root.takeItems(from, from + count - 1);
                    expected.remove(from, count);
                    for (auto item : moved) {
                        QVERIFY(!item->hasParent());
                        QCOMPARE(root.rowOfChild(item), -1);
                    }
                    root.appendItems(moved);
                    expected += moved;
                }
            }
            QCOMPARE(root.childCount(), expected.size());
            for (int row = 0; row < expected.size(); ++row) {
                QCOMPARE(root.childAt(row), expected[row]);
                QCOMPARE(root.rowOfChild(expected[row]), row);
                QCOMPARE(expected[row]->parent(), &root);
            }
            QCOMPARE(root.rowOfChild(nullptr), -1);
        }

        root.sortChildren(std::less<AbstractModelItem*>());
        std::sort(expected.begin(), expected.end(), std::less<AbstractModelItem*>());
        for (int row = 0; row < expected.size(); ++row) {
            QCOMPARE(root.childAt(row), expected[row]);
            QCOMPARE(root.rowOfChild(expected[row]), row);
        }
    }

    void temporaryReparenting()
    {
        Item source;
        Item destination;
        auto first = new Item;
        auto second = new Item;
        source.appendItems({ first, second });
        // Patch application can attach items to the new tree before removing the old tree.
        destination.appendItem(second);
        QCOMPARE(source.rowOfChild(second), 1);
        QCOMPARE(destination.rowOfChild(second), 0);
        source.takeItems(0, 1);
        QCOMPARE(source.childCount(), 1);
        QCOMPARE(source.rowOfChild(second), 0);
        QCOMPARE(second->parent(), &destination);
        source.removeItem(second); // Reparented items must not be deleted by the old tree.
        QCOMPARE(source.childCount(), 1);
        second->setParent(&source);
        source.takeItem(second);
        second->setParent(&destination);
        delete first;

        Item draft;
        draft.setParent(&destination); // A parent pointer does not imply membership.
        QCOMPARE(destination.rowOfChild(&draft), -1);
        destination.prependItems({ second, second });
        QCOMPARE(destination.childCount(), 1);
    }

    void batchInsertionWithDuplicates()
    {
        Item root;
        auto first = new Item;
        auto second = new Item;
        auto third = new Item;
        root.appendItem(first);
        root.prependItems({ second, first, third, second });
        // Existing children are skipped; repeated new children keep their last position.
        const QVector<AbstractModelItem*> expected { third, second, first };
        QCOMPARE(root.childCount(), expected.size());
        for (int row = 0; row < expected.size(); ++row) {
            QCOMPARE(root.childAt(row), expected[row]);
            QCOMPARE(root.rowOfChild(expected[row]), row);
        }
        root.insertItems(1, {});
        QCOMPARE(root.childCount(), expected.size());
    }

    void wordCountCompatibility()
    {
        const QRegularExpression separators("[\\s.,!():;]+");
        auto check = [&](const QString& text) {
            QCOMPARE(TextHelper::wordsCount(text),
                     text.split(separators, Qt::SkipEmptyParts).count());
        };
        check({});
        check(QStringLiteral("Привет, мир! Hello\tworld. 中文 日本語 👩‍💻"));
        check(QStringLiteral("  .,!!():;\r\n\t "));
        check(QStringLiteral("one\u00a0two\u2003three\u2028four"));
        check(QString(20000, 'x'));
        // Includes malformed UTF-16, whose treatment must also match QString::split.
        for (uint character = 0; character <= 0xffff; ++character) {
            check(QStringLiteral("a") + QChar(character) + QStringLiteral("b"));
        }
        QRandomGenerator random(17);
        for (int i = 0; i < 1000; ++i) {
            QString text;
            for (int j = 0; j < 200; ++j) {
                text += QChar(random.bounded(128));
            }
            check(text);
        }
    }

    void screenplayCounters()
    {
        ScreenplayInformationModel information;
        ScreenplayTextModel model;
        model.setInformationModel(&information);
        std::unique_ptr<ScreenplayTextModelTextItem> item(
            static_cast<ScreenplayTextModelTextItem*>(model.createTextItem()));
        const auto options = information.chronometerOptions();
        for (auto type : { TextParagraphType::Action, TextParagraphType::Dialogue,
                           TextParagraphType::SceneHeading, TextParagraphType::Character }) {
            item->setParagraphType(type);
            for (const auto& text : { QString(), QStringLiteral("Привет, мир!"),
                                     QStringLiteral("Action and dialogue. ").repeated(150) }) {
                item->setText(text);
                item->updateCounters(true);
                QCOMPARE(item->wordsCount(), TextHelper::wordsCount(text));
                QCOMPARE(item->charactersCount(), qMakePair(text.size() - text.count(' '),
                                                          text.size() + 1));
                QCOMPARE(item->duration().count(),
                         ScreenplayChronometer::duration(type, text, information.templateId(),
                                                        options).count());
                QCOMPARE(item->eighths(), ScreenplayChronometer::eighths(
                                            type, text, information.templateId(), options));
            }
        }
    }

    void combinedChronometry()
    {
        for (auto mode : { ChronometerType::Page, ChronometerType::Characters,
                           ChronometerType::Sophocles }) {
            ChronometerOptions options;
            options.type = mode;
            options.page.seconds = 73;
            options.characters.considerSpaces = false;
            for (auto type : { TextParagraphType::Action, TextParagraphType::Dialogue,
                               TextParagraphType::SceneHeading, TextParagraphType::Character }) {
                for (const auto& text : { QString(), QStringLiteral("Привет, мир!"),
                                         QStringLiteral("A line of dialogue.\n").repeated(100) }) {
                    const auto result = ScreenplayChronometer::counters(type, text, {}, options);
                    QCOMPARE(result.duration.count(),
                             ScreenplayChronometer::duration(type, text, {}, options).count());
                    QCOMPARE(result.eighths,
                             ScreenplayChronometer::eighths(type, text, {}, options));
                }
            }
        }
    }

    void nestedXmlRoundTrip()
    {
        ScreenplayInformationModel information;
        ScreenplayTextModel model;
        model.setInformationModel(&information);
        std::unique_ptr<TextModelFolderItem> folder(model.createFolderItem(TextFolderType::Act));
        auto scene = model.createGroupItem(TextGroupType::Scene);
        auto beat = model.createGroupItem(TextGroupType::Beat);
        auto heading = model.createTextItem();
        heading->setParagraphType(TextParagraphType::SceneHeading);
        heading->setText(QStringLiteral("INT. HOUSE - DAY"));
        scene->appendItem(heading);
        for (int i = 0; i < 20; ++i) {
            auto text = model.createTextItem();
            text->setParagraphType(TextParagraphType::Action);
            text->setText(QStringLiteral("Привет, мир! Action %1.").arg(i));
            beat->appendItem(text);
        }
        scene->appendItem(beat);
        folder->appendItem(scene);
        const auto xml = folder->toXml();
        QXmlStreamReader reader(xml);
        QVERIFY(reader.readNextStartElement());
        std::unique_ptr<TextModelFolderItem> restored(model.TextModel::createFolderItem(reader));
        QVERIFY(!reader.hasError());
        QCOMPARE(restored->toXml(), xml);
        QCOMPARE(restored->wordsCount(), folder->wordsCount());
        QCOMPARE(restored->charactersCount(), folder->charactersCount());
        QCOMPARE(restored->childCount(), 1);
        auto restoredScene = static_cast<TextModelGroupItem*>(restored->childAt(0));
        QCOMPARE(restoredScene->heading(), scene->heading());
        QCOMPARE(restoredScene->text(), scene->text());
        QCOMPARE(restoredScene->childCount(), 2);
        QCOMPARE(restoredScene->childAt(1)->childCount(), 20);
    }

    void benchmarkLoadScene()
    {
        ScreenplayInformationModel information;
        ScreenplayTextModel model;
        model.setInformationModel(&information);
        std::unique_ptr<TextModelGroupItem> scene(model.createGroupItem(TextGroupType::Scene));
        QVector<AbstractModelItem*> paragraphs;
        for (int i = 0; i < 1000; ++i) {
            auto item = model.createTextItem();
            item->setParagraphType(TextParagraphType::Action);
            item->setText(QStringLiteral("Some screenplay action."));
            paragraphs.append(item);
        }
        scene->appendItems(paragraphs);
        const auto xml = scene->toXml();
        QBENCHMARK {
            QXmlStreamReader reader(xml);
            reader.readNextStartElement();
            std::unique_ptr<TextModelGroupItem> restored(model.TextModel::createGroupItem(reader));
        }
    }

    void benchmarkRows()
    {
        Item root;
        QVector<AbstractModelItem*> items;
        for (int i = 0; i < 20000; ++i) {
            items.append(new Item);
        }
        root.appendItems(items);
        qint64 sum = 0;
        QBENCHMARK {
            sum = 0;
            for (auto item : items) {
                sum += root.rowOfChild(item);
            }
        }
        QCOMPARE(sum, qint64(199990000));
    }

    void benchmarkWords()
    {
        const auto text = QStringLiteral("Some words, punctuation! Привет мир. ").repeated(200);
        int count = 0;
        QBENCHMARK { count = TextHelper::wordsCount(text); }
        QCOMPARE(count, 1000);
    }

    void benchmarkScreenplayCounters()
    {
        ScreenplayInformationModel information;
        ScreenplayTextModel model;
        model.setInformationModel(&information);
        std::unique_ptr<TextModelTextItem> item(model.createTextItem());
        item->setParagraphType(TextParagraphType::Action);
        item->setText(QStringLiteral("Some screenplay action for the benchmark. ").repeated(20));
        QBENCHMARK { item->updateCounters(true); }
    }

    void benchmarkChronometry_data()
    {
        QTest::addColumn<bool>("combined");
        QTest::newRow("separate-layouts") << false;
        QTest::newRow("shared-layout") << true;
    }

    void benchmarkParagraphMetadata()
    {
        ScreenplayInformationModel information;
        ScreenplayTextModel model;
        model.setInformationModel(&information);
        std::unique_ptr<TextModelTextItem> item(model.createTextItem());
        item->setParagraphType(TextParagraphType::Action);
        item->setText(QStringLiteral("Some screenplay action for the benchmark. ").repeated(20));
        bool alternate = false;
        QBENCHMARK {
            alternate = !alternate;
            item->setAlignment(alternate ? Qt::AlignLeft : Qt::AlignRight);
        }
    }

    void benchmarkEditInLargeScene()
    {
        ScreenplayInformationModel information;
        ScreenplayTextModel model;
        model.setInformationModel(&information);
        std::unique_ptr<TextModelGroupItem> scene(model.createGroupItem(TextGroupType::Scene));
        QVector<AbstractModelItem*> paragraphs;
        for (int i = 0; i < 1000; ++i) {
            auto item = model.createTextItem();
            item->setParagraphType(TextParagraphType::Action);
            item->setText(QStringLiteral("Some screenplay action."));
            paragraphs.append(item);
        }
        scene->appendItems(paragraphs);
        auto edited = static_cast<TextModelTextItem*>(scene->childAt(500));
        bool alternate = false;
        QBENCHMARK {
            alternate = !alternate;
            edited->setText(alternate ? QStringLiteral("One word added.")
                                      : QStringLiteral("One word."));
        }
    }

    void benchmarkTakeRange()
    {
        Item root;
        QVector<AbstractModelItem*> items;
        for (int i = 0; i < 20000; ++i) {
            items.append(new Item);
        }
        root.appendItems(items);
        bool alternate = false;
        QBENCHMARK {
            const auto moved = items.mid(alternate ? 10000 : 0, 10000);
            root.takeItems(0, 9999);
            root.appendItems(moved);
            alternate = !alternate;
        }
        QCOMPARE(root.childCount(), 20000);
    }

    void benchmarkChronometry()
    {
        QFETCH(bool, combined);
        const auto text = QStringLiteral("Some screenplay action for the benchmark. ").repeated(20);
        ChronometerOptions options;
        ScreenplayChronometer::Counters result {};
        QBENCHMARK {
            if (combined) {
                result = ScreenplayChronometer::counters(
                    TextParagraphType::Action, text, {}, options);
            } else {
                result.duration = ScreenplayChronometer::duration(
                    TextParagraphType::Action, text, {}, options);
                result.eighths = ScreenplayChronometer::eighths(
                    TextParagraphType::Action, text, {}, options);
            }
        }
        QVERIFY(result.duration.count() > 0);
        QVERIFY(result.eighths > 0);
    }
};

QTEST_MAIN(ModelPerformanceTest)
#include "tst_model_performance.moc"

#include "legacy_elided_text.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <domain/document_object.h>
#include <management_layer/plugins/screenplay_text_structure/ui/screenplay_text_structure_delegate.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/images_list/images_list_preview.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/tree/tree_view.h>
#include <utils/helpers/image_helper.h>

#include <QDir>
#include <QPainter>
#include <QPixmapCache>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QtTest>

namespace {
QImage render(QWidget& widget)
{
    QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    widget.render(&image);
    return image;
}

class CountingDelegate : public QStyledItemDelegate
{
public:
    mutable QSet<int> rows;
    mutable int paints = 0;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        rows.insert(index.row());
        ++paints;
        QStyledItemDelegate::paint(painter, option, index);
    }
};

class CountingPreview : public Ui::ImagesListPreview
{
public:
    using Ui::ImagesListPreview::ImagesListPreview;
    using Ui::ImagesListPreview::mouseMoveEvent;
    int paints = 0;

protected:
    void paintEvent(QPaintEvent* event) override
    {
        ++paints;
        Ui::ImagesListPreview::paintEvent(event);
    }
};

QPixmap sampleImage(const QColor& color)
{
    QPixmap image(2688, 1800);
    image.fill(color);
    QPainter painter(&image);
    painter.setPen(QPen(Qt::white, 3));
    for (int x = 0; x < image.width(); x += 19) {
        painter.drawLine(x, 0, image.width() - x, image.height());
    }
    return image;
}
} // namespace

class RenderingTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QLoggingCategory::setFilterRules("*.debug=false");
    }

    void elisionPreservesLayout()
    {
        const QStringList texts{ QString(), "Short", QString("A long scene description. ").repeated(40),
                                 "First line\nSecond line\nThird line", "    leading and trailing   ",
                                 QString::fromUtf8("Кириллица и emoji 👩‍💻 — пример текста"),
                                 QString::fromUtf8("مرحبا بالعالم هذا نص عربي طويل للاختبار"),
                                 QString::fromUtf8("中文测试文字日本語の文章"),
                                 QString("W").repeated(250) };
        for (auto direction : { Qt::LeftToRight, Qt::RightToLeft }) {
            QApplication::setLayoutDirection(direction);
            for (int size : { 11, 16, 23 }) {
                QFont font("DejaVu Sans");
                font.setPixelSize(size);
                for (auto width : { 1.0, 80.5, 300.0 }) {
                    for (int lines : { 1, 2, 5 }) {
                        const QRectF rect(0, 0, width, QFontMetricsF(font).lineSpacing() * lines);
                        for (const auto& text : texts) {
                            const auto expected = legacyElidedText(text, font, rect);
                            QCOMPARE(TextHelper::elidedText(text, font, rect), expected);
                            QCOMPARE(TextHelper::elidedText(text, font, rect.translated(12, 50)),
                                     expected);
                        }
                    }
                }
            }
        }
        QApplication::setLayoutDirection(Qt::LeftToRight);
    }

    void shadowCacheSeparatesInputs()
    {
        QPixmapCache::clear();
        QPixmap source(90, 60);
        for (int shape = 0; shape < 3; ++shape) {
            source.fill(Qt::transparent);
            {
                QPainter painter(&source);
                painter.setBrush(QColor(20, 80, 120, shape == 2 ? 80 : 255));
                painter.setPen(Qt::NoPen);
                if (shape == 0) {
                    painter.drawRect(source.rect());
                } else {
                    painter.drawEllipse(source.rect());
                }
            }
            for (auto margins : { QMarginsF(8, 8, 8, 8), QMarginsF(3, 11, 19, 7) }) {
                for (qreal blur : { 4.0, 4.001, 12.0 }) {
                    for (auto color : { QColor(Qt::black), QColor(120, 20, 40, 100) }) {
                        const auto expected = ImageHelper::dropShadow(source, margins, blur, color);
                        QCOMPARE(ImageHelper::dropShadow(source, margins, blur, color, true).toImage(),
                                 expected.toImage());
                        QCOMPARE(ImageHelper::dropShadow(source, margins, blur, color, true).toImage(),
                                 expected.toImage());
                    }
                }
            }
        }
    }

    void roundedShadowsMatchRasterization()
    {
        for (auto size : { QSize(33, 17), QSize(320, 160) }) {
            for (qreal radius : { 0.0, 4.5, 24.0 }) {
                for (auto background : { QColor(Qt::white), QColor(20, 80, 120, 90) }) {
                    QPixmap source(size);
                    source.fill(Qt::transparent);
                    {
                        QPainter painter(&source);
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(background);
                        painter.drawRoundedRect(source.rect(), radius, radius);
                    }
                    const QMarginsF margins(8, 8, 8, 8);
                    const QColor shadow(0, 0, 0, 180);
                    const auto expected = ImageHelper::dropShadow(source, margins, 8, shadow);
                    for (int repeat = 0; repeat < 2; ++repeat) {
                        QCOMPARE(ImageHelper::roundedRectShadow(size, background, radius, margins,
                                                               8, shadow).toImage(),
                                 expected.toImage());
                    }
                    QPixmapCache::clear();
                    QCOMPARE(ImageHelper::roundedRectShadow(size, background, radius, margins,
                                                           8, shadow).toImage(), expected.toImage());
                }
            }
        }
    }

    void shadowsSurviveCachePressure()
    {
        const auto oldLimit = QPixmapCache::cacheLimit();
        QPixmapCache::setCacheLimit(1);
        const auto first = ImageHelper::roundedRectShadow(
            QSize(320, 160), Qt::white, 8, QMarginsF(8, 8, 8, 8), 8, Qt::black);
        const auto second = ImageHelper::roundedRectShadow(
            QSize(320, 160), Qt::white, 8, QMarginsF(8, 8, 8, 8), 8, Qt::black);
        QPixmapCache::setCacheLimit(oldLimit);
        QVERIFY(!first.isNull());
        QCOMPARE(first.toImage(), second.toImage());
    }

    void rippleRepaintsOnlyItsRow()
    {
        QStandardItemModel model(100, 1);
        for (int row = 0; row < model.rowCount(); ++row) {
            model.setData(model.index(row, 0), QString::number(row));
        }
        CountingDelegate delegate;
        TreeView tree;
        tree.setItemDelegate(&delegate);
        tree.setModel(&model);
        tree.resize(500, 600);
        tree.show();
        QTest::qWait(60);
        QTest::mouseClick(tree.viewport(), Qt::LeftButton, Qt::NoModifier,
                          tree.visualRect(model.index(3, 0)).center());
        QTest::qWait(70);
        delegate.rows.clear();
        delegate.paints = 0;
        QTest::qWait(100);
        QVERIFY(delegate.paints > 0);
        QCOMPARE(delegate.rows, QSet<int>{ 3 });
        delegate.rows.clear();
        QTest::mouseClick(tree.viewport(), Qt::LeftButton, Qt::NoModifier,
                          tree.visualRect(model.index(9, 0)).center());
        QTest::qWait(50);
        QVERIFY(delegate.rows.contains(3));
        QVERIFY(delegate.rows.contains(9));
        tree.verticalScrollBar()->setValue(80);
        QTest::qWait(500);
        const auto afterAnimation = tree.viewport()->grab().toImage();
        tree.viewport()->update();
        QTest::qWait(30);
        QCOMPARE(tree.viewport()->grab().toImage(), afterAnimation);
    }

    void previewInvalidatesScaledImage()
    {
        QWidget parent;
        parent.resize(800, 600);
        CountingPreview preview(&parent);
        preview.setImages({ { {}, sampleImage(Qt::red) }, { {}, sampleImage(Qt::blue) } });
        parent.show();
        preview.showPreview(0, QRectF(20, 20, 100, 70));
        QTest::qWait(220);
        QTest::mouseMove(&preview, QPoint(400, 300));
        QTest::qWait(100);
        preview.paints = 0;
        // Call the handler directly to exclude platform expose events from moving the OS cursor.
        QMouseEvent move(QEvent::MouseMove, QPointF(420, 320), Qt::NoButton, Qt::NoButton,
                         Qt::NoModifier);
        preview.mouseMoveEvent(&move);
        QTest::qWait(30);
        QCOMPARE(preview.paints, 0);
        const auto first = render(preview);
        QCOMPARE(render(preview), first);
        QPixmapCache::clear();
        QCOMPARE(render(preview), first);
        preview.showPreview(1);
        const auto second = render(preview);
        QVERIFY(first != second);
        QPixmapCache::clear();
        QCOMPARE(render(preview), second);
        parent.resize(600, 450);
        QTest::qWait(40);
        const auto resized = render(preview);
        QCOMPARE(resized.size(), QSize(600, 450));
        QPixmapCache::clear();
        QCOMPARE(render(preview), resized);
        preview.showPreview(0);
        QVERIFY(render(preview) != resized);
    }

    // Run once against the old library with STARC_WRITE_REFERENCE=1, then against the new one.
    void compareCardReference()
    {
        const auto directory = qEnvironmentVariable("STARC_RENDER_REFERENCE_DIR");
        if (directory.isEmpty()) {
            QSKIP("Set STARC_RENDER_REFERENCE_DIR for before/after pixel comparison");
        }
        QDir().mkpath(directory);
        Card card;
        card.setAttribute(Qt::WA_Hover, false);
        int index = 0;
        for (auto size : { QSize(300, 160), QSize(700, 500) }) {
            card.resize(size);
            for (auto color : { QColor(Qt::white), QColor(40, 70, 120, 100) }) {
                // The old cache aliases shadows of equally sized opaque/translucent cards.
                // Use distinct sizes to compare rasterization independently of that fixed bug.
                card.resize(size + QSize(index % 2, index % 2));
                card.setBackgroundColor(color);
                const auto image = render(card);
                const auto path = directory + QString("/card-%1.png").arg(index++);
                if (qEnvironmentVariableIsSet("STARC_WRITE_REFERENCE")) {
                    QVERIFY(image.save(path));
                } else {
                    QCOMPARE(image, QImage(path).convertToFormat(image.format()));
                }
            }
        }
    }

    void benchmarkSceneDelegate()
    {
        using namespace BusinessLayer;
        QStandardItemModel model(1, 1);
        const auto index = model.index(0, 0);
        model.setData(index, int(TextModelItemType::Group), Qt::UserRole);
        model.setData(index, "INT. ROOM - DAY", TextModelGroupItem::GroupHeadingRole);
        model.setData(index, QString("A character walks into the room and looks around. ").repeated(80),
                      TextModelGroupItem::GroupTextRole);
        model.setData(index, "1", TextModelGroupItem::GroupNumberRole);
        Ui::ScreenplayTextStructureDelegate delegate;
        QImage target(320, 120, QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&target);
        QStyleOptionViewItem option;
        option.rect = target.rect();
        delegate.paint(&painter, option, index);
        QBENCHMARK { delegate.paint(&painter, option, index); }
    }

    void benchmarkCard()
    {
        Card card;
        card.resize(700, 500);
        card.setBackgroundColor(Qt::white);
        QImage target(card.size(), QImage::Format_ARGB32_Premultiplied);
        card.render(&target);
        QBENCHMARK { card.render(&target); }
    }

    void benchmarkButton()
    {
        Button button;
        button.resize(240, 64);
        button.setBackgroundColor(Qt::blue);
        button.setContained(true);
        button.setText("Continue");
        QImage target(button.size(), QImage::Format_ARGB32_Premultiplied);
        button.render(&target);
        QBENCHMARK { button.render(&target); }
    }

    void benchmarkPreview()
    {
        QWidget parent;
        parent.resize(1000, 700);
        Ui::ImagesListPreview preview(&parent);
        preview.setImages({ { {}, sampleImage(Qt::red) } });
        parent.show();
        preview.showPreview(0, QRectF(20, 20, 100, 70));
        QTest::qWait(220);
        QImage target(preview.size(), QImage::Format_ARGB32_Premultiplied);
        preview.render(&target);
        QBENCHMARK { preview.render(&target); }
    }
};

QTEST_MAIN(RenderingTest)
#include "tst_rendering.moc"

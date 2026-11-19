#include <domain/document_object.h>
#include <ui/modules/images_list/images_list.h>
#include <ui/widgets/image_cropper/image_cropper.h>
#include <ui/widgets/text_field/text_field.h>

#include <QDir>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QSignalSpy>
#include <QTextCursor>
#include <QtTest>

namespace {
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

QImage render(QWidget& widget, const QRegion& region = {})
{
    QImage result(widget.size(), QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    widget.render(&result, QPoint(), region);
    return result;
}

QVector<Domain::DocumentImage> sampleImages(int count)
{
    const auto source = sampleImage(Qt::blue);
    QVector<Domain::DocumentImage> images;
    for (int i = 0; i < count; ++i) {
        images.append({ QUuid::createUuid(), source });
    }
    return images;
}

class ObservedImagesList : public Ui::ImagesList
{
public:
    using Ui::ImagesList::mouseMoveEvent;
    QRegion painted;

protected:
    void paintEvent(QPaintEvent* event) override
    {
        painted += event->region();
        Ui::ImagesList::paintEvent(event);
    }
};
} // namespace

class RenderingRound2Test : public QObject
{
    Q_OBJECT

    void compareReference(const QString& name, const QImage& image)
    {
        const auto directory = qEnvironmentVariable("STARC_RENDER_REFERENCE_DIR");
        if (directory.isEmpty()) {
            QSKIP("Set STARC_RENDER_REFERENCE_DIR to compare with the first-round library");
        }
        QDir().mkpath(directory);
        const auto path = directory + '/' + name + ".png";
        if (qEnvironmentVariableIsSet("STARC_WRITE_REFERENCE")) {
            QVERIFY(image.save(path));
        } else {
            const auto expected = QImage(path).convertToFormat(image.format());
            QVERIFY2(!expected.isNull(), qPrintable(path));
            QCOMPARE(image, expected);
        }
    }

private slots:
    void initTestCase()
    {
        QLoggingCategory::setFilterRules("*.debug=false");
        QApplication::setCursorFlashTime(0);
    }

    void cropperReferenceAndInvalidation()
    {
        ImageCropper cropper;
        cropper.resize(800, 600);
        cropper.setImage(sampleImage(Qt::blue));
        const auto original = render(cropper);
        compareReference("cropper-blue", original);
        QCOMPARE(render(cropper), original);
        QPixmapCache::clear();
        QCOMPARE(render(cropper), original);
        cropper.setImage(sampleImage(Qt::red));
        const auto replacement = render(cropper);
        QVERIFY(replacement != original);
        compareReference("cropper-red", replacement);
        cropper.resize(600, 450);
        compareReference("cropper-resized", render(cropper));
        cropper.setProportion(QSizeF(16, 9));
        cropper.setProportionFixed(true);
        compareReference("cropper-fixed-proportion", render(cropper));
        compareReference("cropped-result", cropper.croppedImage().toImage());
        auto highDpiImage = cropper.image();
        highDpiImage.setDevicePixelRatio(2);
        cropper.setImage(highDpiImage);
        compareReference("cropper-high-dpi-source", render(cropper));
        const auto limit = QPixmapCache::cacheLimit();
        QPixmapCache::setCacheLimit(1);
        const auto uncached = render(cropper);
        QPixmapCache::setCacheLimit(limit);
        QCOMPARE(render(cropper), uncached);
        cropper.setImage({});
        compareReference("cropper-empty", render(cropper));
    }

    void textFieldReference_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("password");
        QTest::newRow("empty") << QString() << false;
        QTest::newRow("newline") << QString("\n") << false;
        QTest::newRow("spaces") << QString("   \u00a0") << false;
        QTest::newRow("unicode") << QString::fromUtf8("Текст 👩‍💻 مرحبا") << false;
        QTest::newRow("multiline") << QString("First\nSecond\nThird") << false;
        QTest::newRow("password") << QString::fromUtf8("Пароль 🔑") << true;
    }

    void textFieldReference()
    {
        QFETCH(QString, text);
        QFETCH(bool, password);
        TextField field;
        field.resize(420, 180);
        field.setLabel("Description");
        field.setPlaceholderText("Enter text");
        field.setText(text);
        field.setPasswordModeEnabled(password);
        const QString name = QString("field-%1").arg(QTest::currentDataTag());
        compareReference(name, render(field));
        field.clear();
        compareReference(name + "-cleared", render(field));
    }

    void galleryReference()
    {
        Ui::ImagesList gallery;
        gallery.resize(400, 720);
        gallery.setImageSize(72.5);
        gallery.setImageSpacing(12.5);
        gallery.setImages(sampleImages(21));
        compareReference("gallery-full", render(gallery));
        const QRegion partial = QRegion(QRect(0, 0, 80, 80)) + QRect(84, 84, 82, 82);
        compareReference("gallery-disjoint-update", render(gallery, partial));
        gallery.setImageSize(91.5);
        compareReference("gallery-size-change", render(gallery));
        gallery.resize(500, 720);
        compareReference("gallery-wrap-change", render(gallery));
        gallery.setReadOnly(true);
        compareReference("gallery-read-only", render(gallery));
    }

    void focusedDescendantReference_data()
    {
        QTest::addColumn<bool>("toolWindow");
        QTest::newRow("embedded") << false;
        QTest::newRow("owned-tool-window") << true;
    }

    void focusedDescendantReference()
    {
        QFETCH(bool, toolWindow);
        TextField field;
        field.resize(420, 180);
        field.setLabel("Description");
        QWidget child(&field, toolWindow ? Qt::Tool : Qt::Widget);
        child.setGeometry(20, 80, 180, 50);
        QLineEdit input(&child);
        input.setText("Nested input");
        field.show();
        child.show();
        QApplication::setActiveWindow(toolWindow ? &child : &field);
        input.setFocus();
        QTRY_COMPARE(QApplication::focusWidget(), &input);
        QTest::qWait(300);
        compareReference(QString("focused-descendant-%1").arg(QTest::currentDataTag()), render(field));
    }

    void galleryHoverUpdatesOnlyOneCell()
    {
        ObservedImagesList gallery;
        gallery.resize(400, 500);
        gallery.setImageSize(80);
        gallery.setImageSpacing(16);
        gallery.setImages(sampleImages(5));
        gallery.show();
        QTest::qWait(60);
        QTest::mouseMove(&gallery, QPoint(30, 30));
        QTest::qWait(60);
        gallery.painted = {};
        QTest::qWait(100);
        QVERIFY(!gallery.painted.isEmpty());
        QVERIFY(QRect(0, 0, 82, 82).contains(gallery.painted.boundingRect()));
        QTest::qWait(200);
        gallery.painted = {};
        QMouseEvent move(QEvent::MouseMove, QPointF(40, 40), Qt::NoButton, Qt::NoButton,
                         Qt::NoModifier);
        gallery.mouseMoveEvent(&move);
        QTest::qWait(30);
        QVERIFY(gallery.painted.isEmpty());
        // The add button follows the fifth image on the second row.
        QTest::mouseMove(&gallery, QPoint(125, 125));
        QTest::qWait(60);
        gallery.painted = {};
        QTest::qWait(100);
        QVERIFY(gallery.painted.intersects(QRect(96, 96, 80, 80)));
        QVERIFY(!gallery.painted.intersects(QRect(300, 300, 80, 80)));
    }

    void galleryReordering()
    {
        Ui::ImagesList gallery;
        gallery.resize(500, 300);
        gallery.setImageSize(80);
        gallery.setImageSpacing(16);
        gallery.setImagesReorderingEnabled(true);
        const auto images = sampleImages(3);
        gallery.setImages(images);
        QSignalSpy orderChanged(&gallery, &Ui::ImagesList::imagesOrderChanged);
        gallery.show();
        QTest::qWait(60);
        QTest::mousePress(&gallery, Qt::LeftButton, Qt::NoModifier, QPoint(30, 40));
        QMouseEvent move(QEvent::MouseMove, QPointF(240, 40), Qt::NoButton, Qt::LeftButton,
                         Qt::NoModifier);
        QApplication::sendEvent(&gallery, &move);
        QTest::mouseRelease(&gallery, Qt::LeftButton, Qt::NoModifier, QPoint(240, 40));
        QTRY_COMPARE(orderChanged.count(), 1);
        const auto order = qvariant_cast<QVector<QUuid>>(orderChanged.first().first());
        QCOMPARE(order, QVector<QUuid>({ images[1].uuid, images[2].uuid, images[0].uuid }));
    }

    void benchmarkCropper()
    {
        ImageCropper cropper;
        cropper.resize(1000, 700);
        cropper.setImage(sampleImage(Qt::blue));
        QImage target(cropper.size(), QImage::Format_ARGB32_Premultiplied);
        cropper.render(&target);
        QBENCHMARK { cropper.render(&target); }
    }

    void benchmarkLongTextField()
    {
        TextField field;
        field.resize(600, 300);
        field.setLabel("Description");
        field.setPlaceholderText("Enter text");
        field.setText(QString("A paragraph with a detailed description of the scene.\n").repeated(4000));
        QImage target(field.size(), QImage::Format_ARGB32_Premultiplied);
        field.render(&target);
        QBENCHMARK { field.render(&target); }
    }

    void benchmarkGalleryPartialUpdate()
    {
        Ui::ImagesList gallery;
        gallery.resize(800, 5000);
        gallery.setImageSize(64.5);
        gallery.setImages(sampleImages(300));
        QImage target(80, 80, QImage::Format_ARGB32_Premultiplied);
        const QRegion region(QRect(0, 0, 80, 80));
        gallery.render(&target, QPoint(), region);
        QBENCHMARK { gallery.render(&target, QPoint(), region); }
    }
};

QTEST_MAIN(RenderingRound2Test)
#include "tst_rendering_round_2.moc"

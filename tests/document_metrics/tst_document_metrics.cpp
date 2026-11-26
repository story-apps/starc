#include <3rd_party/fileformats/qtzip/qtzipreader.h>
#include <business_layer/document/text/text_document.h>
#include <business_layer/export/abstract_docx_exporter.h>
#include <business_layer/export/export_options.h>
#include <business_layer/export/screenplay/screenplay_export_options.h>
#include <business_layer/export/screenplay/screenplay_pdf_exporter.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractTextDocumentLayout>
#include <QDataStream>
#include <QDir>
#include <QDomDocument>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QtTest>

#include <memory>

using namespace BusinessLayer;

namespace {
QByteArray geometry(QTextDocument& document)
{
    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly);
    out << document.documentLayout()->documentSize() << document.pageCount();
    for (auto block = document.begin(); block.isValid(); block = block.next()) {
        out << document.documentLayout()->blockBoundingRect(block);
        const auto layout = block.layout();
        out << layout->lineCount();
        for (int i = 0; i < layout->lineCount(); ++i) {
            const auto line = layout->lineAt(i);
            out << line.textStart() << line.textLength() << line.position()
                << line.naturalTextWidth() << line.height();
        }
    }
    return result;
}

class TestDocxExporter : public AbstractDocxExporter
{
public:
    ScreenplayTemplate textTemplate{ ":/templates/screenplay/world_cp" };

protected:
    const TextTemplate& documentTemplate(const ExportOptions&) const override
    {
        return textTemplate;
    }
    QVector<TextParagraphType> paragraphTypes() const override
    {
        return { TextParagraphType::Action };
    }
    TextDocument* createDocument(const ExportOptions&) const override
    {
        return new TextDocument;
    }
    TextDocument* prepareDocument(AbstractModel*, const ExportOptions& options) const override
    {
        auto document = createDocument(options);
        const auto style = textTemplate.paragraphStyle(TextParagraphType::Action);
        QTextCursor cursor(document);
        cursor.setBlockFormat(style.blockFormat());
        cursor.setBlockCharFormat(style.charFormat());
        cursor.insertText("The first paragraph.", style.charFormat());
        auto blockFormat = style.blockFormat();
        blockFormat.setLineHeight(40, QTextBlockFormat::FixedHeight);
        cursor.insertBlock(blockFormat, style.charFormat());
        cursor.insertText("Custom line height.", style.charFormat());
        return document;
    }
};

class TestModel : public ScreenplayTextModel
{
public:
    using ScreenplayTextModel::toXml;
};
} // namespace

class DocumentMetricsTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        for (const auto& name : QDir(":/fonts").entryList(QDir::Files)) {
            QVERIFY2(QFontDatabase::addApplicationFont(":/fonts/" + name) >= 0, qPrintable(name));
        }
    }

    void unitsAndFractionalSizes()
    {
        QCOMPARE(MeasurementHelper::mmToPx(25.4), 96.0);
        QCOMPARE(MeasurementHelper::mmToPx(25.4, false), 96.0);
        QCOMPARE(MeasurementHelper::ptToPx(72), 96.0);
        QCOMPARE(MeasurementHelper::pxToPt(14), 10.5);
        QCOMPARE(MeasurementHelper::mmToTwips(25.4), 1440);
        QCOMPARE(MeasurementHelper::pxToTwips(-16), -240);
        ScreenplayTemplate original(":/templates/screenplay/world_cp");
        auto style = original.paragraphStyle(TextParagraphType::Action);
        auto font = style.font();
        font.setPointSizeF(10.5);
        style.setFont(font);
        original.setParagraphStyle(style);
        ScreenplayTemplate restored;
        restored.load(original.save());
        QCOMPARE(restored.paragraphStyle(TextParagraphType::Action).font().pointSizeF(), 10.5);
        QCOMPARE(restored.paragraphStyle(TextParagraphType::Action).blockFormat().lineHeight(),
                 style.blockFormat().lineHeight());
    }

    void inlineFontRoundTrip()
    {
        std::unique_ptr<Domain::DocumentObject> data(Domain::ObjectsBuilder::createDocument(
            { }, QUuid::createUuid(), Domain::DocumentObjectType::ScreenplayText, { }, { }));
        ScreenplayInformationModel information;
        TestModel model;
        model.setInformationModel(&information);
        model.setDocument(data.get());
        auto item = model.createTextItem();
        item->setParagraphType(TextParagraphType::Action);
        item->setText("A fractional font size");
        model.appendItem(item, model.itemForIndex(model.index(0, 0)));
        QTextLayout::FormatRange range;
        range.start = 0;
        range.length = item->text().size();
        range.format.setFontWeight(QFont::Bold);
        item->setFormats({ range });
        QCOMPARE(item->formats().size(), 1);
        QVERIFY(item->formats().first().isBold);
        QVERIFY(item->formats().first().font.family.isEmpty());
        QCOMPARE(item->formats().first().font.size, 0.0);
        QFont font("Courier Prime");
        font.setPointSizeF(10.5);
        range.format.setFont(font);
        item->setFormats({ range });
        model.updateItem(item);
        const auto xml = model.toXml();
        std::unique_ptr<Domain::DocumentObject> copy(Domain::ObjectsBuilder::createDocument(
            { }, QUuid::createUuid(), Domain::DocumentObjectType::ScreenplayText, xml, { }));
        TestModel restored;
        restored.setInformationModel(&information);
        restored.setDocument(copy.get());
        QCOMPARE(restored.toXml(), xml);
        auto scene = restored.itemForIndex(restored.index(0, 0));
        auto restoredItem
            = static_cast<TextModelTextItem*>(scene->childAt(scene->childCount() - 1));
        const auto formats = restoredItem->formats();
        QVERIFY(!formats.isEmpty());
        QCOMPARE(formats.first().charFormat().font().pointSizeF(), 10.5);
    }

    void helperMatchesDocumentLayout()
    {
        for (const auto& family : { "Courier Prime", "Courier New", "Arial", "Times New Roman" }) {
            QFont font(family);
            font.setPointSizeF(10.5);
            TextHelper::updateFontHinting(font);
            QTextDocument document;
            TextHelper::configureDocument(&document);
            document.setDefaultFont(font);
            document.setDocumentMargin(0);
            document.setTextWidth(300);
            auto option = document.defaultTextOption();
            option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            document.setDefaultTextOption(option);
            const auto lineHeight = TextHelper::fineLineSpacing(font);
            const auto text
                = QString::fromUtf8("Words with accents: café, русский текст. ").repeated(8)
                + QChar::LineSeparator + QString("VeryLongWordWithoutSpaces").repeated(8);
            QTextCursor cursor(&document);
            QTextBlockFormat blockFormat;
            blockFormat.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);
            cursor.setBlockFormat(blockFormat);
            cursor.insertText(text);
            document.documentLayout()->documentSize();
            const auto layout = document.begin().layout();
            QCOMPARE(TextHelper::layoutHeightForWidth(text, font, 300, lineHeight),
                     layout->lineCount() * lineHeight);
            const auto lastLine = layout->lineAt(layout->lineCount() - 1);
            QCOMPARE(TextHelper::lastLineText(text, font, 300),
                     text.mid(lastLine.textStart(), lastLine.textLength()));
        }
    }

    void editorKeepsDocumentMetrics()
    {
        PageTextEdit editor;
        QVERIFY(editor.document()->useDesignMetrics());
        QTextDocument replacement;
        replacement.setPlainText(QString("A line of text. ").repeated(500));
        editor.setDocument(&replacement);
        QVERIFY(replacement.useDesignMetrics());
        QCOMPARE(replacement.documentLayout()->paintDevice()->logicalDpiY(), 96);
        editor.setDocument(nullptr);
        QVERIFY(editor.document()->useDesignMetrics());
    }

    void decorationsUseDocumentSize()
    {
        QFont font("Courier Prime");
        font.setPointSizeF(10.5);
        TextHelper::updateFontHinting(font);
        const auto expected = QFontMetricsF(font, TextHelper::documentPaintDevice())
                                  .horizontalAdvance("MMMMMMMMMM");
        for (int dpi : { 72, 96, 144, 192 }) {
            QImage image(600, 200, QImage::Format_ARGB32_Premultiplied);
            image.setDotsPerMeterX(qRound(dpi / 0.0254));
            image.setDotsPerMeterY(qRound(dpi / 0.0254));
            QPainter painter(&image);
            painter.setFont(TextHelper::fontForPaintDevice(font, &image));
            const auto actual
                = QFontMetricsF(painter.font(), &image).horizontalAdvance("MMMMMMMMMM");
            QVERIFY2(qAbs(actual - expected) < 0.02,
                     qPrintable(QString("DPI %1: %2 != %3").arg(dpi).arg(actual).arg(expected)));
        }
    }

    void docxKeepsComputedSpacing()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        TestDocxExporter exporter;
        auto style = exporter.textTemplate.paragraphStyle(TextParagraphType::Action);
        auto font = style.font();
        font.setPointSizeF(10.5);
        style.setFont(font);
        exporter.textTemplate.setParagraphStyle(style);
        ExportOptions options;
        options.filePath = dir.filePath("document.docx");
        options.includeTitlePage = false;
        exporter.exportTo(nullptr, options);
        QtZipReader zip(options.filePath);
        QDomDocument styles;
        QVERIFY(styles.setContent(zip.fileData("word/styles.xml")));
        const auto spacing = styles.elementsByTagName("w:spacing");
        bool found = false;
        for (int i = 0; i < spacing.size(); ++i) {
            const auto element = spacing.at(i).toElement();
            if (element.attribute("w:lineRule") == "exact") {
                QCOMPARE(element.attribute("w:line").toInt(),
                         MeasurementHelper::pxToTwips(style.blockFormat().lineHeight()));
                found = true;
            }
        }
        QVERIFY(found);
        QVERIFY(zip.fileData("word/styles.xml").contains("<w:sz w:val=\"21\"/>"));
        QDomDocument document;
        QVERIFY(document.setContent(zip.fileData("word/document.xml")));
        const auto directSpacing = document.elementsByTagName("w:spacing");
        QVERIFY(directSpacing.size() >= 2);
        QCOMPARE(directSpacing.at(1).toElement().attribute("w:lineRule"), QString("exact"));
        QCOMPARE(directSpacing.at(1).toElement().attribute("w:line"), QString("600"));
    }

    void pdfExport()
    {
        std::unique_ptr<Domain::DocumentObject> data(Domain::ObjectsBuilder::createDocument(
            { }, QUuid::createUuid(), Domain::DocumentObjectType::ScreenplayText, { }, { }));
        ScreenplayInformationModel information;
        TestModel model;
        model.setInformationModel(&information);
        model.setDocument(data.get());
        auto scene = model.itemForIndex(model.index(0, 0));
        for (int i = 0; i < 100; ++i) {
            auto item = model.createTextItem();
            item->setParagraphType(TextParagraphType::Action);
            item->setText(QString("Paragraph %1: ").arg(i)
                          + QString("The quick brown fox jumps over the lazy dog. ").repeated(5));
            model.appendItem(item, scene);
        }
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ScreenplayExportOptions options;
        options.templateId = "world_cp";
        options.includeTitlePage = false;
        information.setHeader("HEADER");
        information.setFooter("FOOTER");
        options.filePath = qEnvironmentVariable("STARC_METRICS_PDF", dir.filePath("document.pdf"));
        ScreenplayPdfExporter exporter;
        exporter.exportTo(&model, options);
        QFile file(options.filePath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.size() > 1000);
        QVERIFY(file.read(5) == "%PDF-");
    }

    void layoutSnapshot()
    {
        QByteArray result;
        QDataStream out(&result, QIODevice::WriteOnly);
        for (const auto& name : { "world_cp", "world_cn", "ru", "tamil", "he", "ar" }) {
            ScreenplayTemplate textTemplate(":/templates/screenplay/" + QString(name));
            const auto style = textTemplate.paragraphStyle(TextParagraphType::Action);
            TextDocument document;
            document.setDefaultFont(style.font());
            document.setPageSize(
                QSizeF(MeasurementHelper::mmToPx(155), MeasurementHelper::mmToPx(249)));
            document.setDocumentMargin(0);
            QTextCursor cursor(&document);
            const QString text
                = QString::fromUtf8("A long line, русский текст, العربية עברית தமிழ். ").repeated(6);
            for (int i = 0; i < 50; ++i) {
                if (i)
                    cursor.insertBlock();
                cursor.setBlockFormat(style.blockFormat());
                cursor.setBlockCharFormat(style.charFormat());
                cursor.insertText(text, style.charFormat());
            }
            QVERIFY(document.pageCount() > 1);
            out << geometry(document);
            const auto initial = geometry(document);
            for (qreal zoom : { 0.75, 1.0, 1.5, 2.0 }) {
                QImage image(600, 600, QImage::Format_ARGB32_Premultiplied);
                QPainter painter(&image);
                painter.scale(zoom, zoom);
                document.drawContents(&painter, QRectF(0, 0, 300, 300));
                QCOMPARE(geometry(document), initial);
            }
            const auto height = TextHelper::layoutHeightForWidth(text, style.font(), 300, -1);
            out << height << TextHelper::lastLineText(text, style.font(), 300);
        }
        const auto path = qEnvironmentVariable("STARC_METRICS_SNAPSHOT");
        if (!path.isEmpty()) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(result), qint64(result.size()));
        }
    }

    void layoutDoesNotDependOnScreen()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QByteArray baseline;
        for (const auto& configuration : { "96:1", "120:1.25", "144:1.5", "192:2" }) {
            const auto values = QString(configuration).split(':');
            QProcess child;
            auto env = QProcessEnvironment::systemEnvironment();
            env.insert("QT_FONT_DPI", values[0]);
            env.insert("QT_SCALE_FACTOR", values[1]);
            env.insert("STARC_METRICS_SNAPSHOT", dir.filePath("layout.bin"));
            child.setProcessEnvironment(env);
            child.start(QCoreApplication::applicationFilePath(), { "layoutSnapshot" });
            QVERIFY(child.waitForFinished(60000));
            QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
                     child.readAllStandardOutput().constData());
            QFile file(dir.filePath("layout.bin"));
            QVERIFY(file.open(QIODevice::ReadOnly));
            const auto current = file.readAll();
            QVERIFY(!current.isEmpty());
            if (baseline.isEmpty())
                baseline = current;
            else
                QCOMPARE(current, baseline);
        }
    }
};

QTEST_MAIN(DocumentMetricsTest)
#include "tst_document_metrics.moc"

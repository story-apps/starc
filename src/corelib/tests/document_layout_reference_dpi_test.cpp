#include <utils/helpers/document_layout_config.h>
#include <utils/helpers/document_units.h>

#include <QFont>
#include <QFontMetricsF>
#include <QTextDocument>
#include <QtTest>

class DocumentLayoutReferenceDpiTest : public QObject
{
    Q_OBJECT

private slots:
    void conversions()
    {
        QVERIFY(qAbs(DocumentUnits::mmToDu(25.4) - 96.0) < 0.00001);
        QVERIFY(qAbs(DocumentUnits::ptToDu(12.0) - 16.0) < 0.00001);
        QVERIFY(qAbs(DocumentUnits::duToMm(96.0) - 25.4) < 0.00001);
    }

    void documentDevice()
    {
        QTextDocument doc;
        DocumentLayoutConfig::configure(&doc);
        QVERIFY(doc.useDesignMetrics());
        QCOMPARE(doc.documentLayout()->paintDevice()->logicalDpiX(), 96);
        QCOMPARE(doc.documentLayout()->paintDevice()->logicalDpiY(), 96);
    }

    void pageGeometry()
    {
        QTextDocument doc;
        DocumentLayoutConfig::configure(&doc);
        doc.setDocumentMargin(0);
        doc.setPageSize(DocumentUnits::mmToDu(QSizeF(210, 297)));
        QVERIFY(qAbs(DocumentUnits::duToMm(doc.pageSize().width()) - 210) < 0.001);
        QVERIFY(qAbs(DocumentUnits::duToMm(doc.pageSize().height()) - 297) < 0.001);
    }
};

QTEST_MAIN(DocumentLayoutReferenceDpiTest)
#include "document_layout_reference_dpi_test.moc"

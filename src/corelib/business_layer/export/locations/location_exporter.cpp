#include "location_exporter.h"

#include "location_export_options.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QPainter>
#include <QPainterPath>


namespace BusinessLayer {

TextDocument* LocationExporter::createDocument(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)
    return new SimpleTextDocument;
}

const TextTemplate& LocationExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)
    return TemplatesFacade::simpleTextTemplate();
}

TextDocument* LocationExporter::prepareDocument(AbstractModel* _model,
                                                const ExportOptions& _exportOptions) const
{

    //
    // Настраиваем документ
    //
    PageTextEdit textEdit;
    textEdit.setUsePageMode(true);
    textEdit.setPageSpacing(0);
    auto textDocument = createDocument(_exportOptions);
    textEdit.setDocument(textDocument);
    //
    // ... параметры страницы
    //
    const auto& exportTemplate = documentTemplate(_exportOptions);
    textEdit.setPageFormat(exportTemplate.pageSizeId());
    textEdit.setPageMarginsMm(exportTemplate.pageMargins());
    textEdit.setPageNumbersAlignment(exportTemplate.pageNumbersAlignment());
    //
    // ... начинаем вставку данных в документ
    //
    TextCursor cursor(textDocument);
    cursor.beginEditBlock();

    //
    // Работаем от стандартного шрифта шаблона
    //
    auto baseFont = exportTemplate.baseFont();
    baseFont.setPixelSize(MeasurementHelper::ptToPx(12));
    auto titleFont = baseFont;
    titleFont.setPixelSize(MeasurementHelper::ptToPx(20));
    titleFont.setBold(true);
    auto headingFont = baseFont;
    headingFont.setPixelSize(MeasurementHelper::ptToPx(16));
    headingFont.setBold(true);

    const auto location = qobject_cast<LocationModel*>(_model);
    auto blockFormat = [](qreal _topMargin = 0.0, qreal _bottomMargin = 0.0) {
        QTextBlockFormat format;
        format.setTopMargin(_topMargin);
        format.setBottomMargin(_bottomMargin);
        return format;
    };
    auto charFormat = [](const QFont& _font, const QColor& _textColor = {},
                         const QColor& _backgroundColor = {}) {
        QTextCharFormat format;
        format.setFont(_font);
        if (_textColor.isValid()) {
            format.setForeground(_textColor);
        }
        if (_backgroundColor.isValid()) {
            format.setBackground(_backgroundColor);
        }
        return format;
    };

    const auto& exportOptions = static_cast<const LocationExportOptions&>(_exportOptions);

    //
    // Фотка локации
    //
    if (exportOptions.includeMainPhoto && !location->mainPhoto().image.isNull()) {
        const auto mainPhotoScaled = location->mainPhoto().image.scaled(
            MeasurementHelper::mmToPx(40), MeasurementHelper::mmToPx(40),
            Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const auto borderMargin = MeasurementHelper::mmToPx(3);
        QImage mainPhoto(mainPhotoScaled.size() + QSize(borderMargin, borderMargin),
                         QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&mainPhoto);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(mainPhoto.rect(), Qt::white);
        const auto targetRect
            = mainPhoto.rect().marginsRemoved(QMargins(borderMargin, 0, 0, borderMargin));
        QPainterPath clipPath;
        clipPath.addRoundedRect(targetRect, borderMargin, borderMargin);
        painter.setClipPath(clipPath);
        painter.drawPixmap(targetRect, mainPhotoScaled, mainPhotoScaled.rect());
        const QString imageName = "location_photo";
        textDocument->addResource(QTextDocument::ImageResource, QUrl(imageName), mainPhoto);
        QTextImageFormat format;
        format.setName(imageName);
        cursor.insertImage(format, QTextFrameFormat::FloatRight);
    }
    //
    // Роль локации
    //
    QString roleName;
    {
        switch (location->storyRole()) {
        case LocationStoryRole::Primary: {
            roleName = QCoreApplication::translate("BusinessLayer::LocationExporter",
                                                   "primary location");
            break;
        }
        case LocationStoryRole::Secondary: {
            roleName = QCoreApplication::translate("BusinessLayer::LocationExporter",
                                                   "secondary location");
            break;
        }
        case LocationStoryRole::Tertiary: {
            roleName = QCoreApplication::translate("BusinessLayer::LocationExporter",
                                                   "tertiary location");
            break;
        }
        default: {
            break;
        }
        }
        if (!roleName.isEmpty()) {
            const auto roleNameFormat = charFormat(baseFont, Ui::DesignSystem::color().onAccent(),
                                                   Ui::DesignSystem::color().accent());
            cursor.insertText(" " + roleName + " ", roleNameFormat);
        }
    }
    //
    // Название
    //
    const auto titleFormat = charFormat(titleFont);
    if (roleName.isEmpty()) {
        cursor.setCharFormat(titleFormat);
    } else {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(4)), titleFormat);
    }
    cursor.insertText(location->name());
    //
    // Краткое описание
    //
    const auto baseFormat = charFormat(baseFont);
    if (!location->oneSentenceDescription().isEmpty()) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(4)), baseFormat);
        cursor.insertText(location->oneSentenceDescription());
    }
    //
    // Детальное описание
    //
    if (!location->longDescription().isEmpty()) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(6)), baseFormat);
        cursor.insertText(location->longDescription());
    }
    //
    // Блоки описаний
    //
    const auto headingFormat = charFormat(headingFont);
    auto addHeader = [&cursor, &blockFormat, &headingFormat](const QString& _heading) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(24)), headingFormat);
        cursor.insertText(_heading);
    };
    auto baseBoldFormat = charFormat(baseFont);
    baseBoldFormat.setFontWeight(QFont::Bold);
    auto addField = [&cursor, &blockFormat, &baseFormat, &baseBoldFormat](const QString& _title,
                                                                          const QString& _value) {
        if (_value.isEmpty()) {
            return;
        }

        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(6)), baseBoldFormat);
        cursor.insertText(_title);
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(2)), baseFormat);
        cursor.insertText(_value);
    };
    // clang-format off
    if (exportOptions.includeSenseInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::LocationExporter", "Sense info"));
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Sight"), location->sight());
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Smell"), location->smell());
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Sound"), location->sound());
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Taste"), location->taste());
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Touch"), location->touch());
    }
    //
    if (exportOptions.includeGeographyInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::LocationExporter", "Geography info"));
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Location"), location->location());
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Climate"), location->climate());
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Landmark"), location->landmark());
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "Nearby places"), location->nearbyPlaces());
    }
    if (exportOptions.includeBackgroundInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::LocationExporter", "Background info"));
        addField(QCoreApplication::translate("BusinessLayer::LocationExporter", "History"), location->history());
    }
    // clang-format on
    if (exportOptions.includeAdditionalPhotos && location->photos().size() > 1) {
        addHeader(
            QCoreApplication::translate("BusinessLayer::LocationExporter", "Additional photos"));
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(6)), baseFormat);
        const auto borderMargin = MeasurementHelper::mmToPx(3);
        for (int index = 1; index < location->photos().size(); ++index) {
            const auto photoScaled = location->photos().value(index).image.scaled(
                MeasurementHelper::mmToPx(40), MeasurementHelper::mmToPx(40),
                Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            QImage mainPhoto(QSize(MeasurementHelper::mmToPx(40), MeasurementHelper::mmToPx(40))
                                 + QSize(borderMargin, borderMargin),
                             QImage::Format_ARGB32_Premultiplied);
            QPainter painter(&mainPhoto);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.fillRect(mainPhoto.rect(), Qt::white);
            const auto targetRect
                = mainPhoto.rect().marginsRemoved(QMargins(0, borderMargin, borderMargin, 0));
            QPainterPath clipPath;
            clipPath.addRoundedRect(targetRect, borderMargin, borderMargin);
            painter.setClipPath(clipPath);
            painter.drawPixmap(
                targetRect, photoScaled,
                QRect(0, 0, MeasurementHelper::mmToPx(40), MeasurementHelper::mmToPx(40)));
            const QString imageName = "location_photo_" + QString::number(index);
            textDocument->addResource(QTextDocument::ImageResource, QUrl(imageName), mainPhoto);
            QTextImageFormat format;
            format.setName(imageName);
            cursor.insertImage(format, QTextFrameFormat::InFlow);
        }
    }

    //
    // ... заканчиваем формирование документа
    //
    cursor.endEditBlock();

    return textDocument;
}

} // namespace BusinessLayer

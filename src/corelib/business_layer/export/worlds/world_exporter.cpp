#include "world_exporter.h"

#include "world_export_options.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/worlds/world_model.h>
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
#include <QTextBlock>


namespace BusinessLayer {

TextDocument* WorldExporter::createDocument(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)
    return new SimpleTextDocument;
}

const TextTemplate& WorldExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)
    return TemplatesFacade::simpleTextTemplate();
}

TextDocument* WorldExporter::prepareDocument(AbstractModel* _model,
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
    headingFont.setPixelSize(MeasurementHelper::ptToPx(18));
    headingFont.setBold(true);
    auto subHeadingFont = baseFont;
    subHeadingFont.setPixelSize(MeasurementHelper::ptToPx(16));
    subHeadingFont.setBold(true);
    auto subSubHeadingFont = baseFont;
    subSubHeadingFont.setPixelSize(MeasurementHelper::ptToPx(14));
    subSubHeadingFont.setBold(true);

    const auto world = qobject_cast<WorldModel*>(_model);
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

    const auto& exportOptions = static_cast<const WorldExportOptions&>(_exportOptions);

    //
    // Фотка мира
    //
    if (exportOptions.includeMainPhoto && !world->mainPhoto().image.isNull()) {
        const auto mainPhotoScaled = world->mainPhoto().image.scaled(
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
        const QString imageName = "world_photo";
        textDocument->addResource(QTextDocument::ImageResource, QUrl(imageName), mainPhoto);
        QTextImageFormat format;
        format.setName(imageName);
        cursor.insertImage(format, QTextFrameFormat::FloatRight);
    }
    //
    // Название
    //
    const auto titleFormat = charFormat(titleFont);
    cursor.insertText(world->name(), titleFormat);
    //
    // Краткое описание
    //
    const auto baseFormat = charFormat(baseFont);
    if (!world->oneSentenceDescription().isEmpty()) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(4)), baseFormat);
        cursor.insertText(world->oneSentenceDescription());
    }
    //
    // Детальное описание
    //
    if (!world->longDescription().isEmpty()) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(6)), baseFormat);
        cursor.insertText(world->longDescription());
    }
    //
    // Блоки описаний
    //
    const auto headingFormat = charFormat(headingFont);
    auto addHeader = [&cursor, &blockFormat, &headingFormat](const QString& _heading) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(24)), headingFormat);
        cursor.insertText(_heading);
    };
    const auto subHeadingFormat = charFormat(subHeadingFont);
    auto addSubHeader = [&cursor, &blockFormat, &subHeadingFormat](const QString& _subHeading) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(10), MeasurementHelper::ptToPx(2)),
                           subHeadingFormat);
        cursor.insertText(_subHeading);
    };
    const auto subSubHeadingFormat = charFormat(subSubHeadingFont);
    auto addSubSubHeader
        = [&cursor, &blockFormat, &subSubHeadingFormat](const QString& _subHeading) {
              cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(10)), subSubHeadingFormat);
              cursor.insertText(_subHeading);
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
    auto addWorldItems = [&textEdit, &textDocument, &cursor, &blockFormat, &baseFormat,
                          &addSubHeader, &addSubSubHeader,
                          &addField](const QString& _header, const QVector<WorldItem>& _items) {
        if (_items.isEmpty()) {
            return;
        }

        //
        // Тут завершаем транзакцию, т.к. нужны корректные области курсора после вставки изображений
        //
        cursor.endEditBlock();

        addSubHeader(_header);
        int worldItemStartPosition = 0;
        int worldItemEndPosition = 0;
        for (int index = 0; index < _items.size(); ++index) {
            const auto& race = _items.at(index);

            cursor.beginEditBlock();

            addSubSubHeader(race.name);
            worldItemStartPosition = cursor.position() - race.name.length();
            //
            int photoHeight = 0;
            if (!race.photo.image.isNull()) {
                const auto photoScaled = race.photo.image.scaled(
                    MeasurementHelper::mmToPx(20), MeasurementHelper::mmToPx(20),
                    Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                const auto borderMargin = MeasurementHelper::mmToPx(3);
                QImage photo(photoScaled.size() + QSize(borderMargin, borderMargin),
                             QImage::Format_ARGB32_Premultiplied);
                QPainter painter(&photo);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.fillRect(photo.rect(), Qt::white);
                const auto targetRect
                    = photo.rect().marginsRemoved(QMargins(borderMargin, 0, 0, borderMargin));
                QPainterPath clipPath;
                clipPath.addRoundedRect(targetRect, borderMargin, borderMargin);
                painter.setClipPath(clipPath);
                painter.drawPixmap(targetRect, photoScaled, photoScaled.rect());
                const QString imageName = _header + "_photo_" + QString::number(index);
                textDocument->addResource(QTextDocument::ImageResource, QUrl(imageName), photo);
                QTextImageFormat format;
                format.setName(imageName);
                cursor.insertImage(format, QTextFrameFormat::FloatRight);

                photoHeight = photo.height();
            }
            addField(QCoreApplication::translate("BusinessLayer::WorldExporter",
                                                 "One sentence description"),
                     race.oneSentenceDescription);
            addField(
                QCoreApplication::translate("BusinessLayer::WorldExporter", "Long description"),
                race.longDescription);

            //
            // Если после картинки не было добавлено ни одного блока, то сделаем это, чтобы
            // корректно высчитать высоту недостающего контента и сделать правильный отступ между
            // персонажами, иначе Qt считает, что позиция курсора в блоке выше, чем реальная позиция
            //
            if (photoHeight > 0 && worldItemStartPosition == cursor.block().position()) {
                cursor.insertBlock(blockFormat(), baseFormat);
                cursor.insertText(" ");
            }

            cursor.endEditBlock();
            worldItemEndPosition = cursor.position();

            const auto worldItemHeight = textEdit.cursorRectAt(worldItemEndPosition).bottom()
                - textEdit.cursorRectAt(worldItemStartPosition).top();
            if (photoHeight > 0 && photoHeight > worldItemHeight) {
                cursor.insertBlock(blockFormat(photoHeight - worldItemHeight));
            }
        }

        //
        // Возобновляем транзакцию
        //
        cursor.beginEditBlock();
    };
    // clang-format off
    if (exportOptions.includeWorldDescriptionInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "World description"));
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Overview"), world->overview());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Earth like?"), world->earthLike());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "World history"), world->history());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "World mood"), world->mood());
    }
    //
    if (exportOptions.includeNatureInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "Nature"));
        addSubHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "Basic info"));
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Biology"), world->biology());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Physics"), world->physics());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Astronomy"), world->astronomy());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Geography"), world->geography());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Races"), world->races());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Flora"), world->floras());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Animals"), world->animals());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Natural resources"), world->naturalResources());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Climate"), world->climates());
    }
    if (exportOptions.includeCultureInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "Culture"));
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Religions and beliefs"), world->religions());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Ethics and values"), world->ethics());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Languages"), world->languages());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Class/caste system"), world->castes());
    }
    if (exportOptions.includeSystemInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "System info"));
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Technology"), world->technology());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Economy"), world->economy());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Trade"), world->trade());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Business"), world->business());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Industry"), world->industry());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Currency"), world->currency());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Education"), world->education());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Communication method"), world->communication());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Art"), world->art());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Entertainment"), world->entertainment());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Travel"), world->travel());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Science"), world->science());
    }
    if (exportOptions.includePoliticsInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "Politics"));
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Government format"), world->governmentFormat());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Government history"), world->governmentHistory());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Laws and justice system"), world->laws());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Foreign relations"), world->foreignRelations());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "People perception of government"), world->perceptionOfGovernment());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Propaganda"), world->propaganda());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Anti-government organisations"), world->antiGovernmentOrganisations());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Past war"), world->pastWar());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Current war"), world->currentWar());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Potential war"), world->potentialWar());
    }
    if (exportOptions.includeMagicInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "Magic"));
        addSubHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "Basic info"));
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Magic rule"), world->magicRule());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Who can use"), world->whoCanUse());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Effect to world"), world->effectToWorld());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Effect to society"), world->effectToSociety());
        addField(QCoreApplication::translate("BusinessLayer::WorldExporter", "Effect to technology"), world->effectToTechnology());
        addWorldItems(QCoreApplication::translate("BusinessLayer::WorldExporter", "Magic types"), world->magicTypes());
    }
    // clang-format on
    if (exportOptions.includeAdditionalPhotos && world->photos().size() > 1) {
        addHeader(QCoreApplication::translate("BusinessLayer::WorldExporter", "Additional photos"));
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(6)), baseFormat);
        const auto borderMargin = MeasurementHelper::mmToPx(3);
        for (int index = 1; index < world->photos().size(); ++index) {
            const auto photoScaled = world->photos().value(index).image.scaled(
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
            const QString imageName = "world_photo_" + QString::number(index);
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

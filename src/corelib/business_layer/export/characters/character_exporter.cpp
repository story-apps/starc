#include "character_exporter.h"

#include "character_export_options.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/characters/character_model.h>
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

TextDocument* CharacterExporter::createDocument(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)
    return new SimpleTextDocument;
}

const TextTemplate& CharacterExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)
    return TemplatesFacade::simpleTextTemplate();
}

TextDocument* CharacterExporter::prepareDocument(AbstractModel* _model,
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

    const auto character = qobject_cast<CharacterModel*>(_model);
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

    const auto& exportOptions = static_cast<const CharacterExportOptions&>(_exportOptions);

    //
    // Фотка персонажа
    //
    if (exportOptions.includeMainPhoto && !character->mainPhoto().image.isNull()) {
        const auto mainPhotoScaled = character->mainPhoto().image.scaled(
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
        const QString imageName = "character_photo";
        textDocument->addResource(QTextDocument::ImageResource, QUrl(imageName), mainPhoto);
        QTextImageFormat format;
        format.setName(imageName);
        cursor.insertImage(format, QTextFrameFormat::FloatRight);
    }
    //
    // Роль персонажа
    //
    QString roleName;
    {
        switch (character->storyRole()) {
        case CharacterStoryRole::Primary: {
            roleName = QCoreApplication::translate("BusinessLayer::CharacterExporter",
                                                   "primary character");
            break;
        }
        case CharacterStoryRole::Secondary: {
            roleName = QCoreApplication::translate("BusinessLayer::CharacterExporter",
                                                   "secondary character");
            break;
        }
        case CharacterStoryRole::Tertiary: {
            roleName = QCoreApplication::translate("BusinessLayer::CharacterExporter",
                                                   "tertiary character");
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
    // Имя (возраст, пол)
    //
    const auto titleFormat = charFormat(titleFont);
    if (roleName.isEmpty()) {
        cursor.setCharFormat(titleFormat);
    } else {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(4)), titleFormat);
    }
    cursor.insertText(character->name());
    {
        QString basicInfo;
        //
        switch (character->gender()) {
        case 0: {
            basicInfo += QCoreApplication::translate("BusinessLayer::CharacterExporter", "Male");
            break;
        }
        case 1: {
            basicInfo += QCoreApplication::translate("BusinessLayer::CharacterExporter", "Female");
            break;
        }
        case 2: {
            basicInfo += QCoreApplication::translate("BusinessLayer::CharacterExporter", "Other");
            break;
        }
        default: {
            break;
        }
        }
        //
        if (!character->age().isEmpty()) {
            if (!basicInfo.isEmpty()) {
                basicInfo.prepend(", ");
            }
            basicInfo.prepend(character->age());
        }
        //
        //
        if (!basicInfo.isEmpty()) {
            cursor.insertText(" (" + basicInfo + ")");
        }
    }
    //
    // Краткое описание
    //
    const auto baseFormat = charFormat(baseFont);
    if (!character->oneSentenceDescription().isEmpty()) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(4)), baseFormat);
        cursor.insertText(character->oneSentenceDescription());
    }
    //
    // Детальное описание
    //
    if (!character->longDescription().isEmpty()) {
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(6)), baseFormat);
        cursor.insertText(character->longDescription());
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
    if (exportOptions.includeStoryInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Story info"));
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Short term goal"), character->shortTermGoal());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Long term goal"), character->longTermGoal());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Initial beliefs"), character->initialBeliefs());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Changed beliefs"), character->changedBeliefs());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "What leads to change"), character->whatLeadsToChange());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "First appearance"), character->firstAppearance());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Plot involvement"), character->plotInvolvement());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Conflict"), character->conflict());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Most defining moment"), character->mostDefiningMoment());
    }
    //
    if (exportOptions.includePersonalInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Personal info"));
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Real name"), character->nickname());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Date of birth"), character->dateOfBirth());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Place of birth"), character->placeOfBirth());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Ethnicity/race"), character->ethnicity());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Family"), character->family());
    }
    if (exportOptions.includePhysiqueInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Physique info"));
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Height"), character->height());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Weight"), character->weight());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Body"), character->body());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Skin tone"), character->skinTone());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Hair style"), character->hairStyle());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Hair color"), character->hairColor());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Eye shape"), character->eyeShape());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Eye color"), character->eyeColor());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Facial shape"), character->facialShape());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Distinguish feature"), character->distinguishFeature());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Other facial features"), character->otherFacialFeatures());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Posture"), character->posture());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Other physical appearance"), character->otherPhysicalAppearance());
    }
    if (exportOptions.includeLifeInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Life info"));
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Skills"), character->skills());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "How skills developed"), character->howItDeveloped());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Incompetence"), character->incompetence());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Strength/talent"), character->strength());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Weakness"), character->weakness());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Hobbies"), character->hobbies());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Habits"), character->habits());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Health"), character->health());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Speech"), character->speech());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Pet"), character->pet());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Dress"), character->dress());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Something always carried/weapon/tools"), character->somethingAlwaysCarried());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Accessories"), character->accessories());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Area of residence/environment"), character->areaOfResidence());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Home description"), character->homeDescription());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Neighborhood"), character->neighborhood());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Organization involved"), character->organizationInvolved());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Income"), character->income());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Job/occupation"), character->jobOccupation());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Job rank/position"), character->jobRank());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Job satisfaction"), character->jobSatisfaction());
    }
    if (exportOptions.includeAttitudeInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Attitude info"));
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Personality"), character->personality());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Moral"), character->moral());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Motivation"), character->motivation());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Discouragement"), character->discouragement());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Philosophy"), character->philosophy());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Greatest fear"), character->greatestFear());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Self control"), character->selfControl());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Intelligence level"), character->intelligenceLevel());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Confidence level"), character->confidenceLevel());
    }
    if (exportOptions.includeBiographyInfo) {
        addHeader(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Biography info"));
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Childhood"), character->childhood());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Important past event"), character->importantPastEvent());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Best accomplishment"), character->bestAccomplishment());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Other accomplishment"), character->otherAccomplishment());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Worst moment"), character->worstMoment());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Failure"), character->failure());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Secrets"), character->secrets());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Best memories"), character->bestMemories());
        addField(QCoreApplication::translate("BusinessLayer::CharacterExporter", "Worst memories"), character->worstMemories());
    }
    // clang-format on
    if (exportOptions.includeAdditionalPhotos && character->photos().size() > 1) {
        addHeader(
            QCoreApplication::translate("BusinessLayer::CharacterExporter", "Additional photos"));
        cursor.insertBlock(blockFormat(MeasurementHelper::ptToPx(6)), baseFormat);
        const auto borderMargin = MeasurementHelper::mmToPx(3);
        for (int index = 1; index < character->photos().size(); ++index) {
            const auto photoScaled = character->photos().value(index).image.scaled(
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
            const QString imageName = "character_photo_" + QString::number(index);
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

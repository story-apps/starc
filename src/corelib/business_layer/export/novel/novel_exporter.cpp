#include "novel_exporter.h"

#include "novel_export_options.h"

#include <business_layer/document/novel/text/novel_text_document.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>


namespace BusinessLayer {

TextDocument* NovelExporter::createDocument(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)

    auto document = new NovelTextDocument;
    document->setCorrectionOptions(
        settingsValue(DataStorageLayer::kComponentsNovelEditorCorrectTextOnPageBreaksKey).toBool());
    return document;
}

const TextTemplate& NovelExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    return TemplatesFacade::novelTemplate(_exportOptions.templateId);
}

bool NovelExporter::prepareBlock(const ExportOptions& _exportOptions, TextCursor& _cursor) const
{
    const auto& exportOptions = static_cast<const NovelExportOptions&>(_exportOptions);

    //
    // Убираем блоки завершений частей и глав
    //
    if (const auto blockType = TextBlockStyle::forBlock(_cursor); !exportOptions.includeFooters
        && (blockType == TextParagraphType::PartFooter
            || blockType == TextParagraphType::ChapterFooter)) {
        _cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (_cursor.hasSelection()) {
            _cursor.deleteChar();
        }
        _cursor.movePosition(!_cursor.atEnd() ? QTextCursor::NextCharacter
                                              : QTextCursor::PreviousCharacter,
                             QTextCursor::KeepAnchor);
        _cursor.deleteChar();
        return true;
    }

    //
    // Если нужно использовать декоративные разрывы вместо заголовков сцен
    //
    if (!exportOptions.ornamentalBreak.isEmpty()) {
        switch (TextBlockStyle::forBlock(_cursor)) {
        case TextParagraphType::PartHeading:
        case TextParagraphType::PartFooter:
        case TextParagraphType::ChapterHeading:
        case TextParagraphType::ChapterFooter: {
            m_isFisrtSceneHeader = true;
            break;
        }

        case TextParagraphType::SceneHeading: {
            //
            // ... очищаем текст заголовка сцены
            //
            _cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (_cursor.hasSelection()) {
                _cursor.deleteChar();
            }
            //
            // ... если это первый заголовок, то удалим блок
            //
            if (m_isFisrtSceneHeader) {
                m_isFisrtSceneHeader = false;
                _cursor.movePosition(!_cursor.atEnd() ? QTextCursor::NextCharacter
                                                      : QTextCursor::PreviousCharacter,
                                     QTextCursor::KeepAnchor);
                _cursor.deleteChar();
                return true;
            }
            //
            // ... а если не первый, то вставляем декорацию
            //
            _cursor.insertText(exportOptions.ornamentalBreak);
            break;
        }

        default: {
            break;
        }
        }
    }

    return false;
}
} // namespace BusinessLayer

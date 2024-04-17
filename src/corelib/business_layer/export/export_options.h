#pragma once

#include <QColor>
#include <QHash>

#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Формат экспортируемого файла
 */
enum class ExportFileFormat {
    Pdf,
    Docx,
    Fdx,
    Fountain,
    Markdown,
};

/**
 * @brief Опции экспорта
 */
struct CORE_LIBRARY_EXPORT ExportOptions {
    /**
     * @brief Путь к файлу
     */
    QString filePath;

    /**
     * @brief Формат файла
     */
    ExportFileFormat fileFormat = ExportFileFormat::Pdf;

    /**
     * @brief Идентификатор шаблона экспорта
     */
    QString templateId;

    /**
     * @brief Печатать титульную страницу
     */
    bool includeTiltePage = true;

    /**
     * @brief Печатать синопсис
     */
    bool includeSynopsis = false;

    /**
     * @brief Печатать основной текст документа
     */
    bool includeText = true;

    /**
     * @brief Печатать ли блоки папок
     */
    bool includeFolders = true;

    /**
     * @brief Печатать ли комментарии по тексту
     */
    bool includeInlineNotes = false;

    /**
     * @brief Печатать редакторские пометки
     */
    bool includeReviewMarks = true;

    /**
     * @brief Необходимо ли подсвечивать персонажей
     */
    bool highlightCharacters = false;
    bool highlightCharactersWithDialogues = false;

    /**
     * @brief Каких персонажей и какими цветами подсвечивать
     */
    QHash<QString, QColor> highlightCharactersList;

    /**
     * @brief Водяной знак
     */
    QString watermark;
    /**
     * @brief Цвет водяного знака
     */
    QColor watermarkColor;


    //
    // Параметры самого документа
    //

    /**
     * @brief Верхний колонтитул
     */
    QString header;

    /**
     * @brief Печатать верхний колонтитул на титульной странице
     */
    bool printHeaderOnTitlePage = false;

    /**
     * @brief Нижний колонтитул
     */
    QString footer;

    /**
     * @brief Печатать нижий колонтитул на титульной странице
     */
    bool printFooterOnTitlePage = false;
};

} // namespace BusinessLayer

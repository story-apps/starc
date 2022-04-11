#pragma once

#include <QColor>
#include <QString>

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

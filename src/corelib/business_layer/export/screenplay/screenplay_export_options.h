#pragma once

#include <QColor>
#include <QString>

#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Формат экспортируемого файла
 */
enum class ScreenplayExportFileFormat {
    Pdf,
    Docx,
    Fdx,
    Fountain,
};

/**
 * @brief Опции экспорта
 */
struct CORE_LIBRARY_EXPORT ScreenplayExportOptions {
    /**
     * @brief Путь к файлу
     */
    QString filePath;

    /**
     * @brief Формат файла
     */
    ScreenplayExportFileFormat fileFormat = ScreenplayExportFileFormat::Pdf;

    /**
     * @brief Идентификатор шаблона экспорта
     */
    QString templateId;

    /**
     * @brief Печатать титульную страницу
     */
    bool printTiltePage = true;

    /**
     * @brief Печатать ли блоки папок
     */
    bool printFolders = true;

    /**
     * @brief Печатать ли комментарии по тексту
     */
    bool printInlineNotes = false;

    /**
     * @brief Печатать номера сцен
     */
    bool printScenesNumbers = true;
    bool printScenesNumbersOnLeft = true;
    bool printScenesNumbersOnRight = true;

    /**
     * @brief Печатать номера реплик
     */
    bool printDialoguesNumbers = false;

    /**
     * @brief Печатать редакторские пометки
     */
    bool printReviewMarks = true;

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
     * @brief Нижний колонтитул
     */
    QString footer;
};

} // namespace BusinessLayer

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
     * @brief Список сцен для печати
     * @note Если пустое, значит печатаются все сцены
     */
    QVector<QString> exportScenes;

    /**
     * @brief Печатать номера сцен
     */
    bool showScenesNumbers = true;
    bool showScenesNumbersOnLeft = true;
    bool showScenesNumbersOnRight = true;

    /**
     * @brief Печатать номера реплик
     */
    bool showDialoguesNumbers = false;

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

    /**
     * @brief Песонаж, чьи реплики нужно выделить
     */
    QString highlightCharacter;

    /**
     * @brief Цвет для выделения реплик персонажа
     */
    QColor highlightCharacterColor;


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

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

    ExportOptions() = default;
    ExportOptions(const ExportOptions& _other)
    {
        copy(&_other, this);
    }

    /**
     * @brief Скопировать опции
     * @note Таким образом будем обходить внутреннюю ошибку g++ при инициализации наследников
     */
    static void copy(const ExportOptions* _source, ExportOptions* _dest);

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
    bool includeTitlePage = true;

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


/**
 * @brief Опции экспорта документов
 */
struct CORE_LIBRARY_EXPORT DocumentsExportOptions : public ExportOptions {

    DocumentsExportOptions() = default;
    DocumentsExportOptions(const ExportOptions& _other)
    {
        ExportOptions::copy(&_other, this);
    }

    /**
     * @brief Скопировать опции
     * @note Таким образом будем обходить внутреннюю ошибку g++ при инициализации наследников
     */
    static void copy(const DocumentsExportOptions* _source, DocumentsExportOptions* _dest);

    /**
     * @brief Печатать фотографии
     */
    bool includeMainPhoto = true;

    /**
     * @brief Список документов для выгрузки
     */
    QVector<QString> documents;

    /**
     * @brief Печатать конкретные параметры документов
     */
    bool includeStoryRole = true;
    bool includeOneLineDescription = false;
    bool includeLongDescription = false;
};


/**
 * @brief Опции экспорта документа
 */
struct CORE_LIBRARY_EXPORT DocumentExportOptions : public ExportOptions {

    DocumentExportOptions() = default;
    DocumentExportOptions(const ExportOptions& _other)
    {
        ExportOptions::copy(&_other, this);
    }

    /**
     * @brief Скопировать опции
     * @note Таким образом будем обходить внутреннюю ошибку g++ при инициализации наследников
     */
    static void copy(const DocumentExportOptions* _source, DocumentExportOptions* _dest);
    /**
     * @brief Печатать фотографии
     */
    bool includeMainPhoto = true;
    bool includeAdditionalPhotos = false;
};


} // namespace BusinessLayer

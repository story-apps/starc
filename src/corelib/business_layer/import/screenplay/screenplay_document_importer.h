#pragma once

#include "abstract_screenplay_importer.h"
#include "business_layer/import/abstract_qtextdocument_importer.h"

class QTextCursor;

namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Docx
 */
class CORE_LIBRARY_EXPORT ScreenplayDocumentImporter : public AbstractScreenplayImporter,
                                                       public AbstractQTextDocumentImporter
{
public:
    ScreenplayDocumentImporter() = default;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;

protected:
    /**
     * @brief Получить имя персонажа
     */
    QString characterName(const QString& _text) const override;

    /**
     * @brief Получить документ для импорта
     * @return true, если получилось открыть заданный файл
     */
    bool documentForImport(const QString& _filePath, QTextDocument& _document) const override;

    /**
     * @brief Получить название локации
     */
    QString locationName(const QString& _text) const override;

    /**
     * @brief Обработать блок сцены
     */
    QString processSceneHeading(const ImportOptions& _options, QTextCursor& _cursor) const override;

    /**
     * @brief Записать редакторские заметки
     */
    void writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const override;
};

} // namespace BusinessLayer

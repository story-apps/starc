#pragma once

#include <business_layer/document/text/text_document.h>


namespace BusinessLayer {
class ScreenplayTextModel;
enum class TextParagraphType;
} // namespace BusinessLayer

namespace BusinessLayer {
class TextCursor;

/**
 * @brief Класс документа сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextDocument : public TextDocument
{
    Q_OBJECT

public:
    explicit ScreenplayTextDocument(QObject* _parent = nullptr);

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectCharactersNames, bool _needToCorrectPageBreaks);

    /**
     * @brief Получить номер сцены для заданного блока
     */
    QString sceneNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить номер реплики для заданного блока
     */
    QString dialogueNumber(const QTextBlock& _forBlock) const;
};

} // namespace BusinessLayer

#pragma once

#include <business_layer/document/text/text_document.h>


namespace BusinessLayer {

/**
 * @brief Класс документа сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextDocument : public TextDocument
{
    Q_OBJECT

public:
    explicit ScreenplayTextDocument(QObject* _parent = nullptr);
    ~ScreenplayTextDocument() override;

    /**
     * @brief Настроить отображение поэпизодника
     */
    bool isTreatmentVisible() const;
    void setTreatmentVisible(bool _visible);

    /**
     * @brief Получить список видимых блоков в зависимости от режима отображения поэпизодника или
     * сценария
     */
    QSet<TextParagraphType> visibleBlocksTypes() const;

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

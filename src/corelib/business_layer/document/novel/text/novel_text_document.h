#pragma once

#include <business_layer/document/text/text_document.h>


namespace BusinessLayer {

/**
 * @brief Класс документа сценария
 */
class CORE_LIBRARY_EXPORT NovelTextDocument : public TextDocument
{
    Q_OBJECT

public:
    explicit NovelTextDocument(QObject* _parent = nullptr);
    ~NovelTextDocument() override;

    /**
     * @brief Отображается ли документ как поэпизодник или как сценарий
     */
    bool isOutlineDocument() const;
    void setOutlineDocument(bool _outline);

    /**
     * @brief Отображать ли биты (только для режима сценария, когда isTreatmentDocument == false)
     */
    bool isBeatsVisible() const;
    void setBeatsVisible(bool _visible);

    /**
     * @brief Список видимых блоков в зависимости от режима отображения документа
     */
    QSet<TextParagraphType> visibleBlocksTypes() const;

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectPageBreaks);

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

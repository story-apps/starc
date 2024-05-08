#pragma once

#include <business_layer/document/text/abstract_text_corrector.h>


namespace BusinessLayer {

/**
 * @brief Класс корректирующий текст сценария
 */
class CORE_LIBRARY_EXPORT NovelTextCorrector : public AbstractTextCorrector
{
    Q_OBJECT

public:
    explicit NovelTextCorrector(QTextDocument* _document);
    ~NovelTextCorrector() override;

    /**
     * @brief Установить необходимость корректировать текст блоков имён персонажей
     */
    void setCorrectionOptions(const QStringList& _options) override;

    /**
     * @brief Должен ли быть виден в документе блок заданного типа
     */
    bool isBlockVisible(TextParagraphType _type) const override;

protected:
    /**
     * @brief Очистить все сохранённые параметры
     */
    void clearImpl() override;

    /**
     * @brief Выполнить корректировки
     */
    void makeCorrections(int _position = -1, int _charsChanged = 0) override;

    /**
     * @brief Выполнить "мягкие" корректировки
     */
    void makeSoftCorrections(int _position = -1, int _charsChanged = 0) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

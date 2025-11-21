#pragma once

#include "../simple_text/simple_text_model.h"


namespace BusinessLayer {

/**
 * @brief Модель титульной страницы
 */
class CORE_LIBRARY_EXPORT TitlePageModel : public SimpleTextModel
{
    Q_OBJECT

public:
    explicit TitlePageModel(QObject* _parent = nullptr);
    ~TitlePageModel() override;

    /**
     * @brief Игнорируем установку названия документа
     */
    void setDocumentName(const QString& _name) override;

    /**
     * @brief Список действующих лиц
     */
    void setCharacters(const QVector<QPair<QString, QString>>& _characters);
    QVector<QPair<QString, QString>> characters() const;

signals:
    /**
     * @brief Запрос на обновление списка персонажей
     */
    void charactersUpdateRequested();

protected:
    /**
     * @brief Игнорируем обновление названия документа
     */
    void updateDisplayName(const QModelIndex& _index) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

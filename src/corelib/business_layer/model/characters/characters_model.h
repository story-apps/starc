#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class CharacterModel;

/**
 * @brief Модель списка персонажей
 */
class CORE_LIBRARY_EXPORT CharactersModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit CharactersModel(QObject* _parent = nullptr);
    ~CharactersModel() override;

    /**
     * @brief Добавить модель персонажа
     */
    void addCharacterModel(CharacterModel* _characterModel);

    /**
     * @brief Удалить модель персонажа
     */
    void removeCharacterModel(CharacterModel* _characterModel);

    /**
     * @brief Создать персонажа с заданным именем
     */
    void createCharacter(const QString& _name, const QByteArray& _content = {});

    /**
     * @brief Существует ли персонаж с заданным именем
     */
    bool exists(const QString& _name) const;

    /**
     * @brief Получить модель персонажа по его имени
     */
    CharacterModel* character(const QString& _name) const;

    /**
     * @brief Позиция карточки персонажа на схеме отношений
     */
    QPointF characterPosition(const QString& _name) const;
    void setCharacterPosition(const QString& _name, const QPointF& _position);
    Q_SIGNAL void characterPositionChanged(const QString& _name, const QPointF& _position);

    /**
     * @brief Реализация древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount(const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& _index) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;
    /** @} */

signals:
    /**
     * @brief Необходимо создать персонажа с заданным именем
     */
    void createCharacterRequested(const QString& _name, const QByteArray& _content);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

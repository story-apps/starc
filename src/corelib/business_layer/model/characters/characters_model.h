#pragma once

#include "../abstract_model.h"

#include <QColor>
#include <QRectF>
#include <QUuid>


namespace BusinessLayer {

class CharacterModel;

/**
 * @brief Группа персонажей
 */
class CORE_LIBRARY_EXPORT CharactersGroup
{
public:
    bool isValid() const;

    bool operator==(const CharactersGroup& _other) const;
    bool operator!=(const CharactersGroup& _other) const;

    QUuid id;
    QString name = {};
    QString description = {};
    QRectF rect = {};
    int lineType = Qt::SolidLine;
    QColor color = {};
};

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
     * @brief Получить модель персонажа по его идентификатору
     */
    CharacterModel* character(const QUuid& _uuid) const;

    /**
     * @brief Получить модель персонажа по его имени
     */
    CharacterModel* character(const QString& _name) const;

    /**
     * @brief Получить модель персонажа по его индексу
     */
    CharacterModel* character(int _row) const;

    /**
     * @brief Получить все модели персонажей с заданным именем
     */
    QVector<CharacterModel*> characters(const QString& _name) const;

    /**
     * @brief Группы персонажей
     */
    void createCharactersGroup(const QUuid& _groupId);
    void updateCharactersGroup(const CharactersGroup& _group);
    void removeCharactersGroup(const QUuid& _groupId);
    QVector<CharactersGroup> charactersGroups() const;
    Q_SIGNAL void charactersGroupAdded(const BusinessLayer::CharactersGroup& _group);
    Q_SIGNAL void charactersGroupChanged(const BusinessLayer::CharactersGroup& _group);
    Q_SIGNAL void charactersGroupRemoved(const BusinessLayer::CharactersGroup& _group);

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

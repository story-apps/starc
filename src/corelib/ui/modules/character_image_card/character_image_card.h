#pragma once

#include <ui/widgets/image/image_card.h>


/**
 * @brief Класс карточки персонажа, умеющий генерировать фитографию персонажа
 */
class CORE_LIBRARY_EXPORT CharacterImageCard : public ImageCard
{
    Q_OBJECT

public:
    explicit CharacterImageCard(QWidget* _parent = nullptr);
    ~CharacterImageCard() override;

    /**
     * @brief Сгенерировать фото для персонажа заданного пола
     */
    void generatePhoto(int _gender);

signals:
    /**
     * @brief Пользователь хочет сгенерировать фотку
     */
    void generatePhotoPressed();

protected:
    /**
     * @brief Получить список действий контекстного меню
     */
    QVector<QAction*> contextMenuActions() const override;

    /**
     * @brief Дизейблим возможность генерации постера
     */
    void processReadOnlyChange() override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

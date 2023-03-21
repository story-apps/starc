#pragma once

#include "../abstract_model.h"

#include <QColor>
#include <QUuid>


namespace BusinessLayer {

/**
 * @brief Категории ресурсов
 */
struct BreakdownResourceCategory {
    QUuid uuid;
    QString name;
    QString icon;
    QColor color;
    bool hasIds = false;
};

/**
 * @brief Ресурс
 */
struct BreakdownResource {
    QUuid uuid;
    QUuid categoryUuid;
    QString name;
    QString description;
};

/**
 * @brief Ресурс сцены
 */
struct BreakdownSceneResource {
    QUuid uuid;
    int qty = 0;
    QString description;
};

bool operator==(const BreakdownSceneResource& _lhs, const BreakdownSceneResource& _rhs);

/**
 * @brief Модель справочников сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayDictionariesModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplayDictionariesModel(QObject* _parent = nullptr);
    ~ScreenplayDictionariesModel() override;

    const QVector<QString>& sceneIntros() const;
    void addSceneIntro(const QString& _intro);
    void setSceneIntro(int _index, const QString& _intro);
    void removeSceneIntro(int _index);
    Q_SIGNAL void sceneIntrosChanged();

    const QVector<QString>& sceneTimes() const;
    void addSceneTime(const QString& _time);
    void setSceneTime(int _index, const QString& _time);
    void removeSceneTime(int _index);
    Q_SIGNAL void sceneTimesChanged();

    const QVector<QString>& characterExtensions() const;
    void addCharacterExtension(const QString& _extension);
    void setCharacterExtension(int _index, const QString& _extension);
    void removeCharacterExtension(int _index);
    Q_SIGNAL void charactersExtensionsChanged();

    const QVector<QString>& transitions() const;
    void addTransition(const QString& _transition);
    void setTransition(int _index, const QString& _transition);
    void removeTransition(int _index);
    Q_SIGNAL void transitionsChanged();

    QVector<QString> storyDays() const;
    void addStoryDay(const QString& _day);
    void removeStoryDay(const QString& _day);
    Q_SIGNAL void storyDaysChanged();

    QVector<QPair<QString, QColor>> tags() const;
    void addTags(const QVector<QPair<QString, QColor>>& _tags);
    void removeTags(const QVector<QPair<QString, QColor>>& _tags);
    Q_SIGNAL void tagsChanged();

    QVector<BreakdownResourceCategory> resourceCategories() const;
    void addResourceCategory(const QString& _name, const QString& _icon, const QColor& _color,
                             bool _hasIds);
    void setResourceCategory(const QUuid& _uuid, const QString& _name, const QString& _icon,
                             const QColor& _color, bool _hasIds);
    void moveResourceCategory(const QUuid& _uuid, int _index);
    void removeResourceCategory(const QUuid& _uuid);
    Q_SIGNAL void resourceCategoriesChanged();

    QVector<BreakdownResource> resources() const;
    void addResource(const QUuid& _categoryUuid, const QString& _name, const QString& _description);
    void setResource(const QUuid& _uuid, const QUuid& _categoryUuid, const QString& _name,
                     const QString& _description);
    void setResourceCategory(const QUuid& _uuid, const QUuid& _categoryUuid);
    void moveResource(const QUuid& _uuid, int _index);
    void removeResource(const QUuid& _uuid);
    Q_SIGNAL void resourcesChanged();

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

#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

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

    const QVector<QString>& storyDays() const;
    void addStoryDay(const QString& _day);
    void setStoryDay(int _index, const QString& _day);
    void removeStoryDay(int _index);
    Q_SIGNAL void storyDaysChanged();

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

    const QVector<QPair<QString, QColor>>& tags() const;
    void addTag(const QString& _tag, const QColor& _color);
    void removeTag(int _index);
    Q_SIGNAL void tagsChanged();

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

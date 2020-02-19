#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

class ScreenplayDictionariesModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplayDictionariesModel(QObject* _parent = nullptr);
    ~ScreenplayDictionariesModel() override;

    const QVector<QString>& sceneIntros() const;
    void addSceneIntro(const QString& _intro);

    const QVector<QString>& sceneTimes() const;
    void addSceneTime(const QString& _time);

    const QVector<QString>& characterExtensions() const;
    void addCharacterExtensionTime(const QString& _extension);

    const QVector<QString>& transitions() const;
    void addTransition(const QString& _transition);

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

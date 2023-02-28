#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT NovelDictionariesModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit NovelDictionariesModel(QObject* _parent = nullptr);
    ~NovelDictionariesModel() override;

    QVector<QString> storyDays() const;
    void addStoryDay(const QString& _day);
    void removeStoryDay(const QString& _day);
    Q_SIGNAL void storyDaysChanged();

    QVector<QPair<QString, QColor>> tags() const;
    void addTags(const QVector<QPair<QString, QColor>>& _tags);
    void removeTags(const QVector<QPair<QString, QColor>>& _tags);
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

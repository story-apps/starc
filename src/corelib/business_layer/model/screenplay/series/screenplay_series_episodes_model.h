#pragma once

#include "../../abstract_model.h"

#include <QColor>


namespace BusinessLayer {

class ScreenplaySeriesInformationModel;
class ScreenplayTextModel;
class ScreenplaySeriesEpisodesModelItem;


/**
 * @brief Модель посерийника
 */
class CORE_LIBRARY_EXPORT ScreenplaySeriesEpisodesModel : public AbstractModel
{
    Q_OBJECT

public:
    /**
     * @brief Сюжетная линия
     */
    struct StoryLine {
        QColor color;
        QString name;

        bool operator==(const StoryLine& _other) const;
    };

public:
    explicit ScreenplaySeriesEpisodesModel(QObject* _parent = nullptr);
    ~ScreenplaySeriesEpisodesModel() override;

    /**
     * @brief Задать модель информации о сериале
     */
    void setInformationModel(ScreenplaySeriesInformationModel* _model);
    ScreenplaySeriesInformationModel* informationModel() const;

    /**
     * @brief Список серий сериала
     */
    QVector<ScreenplayTextModel*> episodes() const;
    void setEpisodes(const QVector<ScreenplayTextModel*>& _episodes);
    Q_SIGNAL void episodesChanged(const QVector<ScreenplayTextModel*>& _episodes);

    /**
     * @brief Сюжетные линии
     */
    ScreenplaySeriesEpisodesModelItem* appendStoryLine();
    QVector<StoryLine> storyLines() const;
    void setStoryLines(const QVector<StoryLine>& _storyLines);
    Q_SIGNAL void storyLinesChanged(const QVector<StoryLine>& _storyLines);

    /**
     * @brief Обновить заданный элемент
     */
    void updateItem(ScreenplaySeriesEpisodesModelItem* _item);

    /**
     * @brief Удалить элемент из модели
     */
    void removeItem(ScreenplaySeriesEpisodesModelItem* _item);

    /**
     * @brief Переопределяем методы для собственной реализации модели
     */
    QModelIndex index(int _row, int _column = 0, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount(const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const override;

    /**
     * @brief Получить элемент модели для заданного индекса
     */
    ScreenplaySeriesEpisodesModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить индекс заданного элемента модели
     */
    QModelIndex indexForItem(ScreenplaySeriesEpisodesModelItem* _item) const;


protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    ChangeCursor applyPatch(const QByteArray& _patch) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

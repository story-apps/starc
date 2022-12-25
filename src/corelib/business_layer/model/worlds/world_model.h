#pragma once

#include "../abstract_model.h"

#include <domain/document_object.h>

#include <QColor>
#include <QUuid>


namespace BusinessLayer {

/**
 * @brief Маршрут к другим мирам
 */
class CORE_LIBRARY_EXPORT WorldRoute
{
public:
    bool isValid() const;

    bool operator==(const WorldRoute& _other) const;
    bool operator!=(const WorldRoute& _other) const;

    QUuid world;
    int lineType = Qt::SolidLine;
    QColor color = {};
    QString name = {};
    QString details = {};
};

/**
 * @brief Элемент мира
 */
class CORE_LIBRARY_EXPORT WorldItem
{
public:
    QString name;
    QString oneSentenceDescription;
    QString longDescription;
    Domain::DocumentImage photo;
};

/**
 * @brief Модель данных мира
 */
class CORE_LIBRARY_EXPORT WorldModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit WorldModel(QObject* _parent = nullptr);
    ~WorldModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _newName, const QString& _oldName);
    void setDocumentName(const QString& _name) override;

    QString oneSentenceDescription() const;
    void setOneSentenceDescription(const QString& _text);
    Q_SIGNAL void oneSentenceDescriptionChanged(const QString& _text);

    QString longDescription() const;
    void setLongDescription(const QString& _text);
    Q_SIGNAL void longDescriptionChanged(const QString& _text);

    Domain::DocumentImage mainPhoto() const;
    void setMainPhoto(const QPixmap& _photo);
    Q_SIGNAL void mainPhotoChanged(const Domain::DocumentImage& _photo);

    QVector<Domain::DocumentImage> photos() const;
    void addPhoto(const Domain::DocumentImage& _photo);
    void addPhotos(const QVector<QPixmap>& _photos);
    void removePhoto(const QUuid& _photoUuid);
    Q_SIGNAL void photosChanged(const QVector<Domain::DocumentImage>& _images);

    void createRoute(const QUuid& _toWorld);
    void updateRoute(const WorldRoute& _way);
    void removeRoute(QUuid _toWorld);
    WorldRoute route(const QUuid& _toWorld);
    WorldRoute route(WorldModel* _toWorld);
    QVector<WorldRoute> routes() const;
    Q_SIGNAL void routeAdded(const BusinessLayer::WorldRoute& _route);
    Q_SIGNAL void routeChanged(const BusinessLayer::WorldRoute& _route);
    Q_SIGNAL void routeRemoved(const BusinessLayer::WorldRoute& _route);

    //
    // WORLD INFO
    //

    QString overview() const;
    void setOverview(const QString& _text);
    Q_SIGNAL void overviewChanged(const QString& _text);

    QString earthLike() const;
    void setEarthLike(const QString& _text);
    Q_SIGNAL void earthLikeChanged(const QString& _text);

    QString history() const;
    void setHistory(const QString& _text);
    Q_SIGNAL void historyChanged(const QString& _text);

    QString mood() const;
    void setMood(const QString& _text);
    Q_SIGNAL void moodChanged(const QString& _text);

    //
    // NATURE
    //

    QString biology() const;
    void setBiology(const QString& _text);
    Q_SIGNAL void biologyChanged(const QString& _text);

    QString phisics() const;
    void setPhisics(const QString& _text);
    Q_SIGNAL void phisicsChanged(const QString& _text);

    QString astronomy() const;
    void setAstronomy(const QString& _text);
    Q_SIGNAL void astronomyChanged(const QString& _text);

    QString geography() const;
    void setGeography(const QString& _text);
    Q_SIGNAL void geographyChanged(const QString& _text);

    QVector<WorldItem> races() const;
    void setRaces(const QVector<WorldItem>& _items);
    Q_SIGNAL void racesChanged(const QVector<BusinessLayer::WorldItem>& _items);

    QVector<WorldItem> floras() const;
    void setFloras(const QVector<WorldItem>& _items);
    Q_SIGNAL void florasChanged(const QVector<BusinessLayer::WorldItem>& _items);

    QVector<WorldItem> animals() const;
    void setAnimals(const QVector<WorldItem>& _items);
    Q_SIGNAL void animalsChanged(const QVector<BusinessLayer::WorldItem>& _items);

    QVector<WorldItem> naturalResources() const;
    void setNaturalResources(const QVector<WorldItem>& _items);
    Q_SIGNAL void naturalResourcesChanged(const QVector<BusinessLayer::WorldItem>& _items);

    QVector<WorldItem> climates() const;
    void setClimates(const QVector<WorldItem>& _items);
    Q_SIGNAL void climatesChanged(const QVector<BusinessLayer::WorldItem>& _items);

    //
    // CULTURE
    //

    QVector<WorldItem> religions() const;
    void setReligions(const QVector<WorldItem>& _items);
    Q_SIGNAL void religionsChanged(const QVector<BusinessLayer::WorldItem>& _items);

    QVector<WorldItem> ethics() const;
    void setEthics(const QVector<WorldItem>& _items);
    Q_SIGNAL void ethicsChanged(const QVector<BusinessLayer::WorldItem>& _items);

    QVector<WorldItem> languages() const;
    void setLanguages(const QVector<WorldItem>& _items);
    Q_SIGNAL void languagesChanged(const QVector<BusinessLayer::WorldItem>& _items);

    QVector<WorldItem> castes() const;
    void setCastes(const QVector<WorldItem>& _items);
    Q_SIGNAL void castesChanged(const QVector<BusinessLayer::WorldItem>& _items);

    //    //
    //    // GEOGRAPHY
    //    //

    //    QString world() const;
    //    void setWorld(const QString& _text);
    //    Q_SIGNAL void worldChanged(const QString& _text);

    //    QString climate() const;
    //    void setClimate(const QString& _text);
    //    Q_SIGNAL void climateChanged(const QString& _text);

    //    QString landmark() const;
    //    void setLandmark(const QString& _text);
    //    Q_SIGNAL void landmarkChanged(const QString& _text);

    //    QString nearbyPlaces() const;
    //    void setNearbyPlaces(const QString& _text);
    //    Q_SIGNAL void nearbyPlacesChanged(const QString& _text);

    //    //
    //    // BACKGROUND
    //    //

    //    QString history() const;
    //    void setHistory(const QString& _text);
    //    Q_SIGNAL void historyChanged(const QString& _text);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initImageWrapper() override;
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    void applyPatch(const QByteArray& _patch) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

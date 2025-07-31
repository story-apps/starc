#pragma once

#include "../abstract_model.h"

#include <domain/document_object.h>

#include <QColor>
#include <QUuid>


namespace BusinessLayer {

/**
 * @brief Роль локации в истории
 */
enum class LocationStoryRole {
    Primary,
    Secondary,
    Tertiary,
    Undefined,
};

/**
 * @brief Маршрут к другим локациям
 */
class CORE_LIBRARY_EXPORT LocationRoute
{
public:
    bool isValid() const;

    bool operator==(const LocationRoute& _other) const;
    bool operator!=(const LocationRoute& _other) const;

    QUuid location;
    int lineType = Qt::SolidLine;
    QColor color = {};
    QString name = {};
    QString details = {};
};

/**
 * @brief Сцены локации
 */
class CORE_LIBRARY_EXPORT LocationScenes
{
public:
    bool operator==(const LocationScenes& _other) const;

    QUuid documentUuid;
    QString documentName;
    QVector<QModelIndex> scenesIndexes;
};

/**
 * @brief Модель данных локации
 */
class CORE_LIBRARY_EXPORT LocationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit LocationModel(QObject* _parent = nullptr);
    ~LocationModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _newName, const QString& _oldName);
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;

    QColor color() const;
    void setColor(const QColor& _color);
    Q_SIGNAL void colorChanged(const QColor& _color);

    LocationStoryRole storyRole() const;
    void setStoryRole(LocationStoryRole _role);
    Q_SIGNAL void storyRoleChanged(BusinessLayer::LocationStoryRole _role);

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

    void createRoute(const QUuid& _toLocation);
    void updateRoute(const LocationRoute& _way);
    void removeRoute(QUuid _toLocation);
    LocationRoute route(const QUuid& _toLocation);
    LocationRoute route(LocationModel* _toLocation);
    QVector<LocationRoute> routes() const;
    Q_SIGNAL void routeAdded(const BusinessLayer::LocationRoute& _route);
    Q_SIGNAL void routeChanged(const BusinessLayer::LocationRoute& _route);
    Q_SIGNAL void routeRemoved(const BusinessLayer::LocationRoute& _route);

    //
    // SENSE
    //

    QString sight() const;
    void setSight(const QString& _text);
    Q_SIGNAL void sightChanged(const QString& _text);

    QString smell() const;
    void setSmell(const QString& _text);
    Q_SIGNAL void smellChanged(const QString& _text);

    QString sound() const;
    void setSound(const QString& _text);
    Q_SIGNAL void soundChanged(const QString& _text);

    QString taste() const;
    void setTaste(const QString& _text);
    Q_SIGNAL void tasteChanged(const QString& _text);

    QString touch() const;
    void setTouch(const QString& _text);
    Q_SIGNAL void touchChanged(const QString& _text);

    //
    // GEOGRAPHY
    //

    QString location() const;
    void setLocation(const QString& _text);
    Q_SIGNAL void locationChanged(const QString& _text);

    QString climate() const;
    void setClimate(const QString& _text);
    Q_SIGNAL void climateChanged(const QString& _text);

    QString landmark() const;
    void setLandmark(const QString& _text);
    Q_SIGNAL void landmarkChanged(const QString& _text);

    QString nearbyPlaces() const;
    void setNearbyPlaces(const QString& _text);
    Q_SIGNAL void nearbyPlacesChanged(const QString& _text);

    //
    // BACKGROUND
    //

    QString history() const;
    void setHistory(const QString& _text);
    Q_SIGNAL void historyChanged(const QString& _text);

    //
    // Сцены
    //
    QVector<LocationScenes> scenes() const;
    void setScenes(const QVector<LocationScenes>& _scenes);
    Q_SIGNAL void scenesChanged(const QVector<BusinessLayer::LocationScenes>& _scenes);
    //
    void updateDialogues();
    Q_SIGNAL void scenesUpdateRequested();

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initImageWrapper() override;
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

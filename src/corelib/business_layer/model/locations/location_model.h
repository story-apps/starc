#pragma once

#include "../abstract_model.h"

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
    void setDocumentName(const QString& _name) override;

    LocationStoryRole storyRole() const;
    void setStoryRole(LocationStoryRole _role);
    Q_SIGNAL void storyRoleChanged(BusinessLayer::LocationStoryRole _role);

    QString oneSentenceDescription() const;
    void setOneSentenceDescription(const QString& _text);
    Q_SIGNAL void oneSentenceDescriptionChanged(const QString& _text);

    QString longDescription() const;
    void setLongDescription(const QString& _text);
    Q_SIGNAL void longDescriptionChanged(const QString& _text);

    const QPixmap& mainPhoto() const;
    void setMainPhoto(const QPixmap& _photo);
    Q_SIGNAL void mainPhotoChanged(const QPixmap& _photo);

    void createRoute(const QUuid& _toLocation);
    void updateRoute(const LocationRoute& _way);
    void removeRoute(QUuid _toLocation);
    LocationRoute route(const QUuid& _toLocation);
    LocationRoute route(LocationModel* _toLocation);
    QVector<LocationRoute> routes() const;
    Q_SIGNAL void routeAdded(const BusinessLayer::LocationRoute& _route);
    Q_SIGNAL void routeChanged(const BusinessLayer::LocationRoute& _route);
    Q_SIGNAL void routeRemoved(const BusinessLayer::LocationRoute& _route);

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

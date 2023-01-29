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
    QString documentName() const override;
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

    QString physics() const;
    void setPhysics(const QString& _text);
    Q_SIGNAL void physicsChanged(const QString& _text);

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

    //
    // SYSTEM
    //

    QString technology() const;
    void setTechnology(const QString& _text);
    Q_SIGNAL void technologyChanged(const QString& _text);

    QString economy() const;
    void setEconomy(const QString& _text);
    Q_SIGNAL void economyChanged(const QString& _text);

    QString trade() const;
    void setTrade(const QString& _text);
    Q_SIGNAL void tradeChanged(const QString& _text);

    QString business() const;
    void setBusiness(const QString& _text);
    Q_SIGNAL void businessChanged(const QString& _text);

    QString industry() const;
    void setIndustry(const QString& _text);
    Q_SIGNAL void industryChanged(const QString& _text);

    QString currency() const;
    void setCurrency(const QString& _text);
    Q_SIGNAL void currencyChanged(const QString& _text);

    QString education() const;
    void setEducation(const QString& _text);
    Q_SIGNAL void educationChanged(const QString& _text);

    QString communication() const;
    void setCommunication(const QString& _text);
    Q_SIGNAL void communicationChanged(const QString& _text);

    QString art() const;
    void setArt(const QString& _text);
    Q_SIGNAL void artChanged(const QString& _text);

    QString entertainment() const;
    void setEntertainment(const QString& _text);
    Q_SIGNAL void entertainmentChanged(const QString& _text);

    QString travel() const;
    void setTravel(const QString& _text);
    Q_SIGNAL void travelChanged(const QString& _text);

    QString science() const;
    void setScience(const QString& _text);
    Q_SIGNAL void scienceChanged(const QString& _text);

    //
    // POLITICS
    //

    QString governmentFormat() const;
    void setGovernmentFormat(const QString& _text);
    Q_SIGNAL void governmentFormatChanged(const QString& _text);

    QString governmentHistory() const;
    void setGovernmentHistory(const QString& _text);
    Q_SIGNAL void governmentHistoryChanged(const QString& _text);

    QString laws() const;
    void setLaws(const QString& _text);
    Q_SIGNAL void lawsChanged(const QString& _text);

    QString foreignRelations() const;
    void setForeignRelations(const QString& _text);
    Q_SIGNAL void foreignRelationsChanged(const QString& _text);

    QString perceptionOfGovernment() const;
    void setPerceptionOfGovernment(const QString& _text);
    Q_SIGNAL void perceptionOfGovernmentChanged(const QString& _text);

    QString propaganda() const;
    void setPropaganda(const QString& _text);
    Q_SIGNAL void propagandaChanged(const QString& _text);

    QString antiGovernmentOrganisations() const;
    void setAntiGovernmentOrganisations(const QString& _text);
    Q_SIGNAL void antiGovernmentOrganisationsChanged(const QString& _text);

    QString pastWar() const;
    void setPastWar(const QString& _text);
    Q_SIGNAL void pastWarChanged(const QString& _text);

    QString currentWar() const;
    void setCurrentWar(const QString& _text);
    Q_SIGNAL void currentWarChanged(const QString& _text);

    QString potentialWar() const;
    void setPotentialWar(const QString& _text);
    Q_SIGNAL void potentialWarChanged(const QString& _text);

    //
    // MAGIC
    //

    QString magicRule() const;
    void setMagicRule(const QString& _text);
    Q_SIGNAL void magicRuleChanged(const QString& _text);

    QString whoCanUse() const;
    void setWhoCanUse(const QString& _text);
    Q_SIGNAL void whoCanUseChanged(const QString& _text);

    QString effectToWorld() const;
    void setEffectToWorld(const QString& _text);
    Q_SIGNAL void effectToWorldChanged(const QString& _text);

    QString effectToSociety() const;
    void setEffectToSociety(const QString& _text);
    Q_SIGNAL void effectToSocietyChanged(const QString& _text);

    QString effectToTechnology() const;
    void setEffectToTechnology(const QString& _text);
    Q_SIGNAL void effectToTechnologyChanged(const QString& _text);

    QVector<WorldItem> magicTypes() const;
    void setMagicTypes(const QVector<WorldItem>& _items);
    Q_SIGNAL void magicTypesChanged(const QVector<BusinessLayer::WorldItem>& _items);

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

#pragma once

#include "../abstract_model.h"

#include <domain/starcloud_api.h>


namespace BusinessLayer {

class StructureModel;
struct ChronometerOptions;


/**
 * @brief Модели информации о проекте
 */
class CORE_LIBRARY_EXPORT ProjectInformationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ProjectInformationModel(QObject* _parent = nullptr);
    ~ProjectInformationModel() override;

    //
    // Информация
    //

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;

    const QString& logline() const;
    void setLogline(const QString& _logline);
    Q_SIGNAL void loglineChanged(const QString& _logline);

    const QPixmap& cover() const;
    void setCover(const QPixmap& _cover);
    void setCover(const QUuid& _uuid, const QPixmap& _cover);
    Q_SIGNAL void coverChanged(const QPixmap& _cover);

    //
    // Параметры сценария
    //

    bool overrideCommonSettingsForScreenplay() const;
    void setOverrideCommonSettingsForScreenplay(bool _override);
    Q_SIGNAL void overrideCommonSettingsForScreenplayChanged(bool _override);

    QString templateIdForScreenplay() const;
    void setTemplateIdForScreenplay(const QString& _templateId);
    void setTemplateForScreenplay(const QString& _templateId, const QString& _templateData);
    Q_SIGNAL void templateIdForScreenplayChanged(const QString& _templateId);

    bool showSceneNumbersForScreenplay() const;
    void setShowSceneNumbersForScreenplay(bool _show);
    Q_SIGNAL void showSceneNumbersForScreenplayChanged(bool _show);

    bool showSceneNumbersOnLeftForScreenplay() const;
    void setShowSceneNumbersOnLeftForScreenplay(bool _show);
    Q_SIGNAL void showSceneNumbersOnLeftForScreenplayChanged(bool _show);

    bool showSceneNumbersOnRightForScreenplay() const;
    void setShowSceneNumbersOnRightForScreenplay(bool _show);
    Q_SIGNAL void showSceneNumbersOnRightForScreenplayChanged(bool _show);

    bool showDialoguesNumbersForScreenplay() const;
    void setShowDialoguesNumbersForScreenplay(bool _show);
    Q_SIGNAL void showDialoguesNumbersForScreenplayChanged(bool _show);

    ChronometerOptions chronometerOptionsForScreenplay() const;
    void setChronometerOptionsForScreenplay(const ChronometerOptions& _options);
    Q_SIGNAL void chronometerOptionsForScreenplayChanged(const ChronometerOptions& _options);

    //
    // Модель структуры
    //
    BusinessLayer::StructureModel* structureModel() const;
    void setStructureModel(BusinessLayer::StructureModel* _model);

    //
    // Соавторы
    //

    QVector<Domain::ProjectCollaboratorInfo> collaborators() const;
    void setCollaborators(const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);
    Q_SIGNAL void collaboratorsChanged(
        const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);

    QVector<Domain::TeamMemberInfo> teammates() const;
    void setTeammates(const QVector<Domain::TeamMemberInfo>& _teammates);
    Q_SIGNAL void teammatesChanged(const QVector<Domain::TeamMemberInfo>& _teammates);

signals:
    /**
     * @brief Пользователь хочет добавить соавтора в проект
     */
    void collaboratorInviteRequested(const QString& _email, const QColor& _color, int _role,
                                     const QHash<QUuid, int>& _permissions);
    void collaboratorUpdateRequested(const QString& _email, const QColor& _color, int _role,
                                     const QHash<QUuid, int>& _permissions);
    void collaboratorRemoveRequested(const QString& _email);

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

#pragma once

#include "../abstract_model.h"


namespace Domain {
struct ProjectCollaboratorInfo;
}

namespace BusinessLayer {
class StructureModel;

class CORE_LIBRARY_EXPORT ProjectInformationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ProjectInformationModel(QObject* _parent = nullptr);
    ~ProjectInformationModel() override;

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

    BusinessLayer::StructureModel* structureModel() const;
    void setStructureModel(BusinessLayer::StructureModel* _model);

    QVector<Domain::ProjectCollaboratorInfo> collaborators() const;
    void setCollaborators(const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);
    Q_SIGNAL void collaboratorsChanged(
        const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);

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

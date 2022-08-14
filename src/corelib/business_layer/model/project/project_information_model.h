#pragma once

#include "../abstract_model.h"


namespace Domain {
struct ProjectCollaboratorInfo;
}

namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ProjectInformationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ProjectInformationModel(QObject* _parent = nullptr);
    ~ProjectInformationModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    void setDocumentName(const QString& _name) override;

    const QString& logline() const;
    void setLogline(const QString& _logline);
    Q_SIGNAL void loglineChanged(const QString& _logline);

    const QPixmap& cover() const;
    void setCover(const QPixmap& _cover);
    Q_SIGNAL void coverChanged(const QPixmap& _cover);

    QVector<Domain::ProjectCollaboratorInfo> collaborators() const;
    void setCollaborators(const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);
    Q_SIGNAL void collaboratorsChanged(
        const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);

signals:
    /**
     * @brief Пользователь хочет добавить соавтора в проект
     */
    void collaboratorInviteRequested(const QString& _email, const QColor& _color, int _role);

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

#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

class ProjectInformationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ProjectInformationModel(QObject* _parent = nullptr);
    ~ProjectInformationModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);

    const QString& logline() const;
    void setLogline(const QString& _logline);
    Q_SIGNAL void loglineChanged(const QString& _logline);

    const QPixmap& cover() const;
    void setCover(const QPixmap& _cover);
    Q_SIGNAL void coverChanged(const QPixmap& _cover);

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

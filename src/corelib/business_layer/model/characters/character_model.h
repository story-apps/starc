#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

class CharacterModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit CharacterModel(QObject* _parent = nullptr);
    ~CharacterModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    void setDocumentName(const QString &_name) override;

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

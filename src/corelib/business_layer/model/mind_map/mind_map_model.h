#pragma once

#include "../abstract_model.h"

namespace Domain {
struct DocumentImage;
}


namespace BusinessLayer {

/**
 * @brief Модель ментальной карты
 */
class CORE_LIBRARY_EXPORT MindMapModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit MindMapModel(QObject* _parent = nullptr);
    ~MindMapModel() override;

    QString name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;

    QString description() const;
    void setDescription(const QString& _description);
    Q_SIGNAL void descriptionChanged(const QString& _description);

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

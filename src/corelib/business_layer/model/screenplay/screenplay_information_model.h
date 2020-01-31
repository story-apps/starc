#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

class ScreenplayInformationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplayInformationModel(QObject* _parent = nullptr);
    ~ScreenplayInformationModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    void setDocumentName(const QString &_name) override;

    const QString& header() const;
    void setHeader(const QString& _header);
    Q_SIGNAL void headerChanged(const QString& _header);

    const QString& footer() const;
    void setFooter(const QString& _footer);
    Q_SIGNAL void footerChanged(const QString& _footer);

    const QString& scenesNumbersPrefix() const;
    void setScenesNumbersPrefix(const QString& _prefix);
    Q_SIGNAL void scenesNumbersPrefixChanged(const QString& _prefix);

    int scenesNumberingStartAt() const;
    void setScenesNumberingStartAt(int _startNumber);
    Q_SIGNAL void scenesNumberingStartAtChanged(int _startNumber);

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

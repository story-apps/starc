#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

class TextModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit TextModel(QObject* _parent = nullptr);
    ~TextModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);

    const QString& text() const;
    void setText(const QString& _text);
    Q_SIGNAL void textChanged(const QString& _text);

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


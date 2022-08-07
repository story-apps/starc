#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

/**
 * @brief Модель справочников комикса
 */
class CORE_LIBRARY_EXPORT ComicBookDictionariesModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ComicBookDictionariesModel(QObject* _parent = nullptr);
    ~ComicBookDictionariesModel() override;

    const QStringList& singlePageIntros() const;
    const QStringList& multiplePageIntros() const;

    const QStringList& singlePanelIntros() const;
    const QStringList& multiplePanelIntros() const;

    const QStringList& commonCharacters() const;

    const QStringList& characterExtensions() const;
    void addCharacterExtension(const QString& _extension);
    Q_SIGNAL void charactersExtensionsChanged();

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

#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

/**
 * @brief Модель корзины с удалёнными документами
 */
class CORE_LIBRARY_EXPORT RecycleBinModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit RecycleBinModel(QObject* _parent = nullptr);
    ~RecycleBinModel() override;

    /**
     * @brief Количество документов к удалению
     */
    int documentsToRemoveSize() const;
    void setDocumentsToRemoveSize(int _size);

signals:
    /**
     * @brief Изменилось количество документов к удалению
     */
    void documentsToRemoveSizeChanged(int _size);

    /**
     * @brief Необходимо очистить корзинку
     */
    void emptyRecycleBinRequested();

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

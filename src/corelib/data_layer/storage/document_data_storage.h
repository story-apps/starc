#pragma once

#include <business_layer/model/abstract_image_wrapper.h>

#include <QScopedPointer>

#include <corelib_global.h>


namespace DataStorageLayer {

/**
 * @brief Хранилище документов-изображений
 */
class CORE_LIBRARY_EXPORT DocumentImageStorage : public BusinessLayer::AbstractImageWrapper
{
public:
    DocumentImageStorage();
    ~DocumentImageStorage() override;

    QPixmap load(const QUuid& _uuid) const override;
    QUuid save(const QPixmap& _image) override;

    void clear();
    void saveChanges();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer

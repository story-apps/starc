#pragma once

#include "image_card.h"

class QAction;

/**
 * Compatibility implementation used by STARC's packaged character plugins.
 *
 * The public repository does not include the original implementation, but the
 * official plugins depend on this exported widget. Keeping the class small and
 * based on ImageCard preserves the expected ABI while retaining local image
 * selection and the plugin's photo-generation hook.
 */
class CORE_LIBRARY_EXPORT CharacterImageCard : public ImageCard
{
    Q_OBJECT

public:
    explicit CharacterImageCard(QWidget* _parent = nullptr);
    ~CharacterImageCard() override;

    void generatePhoto(int _progress);

signals:
    void generatePhotoPressed();

protected:
    QVector<QAction*> contextMenuActions() const override;
    void processReadOnlyChange() override;
    void updateTranslations() override;

private:
    QAction* m_generatePhotoAction = nullptr;
};

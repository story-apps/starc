#include "character_image_card.h"

#include <QAction>

CharacterImageCard::CharacterImageCard(QWidget* _parent)
    : ImageCard(_parent)
    , m_generatePhotoAction(new QAction(this))
{
    m_generatePhotoAction->setIconText(u8"\U000F1353");
    connect(m_generatePhotoAction, &QAction::triggered, this,
            &CharacterImageCard::generatePhotoPressed);

    setDecorationIcon(u8"\U000F0006");
    updateTranslations();
}

CharacterImageCard::~CharacterImageCard() = default;

void CharacterImageCard::generatePhoto(int _progress)
{
    Q_UNUSED(_progress)
}

QVector<QAction*> CharacterImageCard::contextMenuActions() const
{
    auto actions = ImageCard::contextMenuActions();
    actions.prepend(m_generatePhotoAction);
    return actions;
}

void CharacterImageCard::processReadOnlyChange()
{
    ImageCard::processReadOnlyChange();
    m_generatePhotoAction->setEnabled(!isReadOnly());
}

void CharacterImageCard::updateTranslations()
{
    ImageCard::updateTranslations();
    m_generatePhotoAction->setText(tr("Generate character photo"));
    setSupportingText(tr("Add character photo"), tr("Change character photo"),
                      tr("Remove the character photo?"));
    setImageCroppingText(tr("Crop character photo"));
}

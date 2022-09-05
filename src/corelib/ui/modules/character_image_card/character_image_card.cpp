#include "character_image_card.h"

#include <3rd_party/webloader/src/NetworkRequestLoader.h>
#include <ui/widgets/image/image_cropping_dialog.h>
#include <ui/widgets/task_bar/task_bar.h>

#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>


namespace {
const QSizeF kImageProportions(288, 340);
}

class CharacterImageCard::Implementation
{
public:
    explicit Implementation(CharacterImageCard* _q);

    /**
     * @brief Сгенерировать фотку персонажа
     */
    void generateCharacterPhoto();

    /**
     * @brief Обрезать заданное изображение
     */
    void cropImage(const QPixmap& _image);


    CharacterImageCard* q = nullptr;

    int characterGender = -1;
    QAction* generatePhotoAction = nullptr;
    QString photoGenerationTaskId;
    QString photoCopyright;
};

CharacterImageCard::Implementation::Implementation(CharacterImageCard* _q)
    : q(_q)
    , generatePhotoAction(new QAction(_q))
{
    generatePhotoAction->setIconText(u8"\U000F0674");
}

void CharacterImageCard::Implementation::generateCharacterPhoto()
{
    if (!photoGenerationTaskId.isEmpty()) {
        return;
    }

    photoGenerationTaskId = QUuid::createUuid().toString();
    TaskBar::addTask(photoGenerationTaskId);
    TaskBar::setTaskTitle(photoGenerationTaskId, tr("Prepare to generating photo"));

    //
    // Загружаем первую страницу, чтобы узнать суммарное количество страниц по запросу
    //
    QString keywords;
    if (characterGender == 0) {
        keywords.append("male,");
    } else if (characterGender == 1) {
        keywords.append("female,");
    }
    keywords.append("face");
    const auto url
        = QString("https://starc.app/api/services/unsplash/search?text=%1").arg(keywords);
    //
    // Выбираем случайную страницу и загружаем её, чтобы получить конкретные фотки
    //
    // NOTE: оптимизация, чтобы не делать бессмысленный запрос на количество страниц, API Unsplash
    // всегда отдаёт максимально 10000 изображений, а соответственно чуть более 400 страниц
    //
    const auto totalPages = 400;
    const auto pageIndex = QRandomGenerator::global()->bounded(0, totalPages);
    const auto photosPageUrl = QString("%1&page=%2").arg(url).arg(pageIndex);
    NetworkRequestLoader::loadAsync(photosPageUrl, q, [this](const QByteArray& _data) {
        //
        // Выбираем случайную фотку на странице и загружаем ссылки на неё
        //
        const auto photos = QJsonDocument::fromJson(_data).object()["results"].toArray();
        const auto photoIndex = QRandomGenerator::global()->bounded(0, photos.size());
        const auto photoInfo = photos.at(photoIndex).toObject();
        const auto photoPageUrl
            = QString("https://starc.app/api/services/unsplash/download?url=%1")
                  .arg(photoInfo["links"].toObject()["download_location"].toString());
        photoCopyright = tr("Photo by %1 on Unsplash.com")
                             .arg(photoInfo["user"].toObject()["name"].toString());
        NetworkRequestLoader::loadAsync(
            photoPageUrl, q, [this, photoInfo](const QByteArray& _data) {
                //
                // Собственно загружаем фотографию
                //
                TaskBar::setTaskTitle(photoGenerationTaskId, tr("Generating photo"));
                const auto url = QJsonDocument::fromJson(_data).object()["url"].toString();
                NetworkRequest* request = new NetworkRequest;
                connect(request, &NetworkRequest::downloadComplete, q,
                        [this](const QByteArray& _imageData) {
                            QPixmap image;
                            image.loadFromData(_imageData);
                            cropImage(image);
                        });
                connect(request, &NetworkRequest::downloadProgress, q, [this, url](int _progress) {
                    TaskBar::setTaskProgress(photoGenerationTaskId, _progress);
                });
                connect(request, &NetworkRequest::finished, q, [this, request, url] {
                    request->deleteLater();
                    TaskBar::finishTask(photoGenerationTaskId);
                    photoGenerationTaskId.clear();
                });
                request->loadAsync(url);
            });
    });
}

void CharacterImageCard::Implementation::cropImage(const QPixmap& _image)
{
    if (_image.isNull()) {
        return;
    }

    auto dlg = new ImageCroppingDialog(q->topLevelWidget());
    dlg->setImage(_image);
    dlg->setImageProportion(kImageProportions);
    dlg->setImageProportionFixed(true);
    dlg->setImageCroppingText(tr("Select an area for the character main photo"));
    if (!photoCopyright.isEmpty()) {
        dlg->setImageCroppingNote(photoCopyright);
        photoCopyright.clear();
    }
    connect(dlg, &ImageCroppingDialog::disappeared, dlg, &ImageCroppingDialog::deleteLater);
    connect(dlg, &ImageCroppingDialog::imageSelected, q, &CharacterImageCard::setImage);

    dlg->showDialog();
}


// **

CharacterImageCard::CharacterImageCard(QWidget* _parent)
    : ImageCard(_parent)
    , d(new Implementation(this))
{
    connect(d->generatePhotoAction, &QAction::triggered, this,
            &CharacterImageCard::generatePhotoPressed);
}

CharacterImageCard::~CharacterImageCard() = default;

void CharacterImageCard::generatePhoto(int _gender)
{
    d->characterGender = _gender;
    d->generateCharacterPhoto();
}

QVector<QAction*> CharacterImageCard::contextMenuActions() const
{
    auto actions = ImageCard::contextMenuActions();
    actions.prepend(d->generatePhotoAction);
    return actions;
}

void CharacterImageCard::processReadOnlyChange()
{
    d->generatePhotoAction->setEnabled(!isReadOnly());
}

void CharacterImageCard::updateTranslations()
{
    ImageCard::updateTranslations();

    d->generatePhotoAction->setText(tr("Generate"));
}

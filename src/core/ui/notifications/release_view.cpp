#include "release_view.h"

#include <3rd_party/webloader/src/NetworkRequest.h>
#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/progress_bar/progress_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/platform_helper.h>

#include <QBoxLayout>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>


namespace Ui {

namespace {
enum State {
    Ready,
    Downloading,
    DownloadingFailed,
    Downloaded,
};
}

class ReleaseView::Implementation
{
public:
    Implementation(ReleaseView* _q, const Domain::Notification& _notification);

    /**
     * @brief Загрузить обновление
     */
    void downloadUpdate();

    /**
     * @brief Остановить загрузку обновления
     */
    void stopDownloading();

    /**
     * @brief Установить обновление
     */
    void installUpdate();

    /**
     * @brief Обновить состояние виджета
     */
    void updateState();


    ReleaseView* q = nullptr;

    Domain::Notification notification;
    State state = Ready;
    QString downloadedFilePath;

    QScopedPointer<NetworkRequest> downloader;

    ImageLabel* avatarLabel = nullptr;
    CaptionLabel* dateTimeLabel = nullptr;
    Subtitle2Label* titleLabel = nullptr;
    Body1Label* bodyLabel = nullptr;
    Subtitle2LinkLabel* readMoreLink = nullptr;
    IconsSmallLabel* installedIcon = nullptr;
    Body1Label* installedLabel = nullptr;
    ProgressBar* downloadingProgress = nullptr;
    IconButton* downloadingAction = nullptr;
    Button* installButton = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
};

ReleaseView::Implementation::Implementation(ReleaseView* _q,
                                            const Domain::Notification& _notification)
    : q(_q)
    , notification(_notification)
    , avatarLabel(new ImageLabel(_q))
    , dateTimeLabel(new CaptionLabel(_q))
    , titleLabel(new Subtitle2Label(_q))
    , bodyLabel(new Body1Label(_q))
    , readMoreLink(new Subtitle2LinkLabel(_q))
    , installedIcon(new IconsSmallLabel(_q))
    , installedLabel(new Body1Label(_q))
    , downloadingProgress(new ProgressBar(_q))
    , downloadingAction(new IconButton(_q))
    , installButton(new Button(_q))
    , buttonsLayout(new QHBoxLayout)
{
    avatarLabel->setImage(QPixmap(":/images/logo"));
    downloadingProgress->hide();
    downloadingAction->hide();

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(installedIcon);
    buttonsLayout->addWidget(installedLabel, 1, Qt::AlignVCenter);
    buttonsLayout->addWidget(downloadingProgress, 1, Qt::AlignVCenter);
    buttonsLayout->addWidget(downloadingAction);
    buttonsLayout->addWidget(installButton);
    buttonsLayout->addStretch();
}

void ReleaseView::Implementation::downloadUpdate()
{
    downloader.reset(new NetworkRequest);

    connect(downloader.data(), &NetworkRequest::downloadProgress, q,
            [this](int _progress) { downloadingProgress->setProgress(_progress / 100.0); });
    connect(downloader.data(), &NetworkRequest::downloadComplete, q,
            [this](const QByteArray& _data, const QUrl _url) {
                if (_data.isEmpty()) {
                    state = DownloadingFailed;
                    updateState();
                    return;
                }

                const QString tempDirPath
                    = QDir::toNativeSeparators(QStandardPaths::writableLocation(
#ifdef Q_OS_LINUX
                        QStandardPaths::DownloadLocation
#else
                        QStandardPaths::TempLocation
#endif
                        ));
                downloadedFilePath = tempDirPath + QDir::separator() + _url.fileName();
                QFileInfo tempFileInfo(downloadedFilePath);
                int copyIndex = 1;
                while (tempFileInfo.exists()) {
                    auto fileName = _url.fileName();
                    fileName.replace(".", QString(".%1.").arg(copyIndex++));
                    downloadedFilePath = tempDirPath + QDir::separator() + fileName;
                    tempFileInfo.setFile(downloadedFilePath);
                }
                QFile tempFile(downloadedFilePath);
                if (tempFile.open(QIODevice::WriteOnly)) {
                    tempFile.write(_data);
                    tempFile.close();

                    state = Downloaded;
                    updateState();
                }
            });
    connect(downloader.data(), &NetworkRequest::error, q, [this] {
        state = DownloadingFailed;
        updateState();
    });

    const auto json = QJsonDocument::fromJson(notification.notification.toUtf8()).object();
    downloader->loadAsync(json["download_link"].toString());

    downloadingProgress->setProgress(0);
    state = Downloading;
    updateState();
}

void ReleaseView::Implementation::stopDownloading()
{
    if (downloader.isNull()) {
        return;
    }

    downloader->stop();
    downloader.reset();

    state = Ready;
    updateState();
}

void ReleaseView::Implementation::installUpdate()
{
    const bool updateStarted =
#ifdef Q_OS_LINUX
        //
        // Т.к. не все линуксы умеют устанавливать AppImage, то просто открываем папку с файлом
        //
        PlatformHelper::showInGraphicalShell(downloadedFilePath);
#else
        //
        // Для остальных операционок запускаем процесс установки обновления
        //
        QDesktopServices::openUrl(QUrl::fromLocalFile(downloadedFilePath));
#endif
    if (updateStarted) {
        QCoreApplication::quit();
    }
}

void ReleaseView::Implementation::updateState()
{
    dateTimeLabel->setText(notification.dateTime.toLocalTime().toString("dd.MM.yyyy hh:mm"));
    const auto json = QJsonDocument::fromJson(notification.notification.toUtf8()).object();
    const auto version = json.value("version").toString();
    //
    // Dev версия
    //
    if (notification.type == Domain::NotificationType::UpdateDevLinux
        || notification.type == Domain::NotificationType::UpdateDevMac
        || notification.type == Domain::NotificationType::UpdateDevWindows) {
        titleLabel->setText(tr("Dev version updated"));
        bodyLabel->setText(
            tr("Story Architect version %1 was published for testing.").arg(version));
        readMoreLink->setText(tr("Check out what's changed"));
    }
    //
    // Стабильная версия
    //
    else {
        titleLabel->setText(tr("Version %1 published").arg(version));
        bodyLabel->setText(json.value("info").toString());
        readMoreLink->setText(tr("Read more about release"));
    }
    auto readMoreUrl = json.value("read_more_link").toString();
    switch (QLocale().language()) {
    case QLocale::Russian:
    case QLocale::Belarusian:
    case QLocale::Ukrainian: {
        readMoreUrl = readMoreUrl.replace("starc.app/", "starc.app/ru/");
        break;
    }

    default: {
        break;
    }
    }
    readMoreLink->setLink(readMoreUrl);

    switch (state) {
    default:
    case Ready: {
        installButton->setText(tr("Download"));
        installedLabel->setText(tr("Installed"));

        downloadingProgress->hide();
        downloadingAction->hide();
        if (QCoreApplication::applicationVersion() == version) {
            installButton->hide();
            installedIcon->setIcon(u8"\U000F05E0");
            installedIcon->setTextColor(Ui::DesignSystem::color().accent());
            installedIcon->show();
            installedLabel->show();
        } else {
            installedIcon->hide();
            installedLabel->hide();
            installButton->show();
        }

        break;
    }

    case Downloading: {
        downloadingAction->setIcon(u8"\U000F0156");
        downloadingAction->setToolTip(tr("Cancel downloading"));

        installedIcon->hide();
        installedLabel->hide();
        installButton->hide();
        downloadingProgress->show();
        downloadingAction->show();

        break;
    }

    case DownloadingFailed: {
        installedIcon->setIcon(u8"\U000F0026");
        installedIcon->setTextColor(Ui::DesignSystem::color().error());
        installedLabel->setText(tr("Downloading failed"));
        downloadingAction->setIcon(u8"\U000F0450");
        downloadingAction->setToolTip(tr("Retry"));

        installedIcon->show();
        installedLabel->show();
        installButton->hide();
        downloadingProgress->hide();
        downloadingAction->show();

        break;
    }

    case Downloaded: {
        installedLabel->setText(tr("Downloaded"));
        installButton->setText(tr("Install"));

        downloadingProgress->hide();
        downloadingAction->hide();
        installedIcon->setIcon(u8"\U000F05E0");
        installedIcon->setTextColor(Ui::DesignSystem::color().accent());
        installedIcon->show();
        installedLabel->show();
        installButton->show();

        break;
    }
    }
}


// ****


ReleaseView::ReleaseView(QWidget* _parent, const Domain::Notification& _notification)
    : Widget(_parent)
    , d(new Implementation(this, _notification))
{
    auto titleLayout = new QVBoxLayout;
    titleLayout->setContentsMargins({});
    titleLayout->setSpacing(0);
    titleLayout->addWidget(d->dateTimeLabel);
    titleLayout->addWidget(d->titleLabel);

    auto topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(d->avatarLabel);
    topLayout->addLayout(titleLayout);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->bodyLabel);
    layout->addWidget(d->readMoreLink);
    layout->addLayout(d->buttonsLayout);
    setLayout(layout);

    connect(d->installButton, &Button::clicked, this, [this] {
        switch (d->state) {
        default:
        case Ready: {
            d->downloadUpdate();
            break;
        }

        case Downloaded: {
            d->installUpdate();
            break;
        }
        }
    });
    connect(d->downloadingAction, &IconButton::clicked, this, [this] {
        if (d->state == Downloading) {
            d->stopDownloading();
        } else if (d->state == DownloadingFailed) {
            d->downloadUpdate();
        }
    });
}

ReleaseView::~ReleaseView() = default;

void ReleaseView::updateTranslations()
{
    d->updateState();
}

void ReleaseView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    layout()->setContentsMargins(isLeftToRight() ? 0.0 : DesignSystem::scrollBar().minimumSize(), 0,
                                 isRightToLeft() ? 0.0 : DesignSystem::scrollBar().minimumSize(),
                                 0);

    for (auto widget : std::vector<Widget*>{
             this,
             d->avatarLabel,
             d->dateTimeLabel,
             d->titleLabel,
             d->bodyLabel,
             d->readMoreLink,
             d->installedIcon,
             d->installedLabel,
             d->downloadingProgress,
             d->downloadingAction,
         }) {
        widget->setBackgroundColor(Ui::DesignSystem::color().primary());
    }

    d->avatarLabel->setFixedSize(Ui::DesignSystem::layout().px(72),
                                 Ui::DesignSystem::layout().px(72));
    const auto leftMargin
        = isLeftToRight() ? Ui::DesignSystem::layout().px24() : Ui::DesignSystem::layout().px12();
    const auto rightMargin
        = isLeftToRight() ? Ui::DesignSystem::layout().px12() : Ui::DesignSystem::layout().px24();
    d->avatarLabel->setContentsMargins(leftMargin, Ui::DesignSystem::layout().px24(), rightMargin,
                                       Ui::DesignSystem::layout().px12());
    d->dateTimeLabel->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onPrimary(), Ui::DesignSystem::disabledTextOpacity()));
    d->dateTimeLabel->setAlignment(isLeftToRight() ? Qt::AlignLeft : Qt::AlignRight);
    d->dateTimeLabel->setContentsMargins(0, Ui::DesignSystem::layout().px24(), 0, 0);
    d->titleLabel->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->titleLabel->setContentsMargins(0, 0, 0, Ui::DesignSystem::layout().px12());
    d->bodyLabel->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->bodyLabel->setContentsMargins(leftMargin, 0, rightMargin, Ui::DesignSystem::layout().px12());
    d->readMoreLink->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onPrimary(), Ui::DesignSystem::disabledTextOpacity()));
    d->readMoreLink->setContentsMargins(leftMargin, 0, rightMargin, 0);
    d->installedIcon->setTextColor(Ui::DesignSystem::color().accent());
    d->installedIcon->setContentsMargins(
        isLeftToRight() ? Ui::DesignSystem::layout().px16() : Ui::DesignSystem::layout().px8(),
        Ui::DesignSystem::layout().px(17),
        isLeftToRight() ? Ui::DesignSystem::layout().px8() : Ui::DesignSystem::layout().px16(),
        Ui::DesignSystem::layout().px(11));
    d->installedLabel->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->installedLabel->setContentsMargins(
        isLeftToRight() ? 0 : Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px(15),
        isLeftToRight() ? Ui::DesignSystem::layout().px8() : 0, Ui::DesignSystem::layout().px(9));
    d->downloadingAction->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->downloadingProgress->setContentsMargins(
        isLeftToRight() ? Ui::DesignSystem::layout().px16() : 0, 0,
        isLeftToRight() ? 0 : Ui::DesignSystem::layout().px16(), 0);
    d->installButton->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->installButton->setTextColor(Ui::DesignSystem::color().accent());
    d->installButton->setContentsMargins(0, Ui::DesignSystem::layout().px(6), 0,
                                         Ui::DesignSystem::layout().px4());

    d->buttonsLayout->setContentsMargins(
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px4(),
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px16());
}

} // namespace Ui

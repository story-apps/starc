#include "cover_generator_view.h"

#include "cover_generator_sidebar.h"

#include <3rd_party/webloader/src/NetworkRequestLoader.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/image/image_cropping_dialog.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QFileDialog>
#include <QPainter>
#include <QResizeEvent>
#include <QSettings>
#include <QStandardPaths>


namespace Ui {

namespace {
const QSizeF kCoverSize(5.4, 8);
const QString kImagesPathKey = QLatin1String("widgets/image-card/files-path");
} // namespace

class CoverGeneratorView::Implementation
{
public:
    explicit Implementation(CoverGeneratorView* _q);

    /**
     * @brief Обновить настройки UI панели инструментов
     */
    void updateToolbarUi();
    void updateToolbarPositon();

    /**
     * @brief Загрузить изображение по ссылке или из файла
     */
    void loadImage(const QString& _url);

    /**
     * @brief Обрезать заданное изображение
     */
    void cropImage(const QPixmap& _image);

    /**
     * @brief Растянуть карточку обложки на всю доступную область
     */
    void updateCoverCardSize();

    /**
     * @brief Перерисовать обложку
     */
    void updateCover();


    CoverGeneratorView* q = nullptr;

    Splitter* splitter = nullptr;

    FloatingToolBar* toolbar = nullptr;
    QAction* saveAction = nullptr;
    QAction* discardAction = nullptr;
    QAction* clearAction = nullptr;
    QAction* saveToFileAction = nullptr;

    Widget* coverBackground = nullptr;
    Card* coverCard = nullptr;
    ImageLabel* coverImage = nullptr;
    QPixmap cover;

    CoverGeneratorSidebar* sidebar = nullptr;
    Debouncer coverParametersDebouncer;
    QString imageCopyright;
    QPixmap backgroundImage;
};

CoverGeneratorView::Implementation::Implementation(CoverGeneratorView* _q)
    : q(_q)
    , splitter(new Splitter(_q))
    , toolbar(new FloatingToolBar(_q))
    , saveAction(new QAction(_q))
    , discardAction(new QAction(_q))
    , clearAction(new QAction(_q))
    , saveToFileAction(new QAction(_q))
    , coverBackground(new Widget(_q))
    , coverCard(new Card(_q))
    , coverImage(new ImageLabel(_q))
    , sidebar(new CoverGeneratorSidebar(_q))
    , coverParametersDebouncer(300)
{
    splitter->setWidgets(coverBackground, sidebar);
    splitter->setSizes({ 7, 3 });
    splitter->setHidePanelButtonAvailable(true, false);

    toolbar->setParent(coverBackground);
    toolbar->setCurtain(true);

    saveAction->setIconText(u8"\U000F012C");
    toolbar->addAction(saveAction);
    //
    discardAction->setIconText(u8"\U000F0156");
    toolbar->addAction(discardAction);
    //
    auto separatorAction = new QAction;
    separatorAction->setSeparator(true);
    toolbar->addAction(separatorAction);
    //
    clearAction->setIconText(u8"\U000F00E2");
    toolbar->addAction(clearAction);
    //
    saveToFileAction->setIconText(u8"\U000F0193");
    toolbar->addAction(saveToFileAction);

    auto coverCardLayout = new QHBoxLayout;
    coverCardLayout->setContentsMargins({});
    coverCardLayout->setSpacing(0);
    coverCardLayout->addWidget(coverImage);
    coverCard->setContentLayout(coverCardLayout);

    auto coverLayout = new QHBoxLayout;
    coverLayout->setContentsMargins({});
    coverLayout->setSpacing(0);
    coverLayout->addWidget(coverCard, 0, Qt::AlignCenter);
    coverBackground->setLayout(coverLayout);
}

void CoverGeneratorView::Implementation::updateToolbarUi()
{
    toolbar->resize(toolbar->sizeHint());
    updateToolbarPositon();
    toolbar->setBackgroundColor(ColorHelper::nearby(Ui::DesignSystem::color().background()));
    toolbar->setTextColor(Ui::DesignSystem::color().onBackground());
    toolbar->raise();
}

void CoverGeneratorView::Implementation::updateToolbarPositon()
{
    toolbar->move(QPointF((coverBackground->width() - toolbar->width()) / 2.0,
                          -Ui::DesignSystem::card().shadowMargins().top())
                      .toPoint());
}

void CoverGeneratorView::Implementation::loadImage(const QString& _url)
{
    //
    // Обрабатываем ссылки
    //
    if (_url.startsWith("http")) {
        NetworkRequest* request = new NetworkRequest;
        connect(request, &NetworkRequest::downloadComplete, q,
                [this](const QByteArray& _imageData) {
                    QPixmap image;
                    image.loadFromData(_imageData);
                    cropImage(image);
                });
        connect(request, &NetworkRequest::downloadProgress, q,
                [_url](int _progress) { TaskBar::setTaskProgress(_url, _progress); });
        connect(request, &NetworkRequest::finished, request, [request, _url] {
            TaskBar::finishTask(_url);
            request->deleteLater();
        });
        TaskBar::addTask(_url);
        TaskBar::setTaskTitle(_url, tr("Loading image"));
        request->loadAsync(_url);
        return;
    }

    //
    // Обрабатываем локальные файлы
    //
    if (!QFile::exists(_url)) {
        return;
    }
    cropImage(QPixmap(_url));
}

void CoverGeneratorView::Implementation::cropImage(const QPixmap& _image)
{
    if (_image.isNull()) {
        return;
    }

    auto dlg = new ImageCroppingDialog(q->topLevelWidget());
    dlg->setImage(_image);
    dlg->setImageProportion(kCoverSize);
    dlg->setImageProportionFixed(true);
    dlg->setImageCroppingText(tr("Select an area for cover background"));
    if (!imageCopyright.isEmpty()) {
        dlg->setImageCroppingNote(imageCopyright);
        imageCopyright.clear();
    }
    connect(dlg, &ImageCroppingDialog::disappeared, dlg, &ImageCroppingDialog::deleteLater);
    connect(dlg, &ImageCroppingDialog::imageSelected, q, [this](const QPixmap& _image) {
        backgroundImage = _image;
        updateCover();
    });

    dlg->showDialog();
}

void CoverGeneratorView::Implementation::updateCoverCardSize()
{
    coverImage->setFixedSize(
        kCoverSize
            .scaled(coverBackground->width() - Ui::DesignSystem::layout().px(128),
                    coverBackground->height() - Ui::DesignSystem::layout().px(128),
                    Qt::KeepAspectRatio)
            .toSize());
}

void CoverGeneratorView::Implementation::updateCover()
{
    const QSizeF coverSize(MeasurementHelper::inchToPx(kCoverSize.width()),
                           MeasurementHelper::inchToPx(kCoverSize.height()));
    cover = QPixmap(coverSize.toSize());

    QPainter painter(&cover);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    //
    // Рисуем фон
    //
    if (!backgroundImage.isNull()) {
        painter.drawPixmap(0, 0, coverSize.width(), coverSize.height(), backgroundImage);
    } else {
        cover.fill(Qt::white);
    }

    //
    // Рисуем оверлей над изображением
    //
    if (const auto textBackgroundColor = sidebar->textBackgroundColor();
        textBackgroundColor.isValid()) {
        painter.fillRect(cover.rect(), textBackgroundColor);
    }

    //
    // Рисуем текст
    //
    const auto margin = MeasurementHelper::inchToPx(0.4);
    const auto availableWidth = coverSize.width() - margin * 2;
    auto paintText = [&painter, margin, availableWidth](const CoverTextParameters& _parameters,
                                                        qreal _top = -1, qreal _bottom = -1) {
        painter.setFont(_parameters.font);
        painter.setPen(_parameters.color);
        const auto height
            = TextHelper::heightForWidth(_parameters.text, _parameters.font, availableWidth);
        const QRectF rect(margin, _top > 0 ? _top : _bottom - height, availableWidth, height);
        painter.drawText(rect, Qt::TextWordWrap | Qt::AlignVCenter | _parameters.align,
                         _parameters.text);

        return rect;
    };
    auto textRect = paintText(sidebar->top1Text(), margin);
    paintText(sidebar->top2Text(), textRect.bottom() + 16);
    //
    auto bottomPos = coverSize.height() - margin;
    textRect = paintText(sidebar->websiteText(), -1, bottomPos);
    bottomPos = textRect.top() - 16;
    textRect = paintText(sidebar->releaseDateText(), -1, bottomPos);
    bottomPos = textRect.top() - 16;
    textRect = paintText(sidebar->creditsText(), -1, bottomPos);
    bottomPos = textRect.top() - 124;
    textRect = paintText(sidebar->afterNameText(), -1, bottomPos);
    bottomPos = textRect.top() - 12;
    textRect = paintText(sidebar->nameText(), -1, bottomPos);
    bottomPos = textRect.top() - 14;
    textRect = paintText(sidebar->beforeNameText(), -1, bottomPos);

    coverImage->setImage(cover);
}


// ****


CoverGeneratorView::CoverGeneratorView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    d->coverBackground->installEventFilter(this);

    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->splitter);
    setLayout(layout);


    connect(d->saveAction, &QAction::triggered, this, &CoverGeneratorView::savePressed);
    connect(d->discardAction, &QAction::triggered, this, &CoverGeneratorView::discardPressed);
    connect(d->clearAction, &QAction::triggered, this, [this] {
        d->sidebar->clear();
        d->backgroundImage = {};
        d->updateCover();
    });
    connect(d->saveToFileAction, &QAction::triggered, this, [this] {
        const auto imagesFolder
            = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        auto imagePath = QFileDialog::getSaveFileName(
            this, tr("Select location for saving cover to file"), imagesFolder,
            QString("%1 (*.png)").arg(tr("PNG image")));
        if (imagePath.isEmpty()) {
            return;
        }

        if (!imagePath.endsWith(".png", Qt::CaseInsensitive)) {
            imagePath.append(".png");
        }
        d->cover.save(imagePath, "PNG");
    });
    //
    connect(d->sidebar, &CoverGeneratorSidebar::textBackgroundColorChanged, this,
            [this] { d->updateCover(); });
    connect(d->sidebar, &CoverGeneratorSidebar::textParametersChanged, &d->coverParametersDebouncer,
            &Debouncer::orderWork);
    connect(&d->coverParametersDebouncer, &Debouncer::gotWork, this, [this] { d->updateCover(); });
    connect(d->sidebar, &CoverGeneratorSidebar::unsplashImageSelected, this,
            [this](const QString& _url, const QString& _copyright) {
                d->imageCopyright = _copyright;
                d->loadImage(_url);
            });
    connect(d->sidebar, &CoverGeneratorSidebar::pasteImageFromClipboardPressed, this, [this] {
        const auto image = QApplication::clipboard()->pixmap();
        if (!image.isNull()) {
            d->cropImage(image);
            return;
        }

        d->loadImage(QApplication::clipboard()->text());
    });
    connect(d->sidebar, &CoverGeneratorSidebar::chooseImgeFromFilePressed, this, [this] {
        QSettings settings;
        const auto imagesFolder = settings.value(kImagesPathKey).toString();
        const auto imagePath = QFileDialog::getOpenFileName(
            this, tr("Select file for background image"), imagesFolder,
            QString("%1 (*.png *.jpeg *.jpg *.bmp *.tiff *.tif *.gif)").arg(tr("Images")));
        if (imagePath.isEmpty()) {
            return;
        }
        settings.setValue(kImagesPathKey, imagePath);

        d->loadImage(imagePath);
    });
}

CoverGeneratorView::~CoverGeneratorView() = default;

const QPixmap& CoverGeneratorView::coverImage() const
{
    return d->cover;
}

bool CoverGeneratorView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->coverBackground && _event->type() == QEvent::Resize) {
        d->updateCoverCardSize();
        d->updateToolbarPositon();
    }

    return Widget::eventFilter(_watched, _event);
}

void CoverGeneratorView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->updateToolbarPositon();
}

void CoverGeneratorView::updateTranslations()
{
    d->saveAction->setToolTip(tr("Use cover for the project"));
    d->discardAction->setToolTip(tr("Close without saving"));
    d->clearAction->setToolTip(tr("Clear cover parameters"));
    d->saveToFileAction->setToolTip(tr("Save cover to the file"));
}

void CoverGeneratorView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->updateToolbarUi();

    setBackgroundColor(Ui::DesignSystem::color().surface());
    d->coverBackground->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->coverCard->setBackgroundColor(Ui::DesignSystem::color().background());
    d->coverImage->setBackgroundColor(Ui::DesignSystem::color().background());
    d->updateCoverCardSize();
}

} // namespace Ui

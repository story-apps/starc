#include "cover_generator_view.h"

#include "cover_generator_sidebar.h"

#include <3rd_party/webloader/src/NetworkRequestLoader.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/splitter/splitter.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/debouncer.h>

#include <QBoxLayout>
#include <QPainter>
#include <QResizeEvent>


namespace Ui {

namespace {
const QSizeF kCoverSize(5.4, 8);
}

class CoverGeneratorView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Растянуть карточку обложки на всю доступную область
     */
    void updateCoverCardSize();

    /**
     * @brief Перерисовать обложку
     */
    void updateCover();


    Splitter* splitter = nullptr;

    FloatingToolBar* toolbar = nullptr;

    Widget* coverBackground = nullptr;
    Card* coverCard = nullptr;
    ImageLabel* coverImage = nullptr;
    QPixmap cover;

    CoverGeneratorSidebar* sidebar = nullptr;
    Debouncer coverParametersDebouncer;
};

CoverGeneratorView::Implementation::Implementation(QWidget* _parent)
    : splitter(new Splitter(_parent))
    , coverBackground(new Widget(_parent))
    , coverCard(new Card(_parent))
    , coverImage(new ImageLabel(_parent))
    , sidebar(new CoverGeneratorSidebar(_parent))
    , coverParametersDebouncer(300)
{
    splitter->setWidgets(coverBackground, sidebar);
    splitter->setSizes({ 7, 3 });

    auto coverCardLayout = new QHBoxLayout;
    coverCardLayout->setContentsMargins({});
    coverCardLayout->setSpacing(0);
    coverCardLayout->addWidget(coverImage);
    coverCard->setLayoutReimpl(coverCardLayout);

    auto coverLayout = new QHBoxLayout;
    coverLayout->setContentsMargins({});
    coverLayout->setSpacing(0);
    coverLayout->addWidget(coverCard, 0, Qt::AlignCenter);
    coverBackground->setLayout(coverLayout);
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
    if (!sidebar->backgroundImage().isNull()) {
        painter.drawPixmap(0, 0, coverSize.width(), coverSize.height(), sidebar->backgroundImage());
    } else {
        cover.fill(coverImage->backgroundColor());
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
        painter.drawText(
            rect, Qt::TextWordWrap | Qt::TextWrapAnywhere | Qt::AlignVCenter | _parameters.align,
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


    connect(d->sidebar, &CoverGeneratorSidebar::coverParametersChanged,
            &d->coverParametersDebouncer, &Debouncer::orderWork);
    connect(&d->coverParametersDebouncer, &Debouncer::gotWork, this, [this] { d->updateCover(); });
}

CoverGeneratorView::~CoverGeneratorView()
{
}

bool CoverGeneratorView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->coverBackground && _event->type() == QEvent::Resize) {
        d->updateCoverCardSize();
    }

    return Widget::eventFilter(_watched, _event);
}

void CoverGeneratorView::updateTranslations()
{
}

void CoverGeneratorView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());
    d->coverBackground->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->coverCard->setBackgroundColor(Ui::DesignSystem::color().background());
    d->coverImage->setBackgroundColor(Ui::DesignSystem::color().background());
    d->updateCoverCardSize();
}

} // namespace Ui

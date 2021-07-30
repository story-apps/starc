#include "screenplay_template_manager.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/settings/screenplay_template/screenplay_template_navigator.h>
#include <ui/settings/screenplay_template/screenplay_template_page_view.h>
#include <ui/settings/screenplay_template/screenplay_template_paragraphs_view.h>
#include <ui/settings/screenplay_template/screenplay_template_tool_bar.h>
#include <ui/settings/screenplay_template/screenplay_template_view_tool_bar.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <utils/helpers/measurement_helper.h>

namespace ManagementLayer {

class ScreenplayTemplateManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить заданное значение в текущей метрике
     */
    qreal mmToCurrentMetrics(qreal _value) const;
    QMarginsF mmMarginsToCurrentMetrics(const QMarginsF& _margins) const;

    /**
     * @brief Получить значение в миллиметрах из текущей метрики
     */
    qreal mmFromCurrentMetrics(qreal _value) const;
    QMarginsF mmMarginsFromCurrentMetrics(const QMarginsF& _margins) const;

    /**
     * @brief Обновить параметры страницы в представлении
     */
    void updatePageParameters();

    /**
     * @brief Сохранить параметры страницы шаблона
     */
    void savePageParameters();

    /**
     * @brief Обновить параметры параграфа в представлении
     */
    void updateParagraphParameters(BusinessLayer::ScreenplayParagraphType _paragraphType);

    /**
     * @brief Сохранить параметры заданного параграфа
     */
    void saveParagraphParameters(BusinessLayer::ScreenplayParagraphType _paragraphType);


    /**
     * @brief Редактируемый шаблон
     */
    BusinessLayer::ScreenplayTemplate currentTemplate;

    /**
     * @brief Используются ли миллиметры (true) или дюймы (false)
     */
    bool useMm = true;

    Ui::ScreenplayTemplateToolBar* toolBar = nullptr;
    Ui::ScreenplayTemplateNavigator* navigator = nullptr;
    Ui::ScreenplayTemplatePageView* pageView = nullptr;
    Widget* titlePageView = nullptr;
    Ui::ScreenplayTemplateParagraphsView* paragraphsView = nullptr;
    Ui::ScreenplayTemplateViewToolBar* viewToolBar = nullptr;
};

ScreenplayTemplateManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new Ui::ScreenplayTemplateToolBar(_parent))
    , navigator(new Ui::ScreenplayTemplateNavigator(_parent))
    , pageView(new Ui::ScreenplayTemplatePageView(_parent))
    , titlePageView(new Widget(_parent))
    , paragraphsView(new Ui::ScreenplayTemplateParagraphsView(_parent))
    , viewToolBar(new Ui::ScreenplayTemplateViewToolBar(_parent))
{
    toolBar->hide();
    navigator->hide();
    pageView->hide();
    titlePageView->hide();
    paragraphsView->hide();
    viewToolBar->hide();
}

qreal ScreenplayTemplateManager::Implementation::mmToCurrentMetrics(qreal _value) const
{
    return useMm ? _value : MeasurementHelper::mmToInch(_value);
}

QMarginsF ScreenplayTemplateManager::Implementation::mmMarginsToCurrentMetrics(
    const QMarginsF& _margins) const
{
    return { mmToCurrentMetrics(_margins.left()), mmToCurrentMetrics(_margins.top()),
             mmToCurrentMetrics(_margins.right()), mmToCurrentMetrics(_margins.bottom()) };
}

qreal ScreenplayTemplateManager::Implementation::mmFromCurrentMetrics(qreal _value) const
{
    return useMm ? _value : MeasurementHelper::inchToMm(_value);
}

QMarginsF ScreenplayTemplateManager::Implementation::mmMarginsFromCurrentMetrics(
    const QMarginsF& _margins) const
{
    return { mmFromCurrentMetrics(_margins.left()), mmFromCurrentMetrics(_margins.top()),
             mmFromCurrentMetrics(_margins.right()), mmFromCurrentMetrics(_margins.bottom()) };
}

void ScreenplayTemplateManager::Implementation::updatePageParameters()
{
    pageView->setTemplateName(currentTemplate.name());
    pageView->setPageSize(currentTemplate.pageSizeId());
    pageView->setPageMargins(mmMarginsToCurrentMetrics(currentTemplate.pageMargins()));
    pageView->setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
    pageView->setLeftHalfOfPage(currentTemplate.leftHalfOfPageWidthPercents());
}

void ScreenplayTemplateManager::Implementation::savePageParameters()
{
    currentTemplate.setName(pageView->templateName());
    currentTemplate.setPageSizeId(pageView->pageSizeId());
    currentTemplate.setPageMargins(mmMarginsFromCurrentMetrics(pageView->pageMargins()));
    currentTemplate.setPageNumbersAlignment(pageView->pageNumbersAlignment());
    currentTemplate.setLeftHalfOfPageWidthPercents(pageView->leftHalfOfPageWidthPercents());
}

void ScreenplayTemplateManager::Implementation::updateParagraphParameters(
    BusinessLayer::ScreenplayParagraphType _paragraphType)
{
    const auto paragraphStyle = currentTemplate.paragraphStyle(_paragraphType);
    paragraphsView->setParagraphEnabled(paragraphStyle.isActive());
    paragraphsView->setFontFamily(paragraphStyle.font().family());
    paragraphsView->setFontSize(MeasurementHelper::pxToPt(paragraphStyle.font().pixelSize()));
    paragraphsView->setStartsFromNewPage(paragraphStyle.isStartFromNewPage());
    paragraphsView->setUppercase(paragraphStyle.font().capitalization() == QFont::AllUppercase);
    paragraphsView->setBold(paragraphStyle.font().bold());
    paragraphsView->setItalic(paragraphStyle.font().italic());
    paragraphsView->setUndeline(paragraphStyle.font().underline());
    paragraphsView->setAlignment(paragraphStyle.align());
    if (paragraphStyle.margins().top() > 0 || paragraphStyle.margins().bottom() > 0) {
        paragraphsView->setTopIndent(mmToCurrentMetrics(paragraphStyle.margins().top()));
        paragraphsView->setBottomIndent(mmToCurrentMetrics(paragraphStyle.margins().bottom()));
        paragraphsView->setVericalIndentationInLines(false);
    } else {
        paragraphsView->setTopIndent(paragraphStyle.linesBefore());
        paragraphsView->setBottomIndent(paragraphStyle.linesAfter());
        paragraphsView->setVericalIndentationInLines(true);
    }
    paragraphsView->setLeftIndent(mmToCurrentMetrics(paragraphStyle.margins().left()));
    paragraphsView->setRightIndent(mmToCurrentMetrics(paragraphStyle.margins().right()));
    paragraphsView->setLeftIndentInTable(
        mmToCurrentMetrics(paragraphStyle.marginsOnHalfPage().left()));
    paragraphsView->setRightIndentInTable(
        mmToCurrentMetrics(paragraphStyle.marginsOnHalfPage().right()));
    paragraphsView->setLineSpacingType(static_cast<int>(paragraphStyle.lineSpacingType()));
    paragraphsView->setLineSpacingValue(mmToCurrentMetrics(paragraphStyle.lineSpacingValue()));
}

void ScreenplayTemplateManager::Implementation::saveParagraphParameters(
    BusinessLayer::ScreenplayParagraphType _paragraphType)
{
    auto paragraphStyle = currentTemplate.paragraphStyle(_paragraphType);
    paragraphStyle.setActive(paragraphsView->isParagraphEnabled());
    paragraphStyle.setStartFromNewPage(paragraphsView->isStartsFromNewPage());
    QFont font(paragraphsView->fontFamily());
    font.setPixelSize(MeasurementHelper::ptToPx(paragraphsView->fontSize()));
    font.setCapitalization(paragraphsView->isUppercase() ? QFont::AllUppercase : QFont::MixedCase);
    font.setBold(paragraphsView->isBold());
    font.setItalic(paragraphsView->isItalic());
    font.setUnderline(paragraphsView->isUnderline());
    paragraphStyle.setFont(font);
    paragraphStyle.setAlign(paragraphsView->alignment());
    QMarginsF margins;
    if (paragraphsView->isVerticalIndentationInLines()) {
        paragraphStyle.setLinesBefore(paragraphsView->topIndent());
        paragraphStyle.setLinesAfter(paragraphsView->bottomIndent());
    } else {
        paragraphStyle.setLinesBefore(0);
        paragraphStyle.setLinesAfter(0);
        margins.setTop(mmFromCurrentMetrics(paragraphsView->topIndent()));
        margins.setBottom(mmFromCurrentMetrics(paragraphsView->bottomIndent()));
    }
    margins.setLeft(mmFromCurrentMetrics(paragraphsView->leftIndent()));
    margins.setRight(mmFromCurrentMetrics(paragraphsView->rightIndent()));
    paragraphStyle.setMargins(margins);
    QMarginsF marginsOnHalfPage = margins;
    marginsOnHalfPage.setLeft(mmFromCurrentMetrics(paragraphsView->leftIndentInTable()));
    marginsOnHalfPage.setRight(mmFromCurrentMetrics(paragraphsView->rightIndentInTable()));
    paragraphStyle.setMarginsOnHalfPage(marginsOnHalfPage);
    paragraphStyle.setLineSpacingType(
        static_cast<BusinessLayer::ScreenplayBlockStyle::LineSpacingType>(
            paragraphsView->lineSpacingType()));
    paragraphStyle.setLineSpacingValue(mmFromCurrentMetrics(paragraphsView->lineSpacingValue()));

    currentTemplate.setParagraphStyle(paragraphStyle);
}


// ****


ScreenplayTemplateManager::ScreenplayTemplateManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(_parentWidget))
{
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::backPressed, this,
            &ScreenplayTemplateManager::closeRequested);
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::pageSettingsPressed, this,
            [this] { emit showViewRequested(d->pageView); });
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::titlePageSettingsPressed, this,
            [this] { emit showViewRequested(d->titlePageView); });
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::paragraphSettingsPressed, this,
            [this] { emit showViewRequested(d->paragraphsView); });
    connect(d->navigator, &Ui::ScreenplayTemplateNavigator::mmCheckedChanged, this,
            [this](bool _mm) {
                if (d->useMm == _mm) {
                    return;
                }

                d->useMm = _mm;
                d->pageView->setUseMm(d->useMm);
                d->paragraphsView->setUseMm(d->useMm);
                d->updatePageParameters();
                d->updateParagraphParameters(d->paragraphsView->currentParagraphType());
            });
    connect(d->viewToolBar, &Ui::ScreenplayTemplateViewToolBar::savePressed, this, [this] {
        d->savePageParameters();
        d->saveParagraphParameters(d->paragraphsView->currentParagraphType());
        BusinessLayer::TemplatesFacade::saveScreenplayTemplate(d->currentTemplate);
    });
    connect(d->paragraphsView, &Ui::ScreenplayTemplateParagraphsView::currentParagraphTypeChanged,
            this,
            [this](BusinessLayer::ScreenplayParagraphType _currentType,
                   BusinessLayer::ScreenplayParagraphType _previousType) {
                d->saveParagraphParameters(_previousType);
                d->updateParagraphParameters(_currentType);
            });
}

ScreenplayTemplateManager::~ScreenplayTemplateManager() = default;

QWidget* ScreenplayTemplateManager::toolBar() const
{
    return d->toolBar;
}

QWidget* ScreenplayTemplateManager::navigator() const
{
    return d->navigator;
}

QWidget* ScreenplayTemplateManager::view() const
{
    return d->pageView;
}

QWidget* ScreenplayTemplateManager::viewToolBar() const
{
    return d->viewToolBar;
}

void ScreenplayTemplateManager::editTemplate(const QString& _templateId)
{
    d->toolBar->checkPageSettings();
    d->navigator->checkMm();
    d->paragraphsView->setCurrentParagraphType(
        BusinessLayer::ScreenplayParagraphType::SceneHeading);

    d->currentTemplate = BusinessLayer::TemplatesFacade::screenplayTemplate(_templateId);

    d->updatePageParameters();
    d->updateParagraphParameters(d->paragraphsView->currentParagraphType());
}

void ScreenplayTemplateManager::duplicateTemplate(const QString& _templateId)
{
    d->toolBar->checkPageSettings();
    d->navigator->checkMm();
    d->paragraphsView->setCurrentParagraphType(
        BusinessLayer::ScreenplayParagraphType::SceneHeading);

    d->currentTemplate = BusinessLayer::TemplatesFacade::screenplayTemplate(_templateId);
    d->currentTemplate.setIsNew();

    d->updatePageParameters();
    d->updateParagraphParameters(d->paragraphsView->currentParagraphType());
}

} // namespace ManagementLayer

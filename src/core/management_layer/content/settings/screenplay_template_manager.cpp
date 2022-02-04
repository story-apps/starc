#include "screenplay_template_manager.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <interfaces/ui/i_document_view.h>
#include <management_layer/plugins_builder.h>
#include <ui/design_system/design_system.h>
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
    explicit Implementation(QWidget* _parent, const PluginsBuilder& _pluginsBuilder);

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
     * @brief Обновить параметры титульной страницы
     */
    void updateTitlePageParameters();

    /**
     * @brief Сохранить титульную страницу
     */
    void saveTitlePage();

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
    Ui::ScreenplayTemplateParagraphsView* paragraphsView = nullptr;
    Ui::ScreenplayTemplateViewToolBar* viewToolBar = nullptr;

    const PluginsBuilder& pluginsBuilder;
    BusinessLayer::ScreenplayInformationModel informationModel;
    BusinessLayer::ScreenplayTitlePageModel titlePageModel;
    QScopedPointer<Domain::DocumentObject> titlePageDocument;
    QWidget* titlePageView = nullptr;
};

ScreenplayTemplateManager::Implementation::Implementation(QWidget* _parent,
                                                          const PluginsBuilder& _pluginsBuilder)
    : toolBar(new Ui::ScreenplayTemplateToolBar(_parent))
    , navigator(new Ui::ScreenplayTemplateNavigator(_parent))
    , pageView(new Ui::ScreenplayTemplatePageView(_parent))
    , paragraphsView(new Ui::ScreenplayTemplateParagraphsView(_parent))
    , viewToolBar(new Ui::ScreenplayTemplateViewToolBar(_parent))
    , pluginsBuilder(_pluginsBuilder)
    , titlePageDocument(Domain::ObjectsBuilder::createDocument(
          {}, {}, Domain::DocumentObjectType::ScreenplayTitlePage, {}))
{
    toolBar->hide();
    navigator->hide();
    pageView->hide();
    titlePageModel.setDocument(titlePageDocument.data());
    titlePageModel.setInformationModel(&informationModel);
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

void ScreenplayTemplateManager::Implementation::updateTitlePageParameters()
{
    titlePageModel.setDocument(nullptr);
    informationModel.setOverrideCommonSettings(true);
    informationModel.setTemplateId(currentTemplate.id());
    titlePageDocument->setContent(currentTemplate.titlePage().toUtf8());
    titlePageModel.setDocument(titlePageDocument.data());
}

void ScreenplayTemplateManager::Implementation::saveTitlePage()
{
    QString titlePageXml = titlePageDocument->content();
    titlePageXml.remove(QLatin1String("<?xml version=\"1.0\"?>"));
    currentTemplate.setTitlePage(titlePageXml);
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


ScreenplayTemplateManager::ScreenplayTemplateManager(QObject* _parent, QWidget* _parentWidget,
                                                     const PluginsBuilder& _pluginsBuilder)
    : QObject(_parent)
    , d(new Implementation(_parentWidget, _pluginsBuilder))
{
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::backPressed, this,
            &ScreenplayTemplateManager::closeRequested);
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::pageSettingsPressed, this,
            [this] { emit showViewRequested(d->pageView); });
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::titlePageSettingsPressed, this, [this] {
        //
        // Получим редактор титульной страницы
        //
        d->titlePageView = d->pluginsBuilder
                               .activateView("application/x-starc/editor/screenplay/title-page",
                                             &d->titlePageModel)
                               ->asQWidget();

        //
        // Смещаем тулбар, чтобы он не наезжал на тулбар редактора шаблона
        //
        FloatingToolBar* titlePageToolbar = nullptr;
        const auto titlePageToolbars = d->titlePageView->findChildren<FloatingToolBar*>();
        for (auto toolbar : titlePageToolbars) {
            if (toolbar->actions().size() > 0) {
                titlePageToolbar = toolbar;
                break;
            }
        }
        Q_ASSERT(titlePageToolbar);
        //
        // ... сначала скрываем, а потом, после того, как размер виджета скорректируется,
        //     смещаем тулбар редактора титульной страницы
        //
        titlePageToolbar->hide();
        QMetaObject::invokeMethod(
            this,
            [this, titlePageToolbar] {
                titlePageToolbar->move(d->viewToolBar->geometry().right()
                                           + Ui::DesignSystem::layout().px12(),
                                       titlePageToolbar->y());
                titlePageToolbar->show();
            },
            Qt::QueuedConnection);

        //
        // Собственно запросим отображение редактора титульной страницы
        //
        emit showViewRequested(d->titlePageView);
    });
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
        d->saveTitlePage();
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
    d->updateTitlePageParameters();
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
    d->updateTitlePageParameters();
    d->updateParagraphParameters(d->paragraphsView->currentParagraphType());
}

} // namespace ManagementLayer

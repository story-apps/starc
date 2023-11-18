#include "template_options_manager.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <interfaces/ui/i_document_view.h>
#include <management_layer/plugins_builder.h>
#include <ui/design_system/design_system.h>
#include <ui/settings/screenplay_template/screenplay_template_navigator.h>
#include <ui/settings/screenplay_template/screenplay_template_page_view.h>
#include <ui/settings/screenplay_template/screenplay_template_paragraphs_view.h>
#include <ui/settings/screenplay_template/screenplay_template_tool_bar.h>
#include <ui/settings/screenplay_template/screenplay_template_view_tool_bar.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <utils/helpers/measurement_helper.h>

#include <QTimer>

#include <variant>


namespace ManagementLayer {

class TemplateOptionsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent, const PluginsBuilder& _pluginsBuilder);

    /**
     * @brief Подготовить представление к редактированию заданного шаблона
     */
    void prepareViewToEdit(const QString& _templateId, bool _isNewTemplate);

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
    void updateParagraphParameters(BusinessLayer::TextParagraphType _paragraphType);

    /**
     * @brief Сохранить параметры заданного параграфа
     */
    void saveParagraphParameters(BusinessLayer::TextParagraphType _paragraphType);

    /**
     * @brief Сохранить шаблон
     */
    void saveTemplate();


    /**
     * @brief Редактируемый шаблон
     */
    struct {
        void clear()
        {
            *this = {};
        }

        BusinessLayer::TextTemplate& get()
        {
            if (screenplay.isValid()) {
                return screenplay;
            } else if (comicBook.isValid()) {
                return comicBook;
            } else if (audioplay.isValid()) {
                return audioplay;
            } else if (stageplay.isValid()) {
                return stageplay;
            } else if (novel.isValid()) {
                return novel;
            } else {
                return simpleText;
            }
        }

        BusinessLayer::SimpleTextTemplate simpleText;
        BusinessLayer::ScreenplayTemplate screenplay;
        BusinessLayer::ComicBookTemplate comicBook;
        BusinessLayer::AudioplayTemplate audioplay;
        BusinessLayer::StageplayTemplate stageplay;
        BusinessLayer::NovelTemplate novel;
    } currentTemplate;

    /**
     * @brief Изменён ли текущий шаблон
     */
    bool currentTemplateChanged = false;

    /**
     * @brief Используются ли миллиметры (true) или дюймы (false)
     */
    bool useMm = true;

    Ui::ScreenplayTemplateToolBar* toolBar = nullptr;
    Ui::ScreenplayTemplateNavigator* navigator = nullptr;
    Ui::ScreenplayTemplatePageView* pageView = nullptr;
    Ui::ScreenplayTemplateParagraphsView* paragraphsView = nullptr;
    Ui::ScreenplayTemplateViewToolBar* viewToolBar = nullptr;

    Domain::DocumentObjectType currentDocumentType = Domain::DocumentObjectType::Undefined;

    const PluginsBuilder& pluginsBuilder;
    BusinessLayer::ScreenplayInformationModel informationModel;
    BusinessLayer::ScreenplayTitlePageModel titlePageModel;
    QScopedPointer<Domain::DocumentObject> titlePageDocument;
    QWidget* titlePageView = nullptr;
};

TemplateOptionsManager::Implementation::Implementation(QWidget* _parent,
                                                       const PluginsBuilder& _pluginsBuilder)
    : toolBar(new Ui::ScreenplayTemplateToolBar(_parent))
    , navigator(new Ui::ScreenplayTemplateNavigator(_parent))
    , pageView(new Ui::ScreenplayTemplatePageView(_parent))
    , paragraphsView(new Ui::ScreenplayTemplateParagraphsView(_parent))
    , viewToolBar(new Ui::ScreenplayTemplateViewToolBar(_parent))
    , pluginsBuilder(_pluginsBuilder)
    , titlePageDocument(Domain::ObjectsBuilder::createDocument(
          {}, {}, Domain::DocumentObjectType::ScreenplayTitlePage, {}, {}))
{
    toolBar->hide();
    navigator->hide();
    pageView->hide();
    titlePageModel.setDocument(titlePageDocument.data());
    titlePageModel.setInformationModel(&informationModel);
    paragraphsView->hide();
    viewToolBar->hide();
}

void TemplateOptionsManager::Implementation::prepareViewToEdit(const QString& _templateId,
                                                               bool _isNewTemplate)
{
    toolBar->checkPageSettings();
    navigator->checkMm();
    paragraphsView->selectFirstParagraphTypeTab();

    currentTemplate.clear();
    switch (currentDocumentType) {
    default:
    case Domain::DocumentObjectType::SimpleText: {
        currentTemplate.simpleText
            = BusinessLayer::TemplatesFacade::simpleTextTemplate(_templateId);
        break;
    }

    case Domain::DocumentObjectType::Screenplay: {
        currentTemplate.screenplay
            = BusinessLayer::TemplatesFacade::screenplayTemplate(_templateId);
        break;
    }

    case Domain::DocumentObjectType::ComicBook: {
        currentTemplate.comicBook = BusinessLayer::TemplatesFacade::comicBookTemplate(_templateId);
        break;
    }

    case Domain::DocumentObjectType::Audioplay: {
        currentTemplate.audioplay = BusinessLayer::TemplatesFacade::audioplayTemplate(_templateId);
        break;
    }

    case Domain::DocumentObjectType::Stageplay: {
        currentTemplate.stageplay = BusinessLayer::TemplatesFacade::stageplayTemplate(_templateId);
        break;
    }

    case Domain::DocumentObjectType::Novel: {
        currentTemplate.novel = BusinessLayer::TemplatesFacade::novelTemplate(_templateId);
        break;
    }
    }
    if (_isNewTemplate) {
        currentTemplate.get().setIsNew();
    }

    updatePageParameters();
    updateTitlePageParameters();
    updateParagraphParameters(paragraphsView->currentParagraphType());
}

qreal TemplateOptionsManager::Implementation::mmToCurrentMetrics(qreal _value) const
{
    return useMm ? _value : MeasurementHelper::mmToInch(_value);
}

QMarginsF TemplateOptionsManager::Implementation::mmMarginsToCurrentMetrics(
    const QMarginsF& _margins) const
{
    return { mmToCurrentMetrics(_margins.left()), mmToCurrentMetrics(_margins.top()),
             mmToCurrentMetrics(_margins.right()), mmToCurrentMetrics(_margins.bottom()) };
}

qreal TemplateOptionsManager::Implementation::mmFromCurrentMetrics(qreal _value) const
{
    return useMm ? _value : MeasurementHelper::inchToMm(_value);
}

QMarginsF TemplateOptionsManager::Implementation::mmMarginsFromCurrentMetrics(
    const QMarginsF& _margins) const
{
    return { mmFromCurrentMetrics(_margins.left()), mmFromCurrentMetrics(_margins.top()),
             mmFromCurrentMetrics(_margins.right()), mmFromCurrentMetrics(_margins.bottom()) };
}

void TemplateOptionsManager::Implementation::updatePageParameters()
{
    pageView->setTemplateName(currentTemplate.get().name());
    pageView->setPageSize(currentTemplate.get().pageSizeId());
    pageView->setPageMargins(mmMarginsToCurrentMetrics(currentTemplate.get().pageMargins()));
    pageView->setPageNumbersAlignment(currentTemplate.get().pageNumbersAlignment());
    pageView->setLeftHalfOfPage(currentTemplate.get().leftHalfOfPageWidthPercents());
}

void TemplateOptionsManager::Implementation::savePageParameters()
{
    currentTemplate.get().setName(pageView->templateName());
    currentTemplate.get().setPageSizeId(pageView->pageSizeId());
    currentTemplate.get().setPageMargins(mmMarginsFromCurrentMetrics(pageView->pageMargins()));
    currentTemplate.get().setPageNumbersAlignment(pageView->pageNumbersAlignment());
    currentTemplate.get().setLeftHalfOfPageWidthPercents(pageView->leftHalfOfPageWidthPercents());
}

void TemplateOptionsManager::Implementation::updateTitlePageParameters()
{
    titlePageModel.setDocument(nullptr);
    informationModel.setOverrideCommonSettings(true);
    informationModel.setTemplateId(currentTemplate.get().id());
    titlePageDocument->setContent(currentTemplate.get().titlePage().toUtf8());
    titlePageModel.setDocument(titlePageDocument.data());
}

void TemplateOptionsManager::Implementation::saveTitlePage()
{
    QString titlePageXml = titlePageDocument->content();
    titlePageXml.remove(QLatin1String("<?xml version=\"1.0\"?>"));
    currentTemplate.get().setTitlePage(titlePageXml);
}

void TemplateOptionsManager::Implementation::updateParagraphParameters(
    BusinessLayer::TextParagraphType _paragraphType)
{
    const auto& paragraphStyle = currentTemplate.get().paragraphStyle(_paragraphType);
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
    paragraphsView->setFirstLineIndentIndent(mmToCurrentMetrics(paragraphStyle.firstLineMargin()));
    paragraphsView->setRightIndent(mmToCurrentMetrics(paragraphStyle.margins().right()));
    paragraphsView->setLeftIndentInTable(
        mmToCurrentMetrics(paragraphStyle.marginsOnHalfPage().left()));
    paragraphsView->setRightIndentInTable(
        mmToCurrentMetrics(paragraphStyle.marginsOnHalfPage().right()));
    paragraphsView->setLineSpacingType(static_cast<int>(paragraphStyle.lineSpacingType()));
    paragraphsView->setLineSpacingValue(mmToCurrentMetrics(paragraphStyle.lineSpacingValue()));
    paragraphsView->setShowParagraphTitle(paragraphStyle.isTitleVisible());
    paragraphsView->setCustomParagraphTitle(paragraphStyle.title());
}

void TemplateOptionsManager::Implementation::saveParagraphParameters(
    BusinessLayer::TextParagraphType _paragraphType)
{
    auto paragraphStyle = currentTemplate.get().paragraphStyle(_paragraphType);
    paragraphStyle.setActive(paragraphsView->isParagraphEnabled());
    paragraphStyle.setStartFromNewPage(paragraphsView->isStartsFromNewPage());
    QFont font(paragraphsView->fontFamily());
    const QStringList fontFamilies = {
        font.family(),
        QLatin1String("Apple Color Emoji"),
        QLatin1String("Noto Color Emoji"),
    };
    font.setFamilies(fontFamilies);
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
    paragraphStyle.setFirstLineMargin(mmFromCurrentMetrics(paragraphsView->firstLineIndent()));
    margins.setRight(mmFromCurrentMetrics(paragraphsView->rightIndent()));
    paragraphStyle.setMargins(margins);
    QMarginsF marginsOnHalfPage = margins;
    marginsOnHalfPage.setLeft(mmFromCurrentMetrics(paragraphsView->leftIndentInTable()));
    marginsOnHalfPage.setRight(mmFromCurrentMetrics(paragraphsView->rightIndentInTable()));
    paragraphStyle.setMarginsOnHalfPage(marginsOnHalfPage);
    paragraphStyle.setLineSpacingType(static_cast<BusinessLayer::TextBlockStyle::LineSpacingType>(
        paragraphsView->lineSpacingType()));
    paragraphStyle.setLineSpacingValue(mmFromCurrentMetrics(paragraphsView->lineSpacingValue()));
    paragraphStyle.setTitleVisible(paragraphsView->showParagraphTitle());
    paragraphStyle.setTitle(paragraphsView->customParagraphTitle());

    currentTemplate.get().setParagraphStyle(paragraphStyle);
}

void TemplateOptionsManager::Implementation::saveTemplate()
{
    savePageParameters();
    saveTitlePage();
    saveParagraphParameters(paragraphsView->currentParagraphType());

    switch (currentDocumentType) {
    default:
    case Domain::DocumentObjectType::SimpleText: {
        BusinessLayer::TemplatesFacade::saveSimpleTextTemplate(currentTemplate.simpleText);
        break;
    }

    case Domain::DocumentObjectType::Screenplay: {
        BusinessLayer::TemplatesFacade::saveScreenplayTemplate(currentTemplate.screenplay);
        break;
    }

    case Domain::DocumentObjectType::ComicBook: {
        BusinessLayer::TemplatesFacade::saveComicBookTemplate(currentTemplate.comicBook);
        break;
    }

    case Domain::DocumentObjectType::Audioplay: {
        BusinessLayer::TemplatesFacade::saveAudioplayTemplate(currentTemplate.audioplay);
        break;
    }

    case Domain::DocumentObjectType::Stageplay: {
        BusinessLayer::TemplatesFacade::saveStageplayTemplate(currentTemplate.stageplay);
        break;
    }

    case Domain::DocumentObjectType::Novel: {
        BusinessLayer::TemplatesFacade::saveNovelTemplate(currentTemplate.novel);
        break;
    }
    }


    currentTemplateChanged = false;
}


// ****


TemplateOptionsManager::TemplateOptionsManager(QObject* _parent, QWidget* _parentWidget,
                                               const PluginsBuilder& _pluginsBuilder)
    : QObject(_parent)
    , d(new Implementation(_parentWidget, _pluginsBuilder))
{
    //
    // При закрытии редактора, спросим пользователя, нужно ли сохранить изменения
    //
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::backPressed, this, [this] {
        if (!d->currentTemplateChanged) {
            emit closeRequested();
            return;
        }

        const int kCancelButtonId = 0;
        const int kNoButtonId = 1;
        const int kYesButtonId = 2;
        auto dialog = new Dialog(d->toolBar->topLevelWidget());
        dialog->showDialog({}, tr("Template was modified. Save changes?"),
                           { { kCancelButtonId, tr("Cancel"), Dialog::RejectButton },
                             { kNoButtonId, tr("Don't save"), Dialog::NormalButton },
                             { kYesButtonId, tr("Save"), Dialog::AcceptButton } });
        QObject::connect(
            dialog, &Dialog::finished,
            [this, kCancelButtonId, kYesButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
                dialog->hideDialog();

                //
                // Пользователь передумал сохранять
                //
                if (_buttonInfo.id == kCancelButtonId) {
                    return;
                }

                //
                // Пользователь хочет сохранить изменения
                //
                if (_buttonInfo.id == kYesButtonId) {
                    d->saveTemplate();
                    emit templateSaved(d->currentDocumentType, d->currentTemplate.get().id());
                }

                emit closeRequested();
            });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
    });

    //
    // Переходы по страницам редактора шаблона
    //
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::pageSettingsPressed, this,
            [this] { emit showViewRequested(d->pageView); });
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::titlePageSettingsPressed, this, [this] {
        //
        // Получим редактор титульной страницы
        //
        auto titlePageView = d->pluginsBuilder.activateView(
            "application/x-starc/editor/screenplay/title-page", &d->titlePageModel);
        titlePageView->setEditingMode(DocumentEditingMode::Edit);
        d->titlePageView = titlePageView->asQWidget();

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
                titlePageToolbar->move((d->titlePageView->width() - titlePageToolbar->width())
                                           / 2.0,
                                       -Ui::DesignSystem::card().shadowMargins().top());
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

    //
    // Пользователь изменил единицы измерения - отразим изменение в интерфейсе
    //
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
        d->saveTemplate();
        emit templateSaved(d->currentDocumentType, d->currentTemplate.get().id());
    });
    connect(d->paragraphsView, &Ui::ScreenplayTemplateParagraphsView::currentParagraphTypeChanged,
            this,
            [this](BusinessLayer::TextParagraphType _currentType,
                   BusinessLayer::TextParagraphType _previousType) {
                d->saveParagraphParameters(_previousType);
                d->updateParagraphParameters(_currentType);
            });

    //
    // Пометим текущий шаблон изменённым, если произошли какие-либо изменения в его параметрах
    //
    auto markChanged = [this] { d->currentTemplateChanged = true; };
    connect(d->pageView, &Ui::ScreenplayTemplatePageView::pageChanged, this, markChanged);
    connect(&d->titlePageModel, &BusinessLayer::ScreenplayTitlePageModel::contentsChanged, this,
            markChanged);
    connect(d->paragraphsView, &Ui::ScreenplayTemplateParagraphsView::currentParagraphChanged, this,
            markChanged);
}

TemplateOptionsManager::~TemplateOptionsManager() = default;

QWidget* TemplateOptionsManager::toolBar() const
{
    return d->toolBar;
}

QWidget* TemplateOptionsManager::navigator() const
{
    return d->navigator;
}

QWidget* TemplateOptionsManager::view() const
{
    return d->pageView;
}

QWidget* TemplateOptionsManager::viewToolBar() const
{
    return d->viewToolBar;
}

Domain::DocumentObjectType TemplateOptionsManager::currentDocumentType() const
{
    return d->currentDocumentType;
}

void TemplateOptionsManager::setCurrentDocumentType(Domain::DocumentObjectType _type)
{
    if (d->currentDocumentType == _type) {
        return;
    }

    d->currentDocumentType = _type;
    d->toolBar->setTitlePageVisible(_type != Domain::DocumentObjectType::SimpleText);
    d->pageView->configureTemplateFor(d->currentDocumentType);
    d->paragraphsView->configureTemplateFor(d->currentDocumentType);
}

void TemplateOptionsManager::editTemplate(const QString& _templateId)
{
    const bool isNewTemplate = false;
    d->prepareViewToEdit(_templateId, isNewTemplate);

    //
    // Пометим что шаблон не был изменён отложенно, т.к. в редакторе текста стоит дебаунсинг на
    // изменение и событие очистки прийдёт только через некоторое время
    //
    QTimer::singleShot(300, this, [this] { d->currentTemplateChanged = false; });
}

void TemplateOptionsManager::duplicateTemplate(const QString& _templateId)
{
    const bool isNewTemplate = true;
    d->prepareViewToEdit(_templateId, isNewTemplate);

    d->currentTemplateChanged = true;
}

} // namespace ManagementLayer

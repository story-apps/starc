#include "templates_facade.h"

#include "screenplay_template.h"
#include "text_template.h"

#include <QApplication>
#include <QDir>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QString>


namespace BusinessLayer {

namespace {

/**
 * @brief Параметры группы шаблонов
 */
template<typename TemplateType>
class TemplateInfo
{
public:
    /**
     * @brief Шаблон по умолчанию
     */
    TemplateType defaultTemplate;

    /**
     * @brief Шаблоны сценариев <id, шаблон>
     */
    QHash<QString, TemplateType> templates;

    /**
     * @brief Модель шаблонов
     */
    QStandardItemModel model;
};

} // namespace

class TemplatesFacade::Implementation
{
public:
    template<typename TemplateType>
    TemplateInfo<TemplateType>& templateInfo();

    template<typename TemplateType>
    QStandardItemModel* templatesModel();

    template<typename TemplateType>
    const TemplateType& getTemplate(const QString& _templateId);

    template<typename TemplateType>
    void setDefaultTemplate(const QString& _templateId);

    template<typename TemplateType>
    void updateTranslations();

    template<typename TemplateType>
    void loadTemplates(const QString& _templatesDir, const QVector<QString> _templateNames);


    TemplateInfo<SimpleTextTemplate> text;
    TemplateInfo<ScreenplayTemplate> screenplay;
};

template<>
TemplateInfo<SimpleTextTemplate>& TemplatesFacade::Implementation::templateInfo<
    SimpleTextTemplate>()
{
    return text;
}
template<>
TemplateInfo<ScreenplayTemplate>& TemplatesFacade::Implementation::templateInfo<
    ScreenplayTemplate>()
{
    return screenplay;
}

template<typename TemplateType>
QStandardItemModel* TemplatesFacade::Implementation::templatesModel()
{
    return &templateInfo<TemplateType>().model;
}

template<typename TemplateType>
const TemplateType& TemplatesFacade::Implementation::getTemplate(const QString& _templateId)
{
    //
    // Если id шаблона задан и он есть в списке доступных шаблонов, возвращаем искомый
    //
    if (!_templateId.isEmpty() && templateInfo<TemplateType>().templates.contains(_templateId)) {
        return templateInfo<TemplateType>().templates[_templateId];
    }

    //
    // Во всех остальных случаях возвращаем дефолтный шаблон
    //
    return templateInfo<TemplateType>().defaultTemplate;
}

template<typename TemplateType>
void TemplatesFacade::Implementation::setDefaultTemplate(const QString& _templateId)
{
    if (_templateId.isEmpty() || !templateInfo<TemplateType>().templates.contains(_templateId)) {
        return;
    }

    templateInfo<TemplateType>().defaultTemplate
        = templateInfo<TemplateType>().templates[_templateId];
}

template<typename TemplateType>
void TemplatesFacade::Implementation::updateTranslations()
{
    auto& templatesModel = this->templateInfo<TemplateType>().model;
    for (int row = 0; row < templatesModel.rowCount(); ++row) {
        auto templateItem = templatesModel.item(row);
        const auto templateId = templateItem->data(kTemplateIdRole).toString();
        templateItem->setText(getTemplate<TemplateType>(templateId).name());
    }
}

template<typename TemplateType>
void TemplatesFacade::Implementation::loadTemplates(const QString& _templatesDir,
                                                    const QVector<QString> _templateNames)
{
    //
    // Настроим путь к папке с шаблонами
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = QString("%1/%2").arg(appDataFolderPath, _templatesDir);
    //
    // ... создаём папку для пользовательских файлов
    //
    const QDir rootFolder = QDir::root();
    rootFolder.mkpath(templatesFolderPath);

    //
    // Обновить шаблон по умолчанию
    //
    auto updateDefaultTemplate
        = [_templatesDir, templatesFolderPath](const QString& _templateName) -> QString {
        const QString defaultTemplatePath
            = QString("%1/%2").arg(templatesFolderPath, _templateName);
        QFile defaultTemplateFile(defaultTemplatePath);
        if (!defaultTemplateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return {};
        }

        QFile defaultTemplateRcFile(QString(":/%1/%2").arg(_templatesDir, _templateName));
        if (!defaultTemplateRcFile.open(QIODevice::ReadOnly)) {
            return {};
        }

        defaultTemplateFile.write(defaultTemplateRcFile.readAll());
        defaultTemplateRcFile.close();
        defaultTemplateFile.close();
        return defaultTemplatePath;
    };

    QString defaultTemplatePath;
    for (const auto& templateName : _templateNames) {
        const auto templatePath = updateDefaultTemplate(templateName);
        if (defaultTemplatePath.isEmpty()) {
            defaultTemplatePath = templatePath;
        }
    }

    //
    // Загрузить шаблоны
    //
    // ... шаблон по умолчанию
    //
    auto& templateInfo = this->templateInfo<TemplateType>();
    templateInfo.defaultTemplate = TemplateType(defaultTemplatePath);
    templateInfo.templates.insert(templateInfo.defaultTemplate.id(), templateInfo.defaultTemplate);
    //
    const auto templatesFiles = QDir(templatesFolderPath).entryInfoList(QDir::Files);
    for (const QFileInfo& templateFile : templatesFiles) {
        TemplateType concreteTemplate(templateFile.absoluteFilePath());
        if (!templateInfo.templates.contains(concreteTemplate.id())) {
            templateInfo.templates.insert(concreteTemplate.id(), concreteTemplate);
        }
    }

    //
    // Настроим модель шаблонов
    //
    auto sortedTemplates = templateInfo.templates.values();
    std::sort(sortedTemplates.begin(), sortedTemplates.end(),
              [](const TemplateType& _lhs, const TemplateType& _rhs) {
                  return _lhs.name() < _rhs.name();
              });
    for (const auto& screenplayTemplate : std::as_const(sortedTemplates)) {
        auto item = new QStandardItem(screenplayTemplate.name());
        item->setData(screenplayTemplate.id(), kTemplateIdRole);
        templateInfo.model.appendRow(item);
    }
}


// ****


QStandardItemModel* TemplatesFacade::simpleTextTemplates()
{
    return instance().d->templatesModel<SimpleTextTemplate>();
}

QStandardItemModel* TemplatesFacade::screenplayTemplates()
{
    return instance().d->templatesModel<ScreenplayTemplate>();
}

const SimpleTextTemplate& TemplatesFacade::simpleTextTemplate(const QString& _templateId)
{
    return instance().d->getTemplate<SimpleTextTemplate>(_templateId);
}

const ScreenplayTemplate& TemplatesFacade::screenplayTemplate(const QString& _templateId)
{
    return instance().d->getTemplate<ScreenplayTemplate>(_templateId);
}

void TemplatesFacade::setDefaultSimpleTextTemplate(const QString& _templateId)
{
    instance().d->setDefaultTemplate<SimpleTextTemplate>(_templateId);
}

void TemplatesFacade::setDefaultScreenplayTemplate(const QString& _templateId)
{
    instance().d->setDefaultTemplate<ScreenplayTemplate>(_templateId);
}

void TemplatesFacade::updateTranslations()
{
    instance().d->updateTranslations<SimpleTextTemplate>();
    instance().d->updateTranslations<ScreenplayTemplate>();
}

TemplatesFacade::~TemplatesFacade() = default;

TemplatesFacade::TemplatesFacade()
    : d(new Implementation)
{
    d->loadTemplates<SimpleTextTemplate>(QLatin1String("templates/text"),
                                         { QLatin1String("mono_cp_a4"), QLatin1String("mono_cn_a4"),
                                           QLatin1String("mono_cp_letter") });
    d->loadTemplates<ScreenplayTemplate>(
        QLatin1String("templates/screenplay"),
        { QLatin1String("world_cp"), QLatin1String("world_cn"), QLatin1String("ar"),
          QLatin1String("he"), QLatin1String("ru"), QLatin1String("tamil"), QLatin1String("us") });
}

TemplatesFacade& TemplatesFacade::instance()
{
    static TemplatesFacade* facade = nullptr;
    if (facade == nullptr) {
        facade = new TemplatesFacade;
    }
    return *facade;
}

} // namespace BusinessLayer

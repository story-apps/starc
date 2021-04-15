#include "templates_facade.h"

#include "text_template.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QApplication>
#include <QDir>
#include <QString>
#include <QStandardItemModel>
#include <QStandardPaths>


namespace BusinessLayer
{

namespace {

/**
 * @brief Параметры группы шаблонов
 */
template<typename TemplateType>
class TemplateInfo {
public:
    /**
     * @brief Шаблон по умолчанию
     */
    TemplateType defaultTemplate;

    /**
     * @brief Шаблоны сценариев <id, шаблон>
     */
    QMap<QString, TemplateType> templates;

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


    TemplateInfo<TextTemplate> text;
};

template<>
TemplateInfo<TextTemplate>& TemplatesFacade::Implementation::templateInfo<TextTemplate>()
{
    return text;
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
    if (!_templateId.isEmpty()
        && templateInfo<TemplateType>().templates.contains(_templateId)) {
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
    if (_templateId.isEmpty()
        || !templateInfo<TemplateType>().templates.contains(_templateId)) {
        return;
    }

    templateInfo<TemplateType>().defaultTemplate = templateInfo<TemplateType>().templates[_templateId];
}

template<typename TemplateType>
void TemplatesFacade::Implementation::updateTranslations()
{
    auto& templatesModel = templateInfo<TemplateType>().model;
    for (int row = 0; row < templatesModel.rowCount(); ++row) {
        auto templateItem = templatesModel.item(row);
        const auto templateId = templateItem->data(kTemplateIdRole).toString();
        templateItem->setText(textTemplate(templateId).name());
    }
}


// ****


QStandardItemModel* TemplatesFacade::textTemplates()
{
    return instance().d->templatesModel<TextTemplate>();
}

const TextTemplate& TemplatesFacade::textTemplate(const QString& _templateId)
{
    return instance().d->getTemplate<TextTemplate>(_templateId);
}

void TemplatesFacade::setDefaultTextTemplate(const QString& _templateId)
{
    instance().d->setDefaultTemplate<TextTemplate>(_templateId);
}

void TemplatesFacade::updateTranslations()
{
    instance().d->updateTranslations<TextTemplate>();
}

TemplatesFacade::~TemplatesFacade() = default;

TemplatesFacade::TemplatesFacade()
    : d(new Implementation)
{
    //
    // Настроим путь к папке с шаблонами
    //
    const QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = QString("%1/%2").arg(appDataFolderPath, "templates/text");
    //
    // ... создаём папку для пользовательских файлов
    //
    const QDir rootFolder = QDir::root();
    rootFolder.mkpath(templatesFolderPath);

    //
    // Обновить шаблон по умолчанию
    //
    auto updateDefaultTemplate = [templatesFolderPath] (const QString& _templateName) -> QString {
        const QString defaultTemplatePath = QString("%1/%2").arg(templatesFolderPath, _templateName);
        QFile defaultTemplateFile(defaultTemplatePath);
        if (!defaultTemplateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return {};
        }

        QFile defaultTemplateRcFile(QString(":/templates/text/%1").arg(_templateName));
        if (!defaultTemplateRcFile.open(QIODevice::ReadOnly)) {
            return {};
        }

        defaultTemplateFile.write(defaultTemplateRcFile.readAll());
        defaultTemplateRcFile.close();
        defaultTemplateFile.close();
        return defaultTemplatePath;
    };

    const QVector<QString> templateNames = { "mono_cp_a4", "mono_cn_a4", "mono_cp_letter" };
    QString defaultTemplatePath;
    for (const auto& templateName : templateNames) {
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
    d->text.defaultTemplate = TextTemplate(defaultTemplatePath);
    d->text.templates.insert(d->text.defaultTemplate.id(), d->text.defaultTemplate);
    //
    const auto templatesFiles = QDir(templatesFolderPath).entryInfoList(QDir::Files);
    for (const QFileInfo& templateFile : templatesFiles) {
        TextTemplate textTemplate(templateFile.absoluteFilePath());
        if (!d->text.templates.contains(textTemplate.id())) {
            d->text.templates.insert(textTemplate.id(), textTemplate);
        }
    }

    //
    // Настроим модель шаблонов
    //
    auto sortedTemplates = d->text.templates.values();
    std::sort(sortedTemplates.begin(), sortedTemplates.end(),
              [] (const TextTemplate& _lhs, const TextTemplate& _rhs) {
        return _lhs.name() < _rhs.name();
    });
    for (const auto& screenplayTemplate : std::as_const(sortedTemplates)) {
        auto item = new QStandardItem(screenplayTemplate.name());
        item->setData(screenplayTemplate.id(), kTemplateIdRole);
        d->text.model.appendRow(item);
    }
}

TemplatesFacade& TemplatesFacade::instance()
{
    static TemplatesFacade* facade = nullptr;
    if (facade == nullptr) {
        facade = new TemplatesFacade;
    }
    return *facade;
}

} // namespace BusinssLayer

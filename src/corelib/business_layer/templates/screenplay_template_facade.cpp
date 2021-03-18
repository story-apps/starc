#include "screenplay_template_facade.h"

#include "screenplay_template.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QApplication>
#include <QDir>
#include <QString>
#include <QStandardItemModel>
#include <QStandardPaths>


namespace BusinessLayer
{

class ScreenplayTemplateFacade::Implementation
{
public:
    /**
     * @brief Шаблон по умолчанию
     */
    ScreenplayTemplate defaultTemplate;

    /**
     * @brief Шаблоны сценариев <id, шаблон>
     */
    QMap<QString, ScreenplayTemplate> templates;

    /**
     * @brief Модель шаблонов
     */
    QStandardItemModel templatesModel;
};


// ****


QStandardItemModel* ScreenplayTemplateFacade::templates()
{
    return &instance().d->templatesModel;
}

const ScreenplayTemplate& ScreenplayTemplateFacade::getTemplate(const QString& _templateId)
{
    //
    // Если id шаблона задан и он есть в списке доступных шаблонов, возвращаем искомый
    //
    if (!_templateId.isEmpty()
        && instance().d->templates.contains(_templateId)) {
        return instance().d->templates[_templateId];
    }

    //
    // Во всех остальных случаях возвращаем дефолтный шаблон
    //
    return instance().d->defaultTemplate;
}

void ScreenplayTemplateFacade::setDefaultTemplate(const QString& _templateId)
{
    if (_templateId.isEmpty()
        || !instance().d->templates.contains(_templateId)) {
        return;
    }

    instance().d->defaultTemplate = instance().d->templates[_templateId];
}

ScreenplayTemplateFacade::~ScreenplayTemplateFacade() = default;

ScreenplayTemplateFacade::ScreenplayTemplateFacade()
    : d(new Implementation)
{
    //
    // Настроим путь к папке с шаблонами
    //
    const QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = QString("%1/%2").arg(appDataFolderPath, "templates/screenplay");
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

        QFile defaultTemplateRcFile(QString(":/templates/screenplay/%1").arg(_templateName));
        if (!defaultTemplateRcFile.open(QIODevice::ReadOnly)) {
            return {};
        }

        defaultTemplateFile.write(defaultTemplateRcFile.readAll());
        defaultTemplateRcFile.close();
        defaultTemplateFile.close();
        return defaultTemplatePath;
    };

    const QVector<QString> templateNames = { "world_cp", "world_cn", "ar", "he", "ru", "us" };
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
    // ... удаляем легаси шаблоны
    //     FIXME: Убрать этот код в версии 0.1.0
    //
    QDir(templatesFolderPath).remove("fd_letter");
    QDir(templatesFolderPath).remove("fd_a4");
    //
    // ... шаблон по умолчанию
    //
    d->defaultTemplate = ScreenplayTemplate(defaultTemplatePath);
    d->templates.insert(d->defaultTemplate.id(), d->defaultTemplate);
    //
    const auto templatesFiles = QDir(templatesFolderPath).entryInfoList(QDir::Files);
    for (const QFileInfo& templateFile : templatesFiles) {
        ScreenplayTemplate screenplayTemplate(templateFile.absoluteFilePath());
        if (!d->templates.contains(screenplayTemplate.id())) {
            d->templates.insert(screenplayTemplate.id(), screenplayTemplate);
        }
    }

    //
    // Настроим модель шаблонов
    //
    auto sortedTemplates = d->templates.values();
    std::sort(sortedTemplates.begin(), sortedTemplates.end(),
              [] (const ScreenplayTemplate& _lhs, const ScreenplayTemplate& _rhs) {
        return _lhs.name() < _rhs.name();
    });
    for (const auto& screenplayTemplate : std::as_const(sortedTemplates)) {
        auto item = new QStandardItem(screenplayTemplate.name());
        item->setData(screenplayTemplate.id(), kTemplateIdRole);
        d->templatesModel.appendRow(item);
    }
}

ScreenplayTemplateFacade& ScreenplayTemplateFacade::instance()
{
    static ScreenplayTemplateFacade facade;
    return facade;
}

} // namespace BusinssLayer

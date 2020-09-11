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
    ScreenplayTemplate m_defaultTemplate;

    /**
     * @brief Шаблоны сценариев
     */
    QMap<QString, ScreenplayTemplate> m_templates;

    /**
     * @brief Модель шаблонов
     */
    QStandardItemModel m_templatesModel;
};


// ****


QStandardItemModel* ScreenplayTemplateFacade::templatesList()
{
    return &instance().d->m_templatesModel;
}

const ScreenplayTemplate& ScreenplayTemplateFacade::getTemplate(const QString& _templateName)
{
    //
    // Если название шаблона задано
    //
    if (!_templateName.isEmpty()) {
        //
        // ... и он есть в списке доступных шаблонов - возвращаем искомый
        //
        if (instance().d->m_templates.contains(_templateName)) {
            return instance().d->m_templates[_templateName];
        }
        //
        // ... а если в списке такого нет, возвращаем дефолтный
        //
        else {
            return instance().d->m_defaultTemplate;
        }
    }

    //
    // Если название шаблона не задано
    //
    const QString currentTemplateName =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "screenplay-editor/current-style",
                DataStorageLayer::SettingsStorage::SettingsPlace::Application).toString();
    //
    // ... получим название текущего используемого шаблона и если такой шаблон есть
    //     в списке доступных, вернём его
    //
    if (!currentTemplateName.isEmpty()
        && instance().d->m_templates.contains(currentTemplateName)) {
        return instance().d->m_templates[currentTemplateName];
    }

    //
    // Во всех остальных случаях возвращаем дефолтный шаблон
    //
    return instance().d->m_defaultTemplate;
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

    const QVector<QString> templateNames = { "fd_letter", "fd_a4", "ru" };
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
    const QDir templatesDir(templatesFolderPath);
    for (const QFileInfo& templateFile : templatesDir.entryInfoList(QDir::Files)) {
        ScreenplayTemplate templateObj(templateFile.absoluteFilePath());
        if (!d->m_templates.contains(templateObj.name())) {
            d->m_templates.insert(templateObj.name(), templateObj);
        }
    }
    //
    // ... шаблон по умолчанию
    //
    d->m_defaultTemplate = ScreenplayTemplate(defaultTemplatePath);

    //
    // Настроим модель шаблонов
    //
    QStandardItem* rootItem = d->m_templatesModel.invisibleRootItem();
    for (const auto& templateObj : d->m_templates) {
        if (templateObj.name() == "mobile") {
            continue;
        }

        QList<QStandardItem*> row;
        row << new QStandardItem(templateObj.name());
        row << new QStandardItem(templateObj.description());
        rootItem->appendRow(row);
    }
}

ScreenplayTemplateFacade& ScreenplayTemplateFacade::instance()
{
    static ScreenplayTemplateFacade facade;
    return facade;
}

} // namespace BusinssLayer

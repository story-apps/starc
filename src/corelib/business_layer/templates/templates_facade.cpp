#include "templates_facade.h"

#include "comic_book_template.h"
#include "screenplay_template.h"
#include "simple_text_template.h"

#include <QApplication>
#include <QDir>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QString>


namespace BusinessLayer {

namespace {

const QLatin1String kTextTemplatesDirectory("templates/text");
const QLatin1String kScreenplayTemplatesDirectory("templates/screenplay");
const QLatin1String kComicBookTemplatesDirectory("templates/comicbook");

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

    template<typename TemplateType>
    void saveTemplate(const QString& _templatesDir, const TemplateType& _template);

    template<typename TemplateType>
    void removeTemplate(const QString& _templatesDir, const QString& _templateId);


    TemplateInfo<SimpleTextTemplate> text;
    TemplateInfo<ScreenplayTemplate> screenplay;
    TemplateInfo<ComicBookTemplate> comicBook;
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
template<>
TemplateInfo<ComicBookTemplate>& TemplatesFacade::Implementation::templateInfo<ComicBookTemplate>()
{
    return comicBook;
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
        auto templateModelItem = templatesModel.item(row);
        const auto templateId = templateModelItem->data(kTemplateIdRole).toString();
        const auto& templateItem = getTemplate<TemplateType>(templateId);
        if (templateItem.isDefault()) {
            templateModelItem->setText(templateItem.name());
        }
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
    for (const auto& templateItem : std::as_const(sortedTemplates)) {
        auto item = new QStandardItem(templateItem.name());
        item->setData(templateItem.id(), kTemplateIdRole);
        templateInfo.model.appendRow(item);
    }
}

template<typename TemplateType>
void TemplatesFacade::Implementation::saveTemplate(const QString& _templatesDir,
                                                   const TemplateType& _template)
{
    //
    // Сохраним шаблон в файл
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = QString("%1/%2").arg(appDataFolderPath, _templatesDir);
    _template.saveToFile(QString("%1/%2").arg(templatesFolderPath, _template.id()));

    auto& templateInfo = this->templateInfo<TemplateType>();

    //
    // Если это обновление дефолтного шаблона, обновим и его тоже
    //
    if (templateInfo.defaultTemplate.id() == _template.id()) {
        templateInfo.defaultTemplate = _template;
    }

    //
    // Добавим шаблон в список, если ещё не был добавлен
    //
    const auto hasTemplate = templateInfo.templates.contains(_template.id());
    templateInfo.templates[_template.id()] = _template;

    //
    // Удаляем старую версию шаблона из модели, т.к. могло измениться название
    //
    bool isNameChanged = false;
    if (hasTemplate) {
        for (int row = 0; row < templateInfo.model.rowCount(); ++row) {
            if (templateInfo.model.item(row)->data(kTemplateIdRole).toString() != _template.id()) {
                continue;
            }

            if (templateInfo.model.item(row)->text() != _template.name()) {
                isNameChanged = true;
                templateInfo.model.removeRow(row);
            }
            break;
        }
    }

    //
    // Добавляем шаблон в модель
    //
    if (!hasTemplate || isNameChanged) {
        auto item = new QStandardItem(_template.name());
        item->setData(_template.id(), kTemplateIdRole);
        bool itemAdded = false;
        for (int row = 0; row < templateInfo.model.rowCount(); ++row) {
            if (templateInfo.model.item(row)->text() > _template.name()) {
                itemAdded = true;
                templateInfo.model.insertRow(row, item);
                break;
            }
        }
        if (!itemAdded) {
            templateInfo.model.appendRow(item);
        }
    }
}

template<typename TemplateType>
void TemplatesFacade::Implementation::removeTemplate(const QString& _templatesDir,
                                                     const QString& _templateId)
{
    //
    // Удаляем файл шаблона
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString templatesFolderPath = QString("%1/%2").arg(appDataFolderPath, _templatesDir);
    QFile::remove(QString("%1/%2").arg(templatesFolderPath, _templateId));

    //
    // Удаляем шаблон из списке
    //
    auto& templateInfo = this->templateInfo<TemplateType>();
    templateInfo.templates.remove(_templateId);

    //
    // Удаляем шаблон из модели
    //
    for (int row = 0; row < templateInfo.model.rowCount(); ++row) {
        if (templateInfo.model.item(row)->data(kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        templateInfo.model.removeRow(row);
        break;
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

QStandardItemModel* TemplatesFacade::comicBookTemplates()
{
    return instance().d->templatesModel<ComicBookTemplate>();
}

const SimpleTextTemplate& TemplatesFacade::simpleTextTemplate(const QString& _templateId)
{
    return instance().d->getTemplate<SimpleTextTemplate>(_templateId);
}

const ScreenplayTemplate& TemplatesFacade::screenplayTemplate(const QString& _templateId)
{
    return instance().d->getTemplate<ScreenplayTemplate>(_templateId);
}

const SimpleTextTemplate& TemplatesFacade::screenplayTitlePageTemplate(const QString& _templateId)
{
    return screenplayTemplate(_templateId).titlePageTemplate();
}

const ComicBookTemplate& TemplatesFacade::comicBookTemplate(const QString& _templateId)
{
    return instance().d->getTemplate<ComicBookTemplate>(_templateId);
}

void TemplatesFacade::setDefaultSimpleTextTemplate(const QString& _templateId)
{
    instance().d->setDefaultTemplate<SimpleTextTemplate>(_templateId);
}

void TemplatesFacade::setDefaultScreenplayTemplate(const QString& _templateId)
{
    instance().d->setDefaultTemplate<ScreenplayTemplate>(_templateId);
}

void TemplatesFacade::setDefaultComicBookTemplate(const QString& _templateId)
{
    instance().d->setDefaultTemplate<ComicBookTemplate>(_templateId);
}

void TemplatesFacade::saveScreenplayTemplate(const ScreenplayTemplate& _template)
{
    instance().d->saveTemplate<ScreenplayTemplate>(kScreenplayTemplatesDirectory, _template);
}

void TemplatesFacade::removeScreenplayTemplate(const QString& _templateId)
{
    instance().d->removeTemplate<ScreenplayTemplate>(kScreenplayTemplatesDirectory, _templateId);
}

void TemplatesFacade::updateTranslations()
{
    instance().d->updateTranslations<SimpleTextTemplate>();
    instance().d->updateTranslations<ScreenplayTemplate>();
    instance().d->updateTranslations<ComicBookTemplate>();
}

TemplatesFacade::~TemplatesFacade() = default;

TemplatesFacade::TemplatesFacade()
    : d(new Implementation)
{
    d->loadTemplates<SimpleTextTemplate>(
        kTextTemplatesDirectory,
        { QLatin1String("mono_cp_a4"), QLatin1String("mono_cn_a4"), QLatin1String("mono_cp_letter"),
          QLatin1String("sans_a4"), QLatin1String("sans_letter") });
    d->loadTemplates<ScreenplayTemplate>(
        kScreenplayTemplatesDirectory,
        { QLatin1String("world_cp"), QLatin1String("world_cn"), QLatin1String("ar"),
          QLatin1String("he"), QLatin1String("ru"), QLatin1String("tamil"), QLatin1String("us") });
    d->loadTemplates<ComicBookTemplate>(kComicBookTemplatesDirectory,
                                        { QLatin1String("world"), QLatin1String("us") });
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

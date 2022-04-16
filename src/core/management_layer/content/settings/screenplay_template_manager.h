#pragma once

#include <QObject>

namespace Domain {
enum class DocumentObjectType;
}


namespace ManagementLayer {

class PluginsBuilder;

/**
 * @brief Менеджер экрана параметров шаблона документа
 */
class TemplateOptionsManager : public QObject
{
    Q_OBJECT

public:
    explicit TemplateOptionsManager(QObject* _parent, QWidget* _parentWidget,
                                    const PluginsBuilder& _pluginsBuilder);
    ~TemplateOptionsManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;
    QWidget* viewToolBar() const;

    /**
     * @brief Задать текущий тим шаблона
     */
    void setCurrentDocumentType(Domain::DocumentObjectType _type);

    /**
     * @brief Редактировать заданный шаблон
     */
    void editTemplate(const QString& _templateId);

    /**
     * @brief Дублировать заданный шаблон
     */
    void duplicateTemplate(const QString& _templateId);

signals:
    /**
     * @brief Запрос на закрытие редактора шаблона
     */
    void closeRequested();

    /**
     * @brief Запрос на отображение заданного представления
     */
    void showViewRequested(QWidget* _view);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

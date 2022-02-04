#pragma once

#include <QObject>


namespace ManagementLayer {

class PluginsBuilder;


/**
 * @brief Менеджер экрана параметров шаблона сценария
 */
class ScreenplayTemplateManager : public QObject
{
    Q_OBJECT

public:
    explicit ScreenplayTemplateManager(QObject* _parent, QWidget* _parentWidget,
                                       const PluginsBuilder& _pluginsBuilder);
    ~ScreenplayTemplateManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;
    QWidget* viewToolBar() const;

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

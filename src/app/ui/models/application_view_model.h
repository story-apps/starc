#ifndef APPLICATION_VIEW_MODEL_H
#define APPLICATION_VIEW_MODEL_H

#include <QObject>


/**
 * @brief Модель основного представления приложения
 */
class ApplicationViewModel : public QObject
{
    Q_OBJECT

    //
    // Представления
    //
    Q_PROPERTY(QString initialToolBar READ initialToolBar CONSTANT)
    Q_PROPERTY(QString currentToolBar READ currentToolBar NOTIFY currentToolBarChanged)
    Q_PROPERTY(QString initialNavigator READ initialNavigator CONSTANT)
    Q_PROPERTY(QString currentNavigator READ currentNavigator NOTIFY currentNavigatorChanged)
    Q_PROPERTY(QString initialView READ initialView CONSTANT)
    Q_PROPERTY(QString currentView READ currentView NOTIFY currentViewChanged)

    //
    // Настройки приложения
    //
    Q_PROPERTY(int language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool useDarkTheme READ useDarkTheme WRITE setUseDarkTheme NOTIFY useDarkThemeChanged)

public:
    explicit ApplicationViewModel(const QString& _initialTabBar, const QString& _initialNavigator,
        const QString& _initialView, QObject* _parent = nullptr);
    ~ApplicationViewModel() override;

    /**
     * @brief Панель инструментов текущего редактора
     */
    QString initialToolBar() const;
    QString currentToolBar() const;
    void setCurrentToolBar(const QString& _toolBar);
    Q_SIGNAL void currentToolBarChanged(const QString& _toolBar);

    /**
     * @brief Навигатор текущего редактора
     */
    QString initialNavigator() const;
    QString currentNavigator() const;
    void setCurrentNavigator(const QString& _navigator);
    Q_SIGNAL void currentNavigatorChanged(const QString& _navigator);

    /**
     * @brief Собственно сам редактор
     */
    QString initialView() const;
    QString currentView() const;
    void setCurrentView(const QString& _view);
    Q_SIGNAL void currentViewChanged(const QString& _view);

    /**
     * @brief Язык приложения
     */
    int language() const;
    void setLanguage(int _language);
    Q_SIGNAL void languageChanged(int _language);

    /**
     * @brief Используется ли тёмная тема (true) или светлая (false)
     */
    bool useDarkTheme() const;
    void setUseDarkTheme(bool _use);
    Q_SIGNAL void useDarkThemeChanged(bool _use);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

#endif // APPLICATION_VIEW_MODEL_H

#pragma once

#include <QApplication>


/**
 * @brief Собственная реализация класса приложения для обработки кастомных событий
 */
class Application : public QApplication
{
    Q_OBJECT

public:
    explicit Application(int& _argc, char** _argv);
    ~Application() override;

    /**
     * @brief Задать управляющего приложением
     */
    void setApplicationManager(QObject* _manager);

    /**
     * @brief Запустить приложение
     */
    void startUp();

    /**
     * @brief Переопределяется для определения события простоя приложения (idle)
     */
    bool notify(QObject* _object, QEvent* _event) override;

protected:
    /**
     * @brief Переопределяется для обработки события открытия файла в Mac OS
     */
    bool event(QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <management_layer/application_manager.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    //
    // Загрузим шрифты в базу шрифтов программы, если их там ещё нет
    //
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/fonts/materialdesignicons.ttf");

    qmlRegisterSingletonType(QUrl("qrc:/DesignSystem.qml"), "app.starc", 1, 0, "DesignSystem");

    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;

    ApplicationManager* applicationManager = new ApplicationManager;
    applicationManager->setupContext(engine.rootContext());

    engine.load(QUrl(QStringLiteral("qrc:/views/ApplicationView.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

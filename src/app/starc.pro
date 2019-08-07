QT += quick sql widgets quickcontrols2
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
    data_layer/database.cpp \
    management_layer/application_manager.cpp \
    management_layer/content/onboarding/onboarding_manager.cpp \
    management_layer/content/projects/projects_manager.cpp \
    ui/models/application_view_model.cpp \
    management_layer/content/splash/splash_manager.cpp \
    ui/models/onboarding_view_model.cpp

RESOURCES += qml.qrc \
    resources.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    data_layer/database.h \
    management_layer/application_manager.h \
    management_layer/content/onboarding/onboarding_manager.h \
    management_layer/content/projects/projects_manager.h \
    ui/models/application_view_model.h \
    management_layer/content/splash/splash_manager.h \
    ui/models/onboarding_view_model.h

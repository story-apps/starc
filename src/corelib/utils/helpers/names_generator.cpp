#include "names_generator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/text_field/text_field.h>

#include <QActionGroup>
#include <QBuffer>
#include <QCoreApplication>
#include <QHash>
#include <QRandomGenerator>
#include <QSet>
#include <qtzip/QtZipReader>


class NamesGenerator::Implementation
{
public:
    struct NameInfo {
        bool isMale = true;
        QString name;
    };

    /**
     * @brief <Country, NameInfo>
     */
    QHash<QString, QVector<NameInfo>> database;

    /**
     * @brief Опции генератора
     */
    struct {
        QString type;
        int gender = 0;
    } options;

    /**
     * @brief Соединения генератора
     */
    QHash<TextField*, QVector<QMetaObject::Connection>> connections;
};


// ****


const QVector<QString> NamesGenerator::types()
{
    QVector<QString> types;
    const auto database = instance().d->database;
    for (auto iter = database.constBegin(); iter != database.constEnd(); ++iter) {
        types.append(iter.key());
    }
    std::sort(types.begin(), types.end());
    return types;
}

QString NamesGenerator::generate(const QString& _type, int _gender)
{
    const auto database = instance().d->database;
    if (database.isEmpty()) {
        return {};
    }

    auto type = _type;
    if (type.isEmpty() || !database.contains(type)) {
        const auto types = NamesGenerator::types();
        type = types.at(QRandomGenerator::global()->bounded(0, types.size()));
    }

    auto names = database.value(type);
    if (_gender == 1) {
        names.erase(
            std::remove_if(names.begin(), names.end(),
                           [](const Implementation::NameInfo& _info) { return !_info.isMale; }),
            names.end());
    } else if (_gender == 2) {
        names.erase(
            std::remove_if(names.begin(), names.end(),
                           [](const Implementation::NameInfo& _info) { return _info.isMale; }),
            names.end());
    }
    return names.value(QRandomGenerator::global()->bounded(0, names.size())).name;
}

void NamesGenerator::bind(TextField* _textField)
{
    _textField->setTrailingIcon(u8"\U000F076E");
    auto generateNameConnection
        = QObject::connect(_textField, &TextField::trailingIconPressed, _textField, [_textField] {
              _textField->setText(NamesGenerator::generate(instance().d->options.type,
                                                           instance().d->options.gender));
          });
    auto configureConnection = QObject::connect(
        _textField, &TextField::trailingIconContextMenuRequested, _textField, [_textField] {
            auto menu = new ContextMenu(_textField);
            QVector<QAction*> actions;

            //
            // Действия для выбора пола
            //
            {
                auto genderGroup = new QActionGroup(menu);
                const auto genderIdKey = "gender-id";
                auto buildGenderAction = [&actions, genderGroup, genderIdKey](const QString& _name,
                                                                              int _gender) {
                    auto action = new QAction;
                    action->setText(_name);
                    action->setProperty(genderIdKey, _gender);
                    action->setCheckable(true);
                    action->setChecked(instance().d->options.gender == _gender);
                    action->setIconText(action->isChecked() ? u8"\U000F012C" : u8"\U000F68c0");
                    genderGroup->addAction(action);
                    actions.append(action);
                    return action;
                };
                //
                auto maleAction = buildGenderAction(
                    QCoreApplication::translate("NamesGenerator", "Male names"), 1);
                //
                auto femaleAction = buildGenderAction(
                    QCoreApplication::translate("NamesGenerator", "Female names"), 2);
                //
                auto bothAction = buildGenderAction(
                    QCoreApplication::translate("NamesGenerator", "Both names"), 0);
                //
                for (auto genderAction : {
                         maleAction,
                         femaleAction,
                         bothAction,
                     }) {
                    QObject::connect(
                        genderAction, &QAction::toggled, genderAction, [genderAction, genderIdKey] {
                            genderAction->setIconText(genderAction->isChecked() ? u8"\U000F012C"
                                                                                : u8"\U000F68c0");
                            if (genderAction->isChecked()) {
                                instance().d->options.gender
                                    = genderAction->property(genderIdKey).toInt();
                            }
                        });
                }
            }

            //
            // Выбор типа имени
            //
            {
                auto typeAction = new QAction;
                auto updateTypeActionText = [typeAction] {
                    typeAction->setText(
                        instance().d->options.type.isEmpty()
                            ? QCoreApplication::translate("NamesGenerator", "Type of name")
                            : instance().d->options.type);
                };
                updateTypeActionText();
                typeAction->setSeparator(true);
                actions.append(typeAction);

                auto typesGroup = new QActionGroup(menu);
                for (const auto& type : NamesGenerator::types()) {
                    auto action = new QAction(typeAction);
                    action->setText(type);
                    action->setCheckable(true);
                    action->setChecked(instance().d->options.type == type);
                    action->setIconText(action->isChecked() ? u8"\U000F012C" : u8"\U000F68c0");
                    typesGroup->addAction(action);
                    QObject::connect(action, &QAction::toggled, action,
                                     [updateTypeActionText, action] {
                                         action->setIconText(action->isChecked() ? u8"\U000F012C"
                                                                                 : u8"\U000F68c0");
                                         instance().d->options.type
                                             = action->isChecked() ? action->text() : QString();
                                         updateTypeActionText();
                                     });
                }
            }

            menu->setBackgroundColor(Ui::DesignSystem::color().background());
            menu->setTextColor(Ui::DesignSystem::color().onBackground());
            menu->setActions(actions);
            QObject::connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

            //
            // Покажем меню
            //
            menu->showContextMenu(QCursor::pos());
        });

    //
    // Сохраняем соединения
    //
    instance().d->connections.insert(_textField, { generateNameConnection, configureConnection });
}

void NamesGenerator::unbind(TextField* _textField)
{
    _textField->setTrailingIcon({});
    for (auto& connection : instance().d->connections.value(_textField)) {
        QObject::disconnect(connection);
    }
    instance().d->connections.remove(_textField);
}

NamesGenerator::~NamesGenerator() = default;

NamesGenerator::NamesGenerator()
    : d(new Implementation)
{
    QtZipReader databaseFileReader(":/data/names_database");
    auto databaseFileData = databaseFileReader.fileData("names_database");
    if (databaseFileData.isEmpty()) {
        return;
    }

    QBuffer databaseFileStream(&databaseFileData);
    if (!databaseFileStream.open(QIODevice::ReadOnly)) {
        return;
    }

    QString typeOfNames;
    QVector<Implementation::NameInfo> names;
    while (!databaseFileStream.atEnd()) {
        const auto textLine = databaseFileStream.readLine().trimmed();
        if (textLine.isEmpty()) {
            continue;
        }

        switch (textLine.at(0)) {
        case 'N': {
            if (!names.isEmpty()) {
                d->database[typeOfNames] = names;
                typeOfNames.clear();
                names.clear();
            }

            typeOfNames = textLine.mid(1);
            break;
        }

        case 'M': {
            names.append({ true, textLine.mid(1) });
            break;
        }

        case 'F': {
            names.append({ false, textLine.mid(1) });
            break;
        }

        default: {
            break;
        }
        }
    }
}

const NamesGenerator& NamesGenerator::instance()
{
    static NamesGenerator database;
    return database;
}

#include "screenplay_text_edit_shortcuts_manager.h"

#include "screenplay_text_edit.h"

#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QShortcut>
#include <QSignalMapper>

using BusinessLayer::ScreenplayParagraphType;


namespace Ui {

class ScreenplayTextEditShortcutsManager::Implementation
{
public:
    explicit Implementation(ScreenplayTextEdit* _editor);

    /**
     * @brief Создать или обновить комбинацию для заданного типа
     */
    void createOrUpdateShortcut(ScreenplayParagraphType _forBlockType);

    //
    // Данные
    //

    /**
     * @brief Редактор сценария
     */
    ScreenplayTextEdit* screenplayEditor = nullptr;

    /**
     * @brief Виджет в контексте которого будут активироваться горячие клавиши
     */
    QWidget* shortcutsContext = nullptr;

    /**
     * @brief Тип блока - горячие клавиши
     */
    QHash<ScreenplayParagraphType, QShortcut*> paragraphTypeToShortcut;
};

ScreenplayTextEditShortcutsManager::Implementation::Implementation(ScreenplayTextEdit* _editor)
    : screenplayEditor(_editor)
{
}

void ScreenplayTextEditShortcutsManager::Implementation::createOrUpdateShortcut(
    ScreenplayParagraphType _forBlockType)
{
    if (shortcutsContext == nullptr) {
        return;
    }

    const auto blockType = static_cast<ScreenplayParagraphType>(_forBlockType);
    const QString typeShortName = BusinessLayer::toString(blockType);
    const QString keySequenceText
        = settingsValue(QString("screenplay-editor/shortcuts/%1").arg(typeShortName)).toString();
    const QKeySequence keySequence(keySequenceText);

    if (paragraphTypeToShortcut.contains(_forBlockType)) {
        paragraphTypeToShortcut.value(_forBlockType)->setKey(keySequence);
    } else {
        paragraphTypeToShortcut[_forBlockType]
            = new QShortcut(keySequence, shortcutsContext, 0, 0, Qt::WidgetWithChildrenShortcut);
    }
}


// ****


ScreenplayTextEditShortcutsManager::ScreenplayTextEditShortcutsManager(ScreenplayTextEdit* _parent)
    : QObject(_parent)
    , d(new Implementation(_parent))
{
    Q_ASSERT(_parent);
}

ScreenplayTextEditShortcutsManager::~ScreenplayTextEditShortcutsManager() = default;

void ScreenplayTextEditShortcutsManager::setShortcutsContext(QWidget* _context)
{
    if (d->shortcutsContext == _context) {
        return;
    }

    d->shortcutsContext = _context;
    qDeleteAll(d->paragraphTypeToShortcut);

    //
    // Создаём шорткаты
    //
    d->createOrUpdateShortcut(ScreenplayParagraphType::UnformattedText);
    d->createOrUpdateShortcut(ScreenplayParagraphType::SceneHeading);
    d->createOrUpdateShortcut(ScreenplayParagraphType::SceneCharacters);
    d->createOrUpdateShortcut(ScreenplayParagraphType::Action);
    d->createOrUpdateShortcut(ScreenplayParagraphType::Character);
    d->createOrUpdateShortcut(ScreenplayParagraphType::Parenthetical);
    d->createOrUpdateShortcut(ScreenplayParagraphType::Dialogue);
    d->createOrUpdateShortcut(ScreenplayParagraphType::Lyrics);
    d->createOrUpdateShortcut(ScreenplayParagraphType::Transition);
    d->createOrUpdateShortcut(ScreenplayParagraphType::Shot);
    d->createOrUpdateShortcut(ScreenplayParagraphType::InlineNote);
    d->createOrUpdateShortcut(ScreenplayParagraphType::FolderHeader);

    //
    // Настраиваем их
    //
    QSignalMapper* mapper = new QSignalMapper(this);
    for (auto shortcutIter = d->paragraphTypeToShortcut.begin();
         shortcutIter != d->paragraphTypeToShortcut.end(); ++shortcutIter) {
        connect(shortcutIter.value(), &QShortcut::activated, mapper,
                qOverload<>(&QSignalMapper::map));
        mapper->setMapping(shortcutIter.value(), static_cast<int>(shortcutIter.key()));
    }
    connect(mapper, &QSignalMapper::mappedInt, this, [this](int _value) {
        d->screenplayEditor->setCurrentParagraphType(static_cast<ScreenplayParagraphType>(_value));
    });
}

void ScreenplayTextEditShortcutsManager::reconfigure()
{
    //
    // Обновим сочетания клавиш для всех блоков
    //
    for (const auto type : d->paragraphTypeToShortcut.keys()) {
        d->createOrUpdateShortcut(type);
    }
}

QString ScreenplayTextEditShortcutsManager::shortcut(ScreenplayParagraphType _forBlockType) const
{
    if (!d->paragraphTypeToShortcut.contains(_forBlockType)) {
        return {};
    }

    return d->paragraphTypeToShortcut.value(_forBlockType)
        ->key()
        .toString(QKeySequence::NativeText);
}

} // namespace Ui

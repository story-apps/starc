#include "text_edit_shortcuts_manager.h"

#include "text_edit.h"

#include <business_layer/templates/text_template.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QShortcut>
#include <QSignalMapper>

using BusinessLayer::TextParagraphType;


namespace Ui
{

class TextEditShortcutsManager::Implementation
{
public:
    explicit Implementation(TextEdit* _editor);

    /**
     * @brief Создать или обновить комбинацию для заданного типа
     */
    void createOrUpdateShortcut(TextParagraphType _forBlockType);

    //
    // Данные
    //

    /**
     * @brief Редактор текста
     */
    TextEdit* textEditor = nullptr;

    /**
     * @brief Виджет в контексте которого будут активироваться горячие клавиши
     */
    QWidget* shortcutsContext = nullptr;

    /**
     * @brief Тип блока - горячие клавиши
     */
    QHash<TextParagraphType, QShortcut*> paragraphTypeToShortcut;
};

TextEditShortcutsManager::Implementation::Implementation(TextEdit* _editor)
    : textEditor(_editor)
{
}

void TextEditShortcutsManager::Implementation::createOrUpdateShortcut(TextParagraphType _forBlockType)
{
    if (shortcutsContext == nullptr) {
        return;
    }

    const auto blockType = static_cast<TextParagraphType>(_forBlockType);
    const QString typeShortName = BusinessLayer::toString(blockType);
    const QString keySequenceText =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                QString("simpletext-editor/shortcuts/%1").arg(typeShortName),
                DataStorageLayer::SettingsStorage::SettingsPlace::Application
                ).toString();
    const QKeySequence keySequence(keySequenceText);

    if (paragraphTypeToShortcut.contains(_forBlockType)) {
        paragraphTypeToShortcut.value(_forBlockType)->setKey(keySequence);
    } else {
        paragraphTypeToShortcut[_forBlockType]
            = new QShortcut(keySequence, shortcutsContext, 0, 0, Qt::WidgetWithChildrenShortcut);
    }
}


// ****


TextEditShortcutsManager::TextEditShortcutsManager(TextEdit* _parent)
    : QObject(_parent),
      d(new Implementation(_parent))
{
    Q_ASSERT(_parent);
}

TextEditShortcutsManager::~TextEditShortcutsManager() = default;

void TextEditShortcutsManager::setShortcutsContext(QWidget* _context)
{
    if (d->shortcutsContext == _context) {
        return;
    }

    d->shortcutsContext = _context;
    qDeleteAll(d->paragraphTypeToShortcut);

    //
    // Создаём шорткаты
    //
    d->createOrUpdateShortcut(TextParagraphType::Heading1);
    d->createOrUpdateShortcut(TextParagraphType::Heading2);
    d->createOrUpdateShortcut(TextParagraphType::Heading3);
    d->createOrUpdateShortcut(TextParagraphType::Heading4);
    d->createOrUpdateShortcut(TextParagraphType::Heading5);
    d->createOrUpdateShortcut(TextParagraphType::Heading6);
    d->createOrUpdateShortcut(TextParagraphType::Text);
    d->createOrUpdateShortcut(TextParagraphType::InlineNote);

    //
    // Настраиваем их
    //
    QSignalMapper* mapper = new QSignalMapper(this);
    for (auto shortcutIter = d->paragraphTypeToShortcut.begin();
         shortcutIter != d->paragraphTypeToShortcut.end();
         ++shortcutIter) {
        connect(shortcutIter.value(), &QShortcut::activated, mapper, qOverload<>(&QSignalMapper::map));
        mapper->setMapping(shortcutIter.value(), static_cast<int>(shortcutIter.key()));
    }
    connect(mapper, &QSignalMapper::mappedInt, this, [this] (int _value) {
        d->textEditor->setCurrentParagraphType(static_cast<TextParagraphType>(_value));
    });
}

void TextEditShortcutsManager::reconfigure()
{
    //
    // Обновим сочетания клавиш для всех блоков
    //
    for (const auto type : d->paragraphTypeToShortcut.keys()) {
        d->createOrUpdateShortcut(type);
    }
}

QString TextEditShortcutsManager::shortcut(TextParagraphType _forBlockType) const
{
    if (!d->paragraphTypeToShortcut.contains(_forBlockType)) {
        return {};
    }

    return d->paragraphTypeToShortcut.value(_forBlockType)->key().toString(QKeySequence::NativeText);
}

} // namespace Ui

#include "comic_book_text_edit_shortcuts_manager.h"

#include "comic_book_text_edit.h"

#include <business_layer/templates/comic_book_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QShortcut>
#include <QSignalMapper>

using BusinessLayer::TextParagraphType;


namespace Ui {

class ComicBookTextEditShortcutsManager::Implementation
{
public:
    explicit Implementation(ComicBookTextEdit* _editor);

    /**
     * @brief Создать или обновить комбинацию для заданного типа
     */
    void createOrUpdateShortcut(TextParagraphType _forBlockType);

    //
    // Данные
    //

    /**
     * @brief Редактор сценария
     */
    ComicBookTextEdit* comicBookEditor = nullptr;

    /**
     * @brief Виджет в контексте которого будут активироваться горячие клавиши
     */
    QWidget* shortcutsContext = nullptr;

    /**
     * @brief Тип блока - горячие клавиши
     */
    QHash<TextParagraphType, QShortcut*> paragraphTypeToShortcut;
};

ComicBookTextEditShortcutsManager::Implementation::Implementation(ComicBookTextEdit* _editor)
    : comicBookEditor(_editor)
{
}

void ComicBookTextEditShortcutsManager::Implementation::createOrUpdateShortcut(
    TextParagraphType _forBlockType)
{
    if (shortcutsContext == nullptr) {
        return;
    }

    const auto blockType = static_cast<TextParagraphType>(_forBlockType);
    const QString typeShortName = BusinessLayer::toString(blockType);
    const QString keySequenceText
        = settingsValue(QString("comicbook-editor/shortcuts/%1").arg(typeShortName)).toString();
    const QKeySequence keySequence(keySequenceText);

    if (paragraphTypeToShortcut.contains(_forBlockType)) {
        paragraphTypeToShortcut.value(_forBlockType)->setKey(keySequence);
    } else {
        paragraphTypeToShortcut[_forBlockType]
            = new QShortcut(keySequence, shortcutsContext, 0, 0, Qt::WidgetWithChildrenShortcut);
    }
}


// ****


ComicBookTextEditShortcutsManager::ComicBookTextEditShortcutsManager(ComicBookTextEdit* _parent)
    : QObject(_parent)
    , d(new Implementation(_parent))
{
    Q_ASSERT(_parent);
}

ComicBookTextEditShortcutsManager::~ComicBookTextEditShortcutsManager() = default;

void ComicBookTextEditShortcutsManager::setShortcutsContext(QWidget* _context)
{
    if (d->shortcutsContext == _context) {
        return;
    }

    d->shortcutsContext = _context;
    qDeleteAll(d->paragraphTypeToShortcut);

    //
    // Создаём шорткаты
    //
    d->createOrUpdateShortcut(TextParagraphType::UnformattedText);
    d->createOrUpdateShortcut(TextParagraphType::Page);
    d->createOrUpdateShortcut(TextParagraphType::Panel);
    d->createOrUpdateShortcut(TextParagraphType::Description);
    d->createOrUpdateShortcut(TextParagraphType::Character);
    d->createOrUpdateShortcut(TextParagraphType::Dialogue);
    d->createOrUpdateShortcut(TextParagraphType::InlineNote);
    d->createOrUpdateShortcut(TextParagraphType::SequenceHeader);

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
        d->comicBookEditor->setCurrentParagraphType(static_cast<TextParagraphType>(_value));
    });
}

void ComicBookTextEditShortcutsManager::reconfigure()
{
    //
    // Обновим сочетания клавиш для всех блоков
    //
    for (const auto type : d->paragraphTypeToShortcut.keys()) {
        d->createOrUpdateShortcut(type);
    }
}

QString ComicBookTextEditShortcutsManager::shortcut(TextParagraphType _forBlockType) const
{
    if (!d->paragraphTypeToShortcut.contains(_forBlockType)) {
        return {};
    }

    return d->paragraphTypeToShortcut.value(_forBlockType)
        ->key()
        .toString(QKeySequence::NativeText);
}

} // namespace Ui

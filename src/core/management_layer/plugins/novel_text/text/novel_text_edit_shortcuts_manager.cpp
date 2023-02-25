#include "novel_text_edit_shortcuts_manager.h"

#include "novel_text_edit.h"

#include <business_layer/templates/novel_template.h>
#include <utils/helpers/shortcuts_helper.h>

#include <QShortcut>
#include <QSignalMapper>

using BusinessLayer::TextParagraphType;


namespace Ui {

class NovelTextEditShortcutsManager::Implementation
{
public:
    explicit Implementation(NovelTextEdit* _editor);

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
    NovelTextEdit* novelEditor = nullptr;

    /**
     * @brief Виджет в контексте которого будут активироваться горячие клавиши
     */
    QWidget* shortcutsContext = nullptr;

    /**
     * @brief Тип блока - горячие клавиши
     */
    QHash<TextParagraphType, QShortcut*> paragraphTypeToShortcut;
};

NovelTextEditShortcutsManager::Implementation::Implementation(NovelTextEdit* _editor)
    : novelEditor(_editor)
{
}

void NovelTextEditShortcutsManager::Implementation::createOrUpdateShortcut(
    TextParagraphType _forBlockType)
{
    if (shortcutsContext == nullptr) {
        return;
    }

    const QKeySequence keyChapter(ShortcutsHelper::novelShortcut(_forBlockType));

    if (paragraphTypeToShortcut.contains(_forBlockType)) {
        paragraphTypeToShortcut.value(_forBlockType)->setKey(keyChapter);
    } else {
        paragraphTypeToShortcut[_forBlockType]
            = new QShortcut(keyChapter, shortcutsContext, 0, 0, Qt::WidgetWithChildrenShortcut);
    }
}


// ****


NovelTextEditShortcutsManager::NovelTextEditShortcutsManager(NovelTextEdit* _parent)
    : QObject(_parent)
    , d(new Implementation(_parent))
{
    Q_ASSERT(_parent);
}

NovelTextEditShortcutsManager::~NovelTextEditShortcutsManager() = default;

void NovelTextEditShortcutsManager::setShortcutsContext(QWidget* _context)
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
    d->createOrUpdateShortcut(TextParagraphType::SceneHeading);
    d->createOrUpdateShortcut(TextParagraphType::BeatHeading);
    d->createOrUpdateShortcut(TextParagraphType::Text);
    d->createOrUpdateShortcut(TextParagraphType::InlineNote);
    d->createOrUpdateShortcut(TextParagraphType::ChapterHeading);
    d->createOrUpdateShortcut(TextParagraphType::PartHeading);

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
        d->novelEditor->setCurrentParagraphType(static_cast<TextParagraphType>(_value));
    });
}

void NovelTextEditShortcutsManager::reconfigure()
{
    //
    // Обновим сочетания клавиш для всех блоков
    //
    for (auto iter = d->paragraphTypeToShortcut.begin(); iter != d->paragraphTypeToShortcut.end();
         ++iter) {
        d->createOrUpdateShortcut(iter.key());
    }
}

void NovelTextEditShortcutsManager::setEnabled(bool _enabled)
{
    for (auto shortcut : d->paragraphTypeToShortcut) {
        shortcut->setEnabled(_enabled);
    }
}

QString NovelTextEditShortcutsManager::shortcut(TextParagraphType _forBlockType) const
{
    if (!d->paragraphTypeToShortcut.contains(_forBlockType)) {
        return {};
    }

    return d->paragraphTypeToShortcut.value(_forBlockType)
        ->key()
        .toString(QKeySequence::NativeText);
}

} // namespace Ui

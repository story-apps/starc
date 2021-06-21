#include "ui_helper.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/widgets/text_edit/spell_check/spell_check_text_edit.h>


void UiHelper::initSpellingFor(SpellCheckTextEdit* _edit)
{
    initSpellingFor(QVector<SpellCheckTextEdit*>{ _edit });
}

void UiHelper::initSpellingFor(const QVector<SpellCheckTextEdit*>& _edits)
{
    auto settingsValue = [](const QString& _key) {
        return DataStorageLayer::StorageFacade::settingsStorage()->value(
            _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
    };
    const auto useSpellChecker
        = settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool();
    const auto spellingLanguage
        = settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString();

    for (auto edit : _edits) {
        edit->setUseSpellChecker(useSpellChecker);
        if (useSpellChecker) {
            edit->setSpellCheckLanguage(spellingLanguage);
        }
    }
}

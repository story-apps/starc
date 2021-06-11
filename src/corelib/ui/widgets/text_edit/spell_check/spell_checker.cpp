#include "spell_checker.h"

#include <hunspell/hunspell.hxx>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>
#include <QTextCodec>


namespace {
/**
 * @brief Тип словаря
 */
enum class SpellCheckerFileType { Affinity, Indexes, Dictionary };
} // namespace


class SpellChecker::Implementation
{
public:
    Implementation();

    /**
     * @brief Получить путь к файлу со словарём
     * @param Язык словаря
     * @param Тип словаря
     * @return Путь к файлу словаря
     */
    QString hunspellFilePath(SpellCheckerFileType _fileType) const;

    /**
     * @brief Добавить слово в словарный запас проверяющего
     * @param Слово для добавления
     *
     * @note После добавления слово считается корректным до удаления объекта проверяющего.
     *		 Для того, чтобы слово всегда считалось корректным его нужно добавить в
     *		 пользовательский словарь.
     */
    void addWordToChecker(const QString& _word) const;


    /**
     * @brief Текущий язык проверки орфографии
     */
    QString languageCode;

    /**
     * @brief Объект проверяющий орфографию
     */
    QScopedPointer<Hunspell> checker;

    /**
     * @brief Кодировка, которую использует проверяющий
     */
    QTextCodec* checkerTextCodec = nullptr;

    /**
     * @brief Путь к файлу со словарём пользователя
     */
    QString userDictionaryPath;
};

SpellChecker::Implementation::Implementation()
{
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString hunspellDictionariesFolderPath
        = appDataFolderPath + QDir::separator() + "hunspell";
    userDictionaryPath
        = hunspellDictionariesFolderPath + QDir::separator() + "user_dictionary.dict";
}

QString SpellChecker::Implementation::hunspellFilePath(SpellCheckerFileType _fileType) const
{
    //
    // Получим файл со словарём в зависимости от выбранного языка
    //
    QString fileName = languageCode;

    //
    // Определим расширение файла, в зависимости от словаря
    //
    fileName += _fileType == SpellCheckerFileType::Affinity ? ".aff" : ".dic";

    //
    // Сохраним словарь на диск во папку программы, если такового ещё нет
    //
    // ... определяемся с именем файла
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString hunspellDictionariesFolderPath
        = appDataFolderPath + QDir::separator() + "hunspell";
    const QString dictionaryFilePath
        = hunspellDictionariesFolderPath + QDir::separator() + fileName;
    return dictionaryFilePath;
}

void SpellChecker::Implementation::addWordToChecker(const QString& _word) const
{
    if (checker == nullptr || checkerTextCodec == nullptr) {
        return;
    }

    //
    // Преобразуем слово в кодировку словаря и добавляем его в словарный запас
    //
    const auto encodedWord = checkerTextCodec->fromUnicode(_word);
    checker->add(encodedWord.constData());
}


// ****


SpellChecker& SpellChecker::instance()
{
    static SpellChecker spellChecker;
    return spellChecker;
}

SpellChecker::~SpellChecker() = default;

void SpellChecker::setSpellingLanguage(const QString& _languageCode)
{
    if (d->languageCode == _languageCode && d->checker && d->checkerTextCodec) {
        return;
    }

    d->languageCode = _languageCode;

    //
    // Удаляем предыдущего проверяющего
    //
    d->checker.reset();

    //
    // Получаем пути к файлам словарей
    //
    const QFileInfo affFileInfo(d->hunspellFilePath(SpellCheckerFileType::Affinity));
    const QFileInfo dicFileInfo(d->hunspellFilePath(SpellCheckerFileType::Dictionary));
    if (!affFileInfo.exists() || affFileInfo.size() == 0 || !dicFileInfo.exists()
        || dicFileInfo.size() == 0) {
        return;
    }

    //
    // Создаём нового проверяющего
    //
    d->checker.reset(new Hunspell(affFileInfo.absoluteFilePath().toLocal8Bit().constData(),
                                  dicFileInfo.absoluteFilePath().toLocal8Bit().constData()));
    if (d->checker.isNull()) {
        return;
    }
    //
    d->checkerTextCodec = QTextCodec::codecForName(d->checker->get_dic_encoding());
    if (d->checkerTextCodec == nullptr) {
        return;
    }

    //
    // Загружаем слова из пользовательского словаря
    //
    if (!d->userDictionaryPath.isEmpty()) {
        QFile userDictonaryFile(d->userDictionaryPath);
        if (userDictonaryFile.open(QIODevice::ReadOnly)) {
            while (!userDictonaryFile.atEnd()) {
                const QString word = QString::fromUtf8(userDictonaryFile.readLine());
                d->addWordToChecker(word.trimmed());
            }
            userDictonaryFile.close();
        }
    }
}

bool SpellChecker::spellCheckWord(const QString& _word) const
{
    //
    // Если проверяющего орфографию не удалось настроить, то и проверять нет смысла
    //
    if (d->checker == nullptr || d->checkerTextCodec == nullptr) {
        return false;
    }

    //
    // Игнорируем двойной минус, т.к. это служебное обозначение в сценарной записи
    //
    if (_word == "--") {
        return true;
    }

    //
    // Собственно проверка
    //
    QString correctedWord = _word;
    //
    // Для слов заканчивающихся на s с апострофом убираем апостроф в конце, т.к. ханспел его не
    // умеет
    //
    if (d->languageCode.startsWith("en")
        && (correctedWord.endsWith("s'", Qt::CaseInsensitive)
            || correctedWord.endsWith("s’", Qt::CaseInsensitive))) {
        correctedWord.chop(1);
    }

    //
    // Преобразуем слово в кодировку словаря и осуществим проверку
    //
    QByteArray encodedWordData = d->checkerTextCodec->fromUnicode(correctedWord);
    const char* encodedWord = encodedWordData.constData();
    return d->checker->spell(encodedWord);
}

QStringList SpellChecker::suggestionsForWord(const QString& _word) const
{
    if (d->checker == nullptr || d->checkerTextCodec == nullptr) {
        return {};
    }

    //
    // Проверяем необходимость получения списка вариантов
    //
    if (spellCheckWord(_word)) {
        return {};
    }

    //
    // Получим массив вариантов
    //
    char** suggestionsArray;
    const QByteArray encodedWordData = d->checkerTextCodec->fromUnicode(_word);
    const char* encodedWord = encodedWordData.constData();
    int suggestionsCount = d->checker->suggest(&suggestionsArray, encodedWord);
    if (suggestionsCount == 0) {
        return {};
    }

    //
    // Преобразуем массив вариантов в список строк
    //
    QStringList suggestions;
    for (int suggestionIndex = 0; suggestionIndex < suggestionsCount; suggestionIndex++) {
        suggestions.append(d->checkerTextCodec->toUnicode(suggestionsArray[suggestionIndex]));
    }
    //
    // Освобождаем память
    //
    d->checker->free_list(&suggestionsArray, suggestionsCount);
    return suggestions;
}

void SpellChecker::ignoreWord(const QString& _word) const
{
    //
    // Добавим слово в словарный запас проверяющего на текущую сессию
    //
    d->addWordToChecker(_word);
}

void SpellChecker::addWordToDictionary(const QString& _word) const
{
    //
    // Добавим слово в словарный запас проверяющего
    //
    d->addWordToChecker(_word);

    //
    // Запишем слово в пользовательский словарь
    //
    if (!d->userDictionaryPath.isEmpty()) {
        QFile userDictonaryFile(d->userDictionaryPath);
        if (userDictonaryFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            userDictonaryFile.write(_word.toUtf8());
            userDictonaryFile.write("\n");
            userDictonaryFile.close();
        }
    }
}

QString SpellChecker::userDictionaryPath() const
{
    return d->userDictionaryPath;
}

SpellChecker::SpellChecker()
    : d(new Implementation)
{
}

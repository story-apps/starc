/*
 *  Copyright © 2018-2023 Hennadii Chernyshchyk <genaloner@gmail.com>
 *
 *  This file is part of QOnlineTranslator.
 *
 *  QOnlineTranslator is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  QOnlineTranslator is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with QOnlineTranslator. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef QONLINETRANSLATOR_H
#define QONLINETRANSLATOR_H

#include "qexample.h"
#include "qoption.h"

#include <QMap>
#include <QPointer>
#include <QUuid>
#include <QVector>

class QStateMachine;
class QState;
class QNetworkAccessManager;
class QNetworkReply;

/**
 * @brief Provides translation data
 */
class QOnlineTranslator : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QOnlineTranslator)

    friend class QOnlineTts;

public:
    /**
     * @brief Represents all languages for translation
     */
    enum Language {
        NoLanguage = -1,
        Auto,
        Afrikaans,
        Albanian,
        Amharic,
        Arabic,
        Armenian,
        Azerbaijani,
        Bashkir,
        Basque,
        Belarusian,
        Bengali,
        Bosnian,
        Bulgarian,
        Cantonese,
        Catalan,
        Cebuano,
        Chichewa,
        Corsican,
        Croatian,
        Czech,
        Danish,
        Dutch,
        English,
        Esperanto,
        Estonian,
        Fijian,
        Filipino,
        Finnish,
        French,
        Frisian,
        Galician,
        Georgian,
        German,
        Greek,
        Gujarati,
        HaitianCreole,
        Hausa,
        Hawaiian,
        Hebrew,
        HillMari,
        Hindi,
        Hmong,
        Hungarian,
        Icelandic,
        Igbo,
        Indonesian,
        Irish,
        Italian,
        Japanese,
        Javanese,
        Kannada,
        Kazakh,
        Khmer,
        Kinyarwanda,
        Klingon,
        KlingonPlqaD,
        Korean,
        Kurdish,
        Kyrgyz,
        Lao,
        Latin,
        Latvian,
        LevantineArabic,
        Lithuanian,
        Luxembourgish,
        Macedonian,
        Malagasy,
        Malay,
        Malayalam,
        Maltese,
        Maori,
        Marathi,
        Mari,
        Mongolian,
        Myanmar,
        Nepali,
        Norwegian,
        Oriya,
        Papiamento,
        Pashto,
        Persian,
        Polish,
        Portuguese,
        Punjabi,
        QueretaroOtomi,
        Romanian,
        Russian,
        Samoan,
        ScotsGaelic,
        SerbianCyrillic,
        SerbianLatin,
        Sesotho,
        Shona,
        SimplifiedChinese,
        Sindhi,
        Sinhala,
        Slovak,
        Slovenian,
        Somali,
        Spanish,
        Sundanese,
        Swahili,
        Swedish,
        Tagalog,
        Tahitian,
        Tajik,
        Tamil,
        Tatar,
        Telugu,
        Thai,
        Tongan,
        TraditionalChinese,
        Turkish,
        Turkmen,
        Udmurt,
        Uighur,
        Ukrainian,
        Urdu,
        Uzbek,
        Vietnamese,
        Welsh,
        Xhosa,
        Yiddish,
        Yoruba,
        YucatecMaya,
        Zulu
    };
    Q_ENUM(Language)

    /**
     * @brief Represents online engines
     */
    enum Engine {
        Google,
        Yandex,
        Bing,
        LibreTranslate,
        Lingva
    };
    Q_ENUM(Engine)

    /**
     * @brief Indicates all possible error conditions found during the processing of the translation
     */
    enum TranslationError {
        /** No error condition */
        NoError,
        /** Unsupported combination of parameters */
        ParametersError,
        /** Network error */
        NetworkError,
        /** Service unavailable or maximum number of requests */
        ServiceError,
        /** The request could not be parsed (report a bug if you see this) */
        ParsingError
    };

    /**
     * @brief Create object
     *
     * Constructs an object with empty data and with parent.
     * You can use translate() to send text to object.
     *
     * @param parent parent object
     */
    explicit QOnlineTranslator(QObject *parent = nullptr);

    /**
     * @brief Translate text
     *
     * @param text text to translate
     * @param engine online engine to use
     * @param translationLang language to translation
     * @param sourceLang language of the passed text
     * @param uiLang ui language to use for display
     */
    void translate(const QString &text, Engine engine = Google, Language translationLang = Auto, Language sourceLang = Auto, Language uiLang = Auto);

    /**
     * @brief Detect language
     *
     * @param text text for language detection
     * @param engine engine to use
     */
    void detectLanguage(const QString &text, Engine engine = Google);

    /**
     * @brief Cancel translation operation (if any).
     */
    void abort();

    /**
     * @brief Check translation progress
     *
     * @return `true` when the translation is still processing and has not finished or was aborted yet.
     */
    bool isRunning() const;

    /**
     * @brief Converts the object to JSON
     *
     * @return JSON representation
     */
    QJsonDocument toJson() const;

    /**
     * @brief Source text
     *
     * @return source text
     */
    QString source() const;

    /**
     * @brief Source transliteration
     *
     * @return transliteration of the source text
     */
    QString sourceTranslit() const;

    /**
     * @brief Source transcription
     *
     * @return transcription of the source text
     */
    QString sourceTranscription() const;

    /**
     * @brief Source language name
     *
     * @return language name of the source text
     */
    QString sourceLanguageName() const;

    /**
     * @brief Source language
     *
     * @return language of the source text
     */
    Language sourceLanguage() const;

    /**
     * @brief Translated text
     *
     * @return translated text.
     */
    QString translation() const;

    /**
     * @brief Translation transliteration
     *
     * @return transliteration of the translated text
     */
    QString translationTranslit() const;

    /**
     * @brief Translation language name
     *
     * @return language name of the translated text
     */
    QString translationLanguageName() const;

    /**
     * @brief Translation language
     *
     * @return language of the translated text
     */
    Language translationLanguage() const;

    /**
     * @brief Translation options
     *
     * @return QMap whose key represents the type of speech, and the value is a QVector of translation options
     * @sa QOption
     */
    QMap<QString, QVector<QOption>> translationOptions() const;

    /**
     * @brief Translation examples
     *
     * @return QMap whose key represents the type of speech, and the value is a QVector of translation examples
     * @sa QExample
     */
    QMap<QString, QVector<QExample>> examples() const;

    /**
     * @brief Last error
     *
     * Error that was found during the processing of the last translation.
     * If no error was found, returns QOnlineTranslator::NoError.
     * The text of the error can be obtained by errorString().
     *
     * @return last error
     */
    TranslationError error() const;

    /**
     * @brief Last error string
     *
     * A human-readable description of the last translation error that occurred.
     *
     * @return last error string
     */
    QString errorString() const;

    /**
     * @brief Check if source transliteration is enabled
     *
     * @return `true` if source transliteration is enabled
     */
    bool isSourceTranslitEnabled() const;

    /**
     * @brief Enable or disable source transliteration
     *
     * @param enable whether to enable source transliteration
     */
    void setSourceTranslitEnabled(bool enable);

    /**
     * @brief Check if translation transliteration is enabled
     *
     * @return `true` if translation transliteration is enabled
     */
    bool isTranslationTranslitEnabled() const;

    /**
     * @brief Enable or disable translation transliteration
     *
     * @param enable whether to enable translation transliteration
     */
    void setTranslationTranslitEnabled(bool enable);

    /**
     * @brief Check if source transcription is enabled
     *
     * @return `true` if source transcription is enabled
     */
    bool isSourceTranscriptionEnabled() const;

    /**
     * @brief Enable or disable source transcription
     *
     * @param enable whether to enable source transcription
     */
    void setSourceTranscriptionEnabled(bool enable);

    /**
     * @brief Check if translation options are enabled
     *
     * @return `true` if translation options are enabled
     * @sa QOption
     */
    bool isTranslationOptionsEnabled() const;

    /**
     * @brief Enable or disable translation options
     *
     * @param enable whether to enable translation options
     * @sa QOption
     */
    void setTranslationOptionsEnabled(bool enable);

    /**
     * @brief Check if translation examples are enabled
     *
     * @return `true` if translation examples are enabled
     * @sa QExample
     */
    bool isExamplesEnabled() const;

    /**
     * @brief Enable or disable translation examples
     *
     * @param enable whether to enable translation examples
     * @sa QExample
     */
    void setExamplesEnabled(bool enable);

    /**
     * @brief Set the URL engine
     *
     * Only affects LibreTranslate and Lingva because these engines have multiple instances.
     * You need to call this function to specify the URL of an instance for them.
     *
     * @param engine engine
     * @param url engine url
     */
    void setEngineUrl(Engine engine, QString url);

    /**
     * @brief Set api key for engine
     *
     * Affects only LibreTranslate.
     *
     * @param engine engine
     * @param apiKey your key for this particular instance
     */
    void setEngineApiKey(Engine engine, QByteArray apiKey);

    /**
     * @brief Language name
     *
     * @param lang language
     * @return language name
     */
    static QString languageName(Language lang);

    /**
     * @brief Language code
     *
     * @param lang language
     * @return language code
     */
    static QString languageCode(Language lang);

    /**
     * @brief Language
     *
     * @param locale locale
     * @return language
     */
    static Language language(const QLocale &locale);

    /**
     * @brief Language
     *
     * @param langCode code
     * @return language
     */
    static Language language(const QString &langCode);

    /**
     * @brief Check if transliteration is supported
     *
     * @param engine engine
     * @param lang language
     * @return `true` if the specified engine supports transliteration for specified language
     */
    static bool isSupportTranslation(Engine engine, Language lang);

signals:
    /**
     * @brief Translation finished
     *
     * This signal is called when the translation is complete.
     */
    void finished();

private slots:
    void skipGarbageText();

    // Google
    void requestGoogleTranslate();
    void parseGoogleTranslate();

    void requestYandexTranslate();
    void parseYandexTranslate();

    void requestYandexSourceTranslit();
    void parseYandexSourceTranslit();

    void requestYandexTranslationTranslit();
    void parseYandexTranslationTranslit();

    void requestYandexDictionary();
    void parseYandexDictionary();

    // Bing
    void requestBingCredentials();
    void parseBingCredentials();

    void requestBingTranslate();
    void parseBingTranslate();

    void requestBingDictionary();
    void parseBingDictionary();

    // LibreTranslate
    void requestLibreLangDetection();
    void parseLibreLangDetection();

    void requestLibreTranslate();
    void parseLibreTranslate();

    // Lingva
    void requestLingvaTranslate();
    void parseLingvaTranslate();

private:
    /*
     * Engines have translation limit, so need to split all text into parts and make request sequentially.
     * Also Yandex and Bing requires several requests to get dictionary, transliteration etc.
     * We use state machine to rely async computation with signals and slots.
     */
    void buildGoogleStateMachine();
    void buildGoogleDetectStateMachine();

    void buildYandexStateMachine();
    void buildYandexDetectStateMachine();

    void buildBingStateMachine();
    void buildBingDetectStateMachine();

    void buildLibreStateMachine();
    void buildLibreDetectStateMachine();

    void buildLingvaStateMachine();
    void buildLingvaDetectStateMachine();

    // Helper functions to build nested states
    void buildSplitNetworkRequest(QState *parent, void (QOnlineTranslator::*requestMethod)(), void (QOnlineTranslator::*parseMethod)(), const QString &text, int textLimit);
    void buildNetworkRequestState(QState *parent, void (QOnlineTranslator::*requestMethod)(), void (QOnlineTranslator::*parseMethod)(), const QString &text = {});

    // Helper functions for transliteration
    void requestYandexTranslit(Language language);
    void parseYandexTranslit(QString &text);

    void resetData(TranslationError error = NoError, const QString &errorString = {});

    // Check for service support
    static bool isSupportTranslit(Engine engine, Language lang);
    static bool isSupportDictionary(Engine engine, Language sourceLang, Language translationLang);

    // Other
    static QString languageApiCode(Engine engine, Language lang);
    static Language language(Engine engine, const QString &langCode);
    static int getSplitIndex(const QString &untranslatedText, int limit);
    static bool isContainsSpace(const QString &text);
    static void addSpaceBetweenParts(QString &text);

    static const QMap<Language, QString> s_genericLanguageCodes;

    // Engines have some language codes exceptions
    static const QMap<Language, QString> s_googleLanguageCodes;
    static const QMap<Language, QString> s_yandexLanguageCodes;
    static const QMap<Language, QString> s_bingLanguageCodes;
    static const QMap<Language, QString> s_lingvaLanguageCodes;

    // Yandex require a random UUID to be generated
    static inline QByteArray s_yandexUcid = QUuid::createUuid().toByteArray(QUuid::Id128);

    // Credentials that is parsed from the web version to receive the translation using the API
    static inline QByteArray s_bingKey;
    static inline QByteArray s_bingToken;
    static inline QString s_bingIg;
    static inline QString s_bingIid;

    // This properties used to store unseful information in states
    static constexpr char s_textProperty[] = "Text";

    // Engines have a limit of characters per translation request.
    // If the query is larger, then it should be splited into several with getSplitIndex() helper function
    static constexpr int s_googleTranslateLimit = 5000;
    static constexpr int s_yandexTranslateLimit = 150;
    static constexpr int s_yandexTranslitLimit = 180;
    static constexpr int s_bingTranslateLimit = 5001;
    static constexpr int s_libreTranslateLimit = 120;

    QStateMachine *m_stateMachine;
    QNetworkAccessManager *m_networkManager;
    QPointer<QNetworkReply> m_currentReply;

    Language m_sourceLang = NoLanguage;
    Language m_translationLang = NoLanguage;
    Language m_uiLang = NoLanguage;
    TranslationError m_error = NoError;

    QString m_source;
    QString m_sourceTranslit;
    QString m_sourceTranscription;
    QString m_translation;
    QString m_translationTranslit;
    QString m_errorString;

    // Self-hosted engines settings
    QByteArray m_libreApiKey; // Can be empty, since free instances ignores api_key param
    QString m_libreUrl;
    QString m_lingvaUrl;

    QMap<QString, QVector<QOption>> m_translationOptions;
    QMap<QString, QVector<QExample>> m_examples;

    bool m_sourceTranslitEnabled = true;
    bool m_translationTranslitEnabled = true;
    bool m_sourceTranscriptionEnabled = true;
    bool m_translationOptionsEnabled = true;
    bool m_examplesEnabled = true;

    bool m_onlyDetectLanguage = false;
};

#endif // QONLINETRANSLATOR_H

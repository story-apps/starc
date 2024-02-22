#include "text_translate_helper.h"

#include <utils/tools/run_once.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>

#include <NetworkRequest.h>
#include <NetworkRequestLoader.h>

namespace {

/**
 * @brief Максимальное количество символов, которые можно поместить в один запрос
 */
constexpr auto kMaximumCharactersPerRequest = 2500;

/**
 * @brief Таймаут извлечения переводчиков из очереди (40 в минуту)
 */
constexpr auto kDequeTimeoutMsec = 60000 / 40;

/**
 * @brief Ключ автоматического определения исходного текста
 */
const QLatin1String kAutoLanguage("auto");

/**
 * @brief Ключ перевода на английский язык
 */
const QLatin1String kEnglishLanguage("en");

/**
 * @brief Список прокси серверов для подключения к гугл переводчику
 */
static QVector<QNetworkProxy> s_proxies;

/**
 * @brief Выкачать новые прокси для работы
 */
void updateProxies()
{
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Собираем список прокси для работы с переводчиком
    //
    const auto proxiesData
        = NetworkRequestLoader::loadSync("https://starc.app/api/services/gproxy/");
    static const QRegularExpression proxyFinder(
        "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})");
    auto match = proxyFinder.match(proxiesData);
    while (match.hasMatch()) {
        const QString ip = match.captured(1);
        const quint16 port = match.captured(2).toInt();
        s_proxies.append({ QNetworkProxy::HttpProxy, ip, port });

        match = proxyFinder.match(proxiesData, match.capturedEnd());
    }

    //
    // Последней добавляем пустую прокси, чтобы после того, как все закончатся, пробовать работать
    // локально
    //
    s_proxies.append(QNetworkProxy());
}

/**
 * @brief Переключиться на использование следующего прокси
 */
QNetworkProxy nextProxy()
{
    if (!s_proxies.isEmpty()) {
        s_proxies.removeFirst();
    } else {
        updateProxies();
    }

    //
    // Если список прокси так и не удалось обновить, то возвращаем дефолтный прокси
    //
    if (s_proxies.isEmpty()) {
        return {};
    }

    return s_proxies.constFirst();
}

/**
 * @brief Текущий прокси для работы
 */
QNetworkProxy currentProxy()
{
    if (!s_proxies.isEmpty()) {
        return s_proxies.constFirst();
    }

    return {};
}
} // namespace


/**
 * @brief Очередь переводов для ограничения кол-ва запросов на единицу времени
 */
class TranslationQueue
{
public:
    /**
     * @brief Добавить переводчик в очередь на выполнение
     */
    static void enque(TextTranslateHelper* _translator, const QString& _text,
                      const QString& _sourceLanguage, const QString& _targetLanguage);

private:
    TranslationQueue() = default;

    /**
     * @brief Запустить следующий переводчик из очереди
     */
    void deque();


    /**
     * @brief Сама очередь переводчиков
     */
    struct TextTranslatorInfo {
        QPointer<TextTranslateHelper> translator;
        QString sourceText;
        QString sourceLanguage;
        QString targetLanguage;
    };
    QList<TextTranslatorInfo> m_queue;

    /**
     * @brief Дата и время последнего перевода
     */
    QDateTime m_lastTranslationDateTime;
};

void TranslationQueue::enque(TextTranslateHelper* _translator, const QString& _text,
                             const QString& _sourceLanguage, const QString& _targetLanguage)
{
    static TranslationQueue instance;

    //
    // Добавляем переводчика в очередь
    //
    instance.m_queue.append({ _translator, _text, _sourceLanguage, _targetLanguage });

    //
    // Пробуем запустить следующего переводчика (если текущий добавленный всего один в ней)
    //
    instance.deque();
}

void TranslationQueue::deque()
{
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Если в очереди больше никого нет, отдыхаем
    //
    if (m_queue.isEmpty()) {
        return;
    }

    //
    // Если ещё не прошло необходимое кол-во времени до следующего запроса, отдыхаем
    //
    const auto currentDateTime = QDateTime::currentDateTime();
    if (m_lastTranslationDateTime.isValid()
        && m_lastTranslationDateTime.msecsTo(currentDateTime) < kDequeTimeoutMsec) {
        return;
    }

    //
    // Извлечём следующий переводчик из очереди и обработаем его
    //
    auto translatorInfo = m_queue.takeFirst();
    if (translatorInfo.translator.isNull()) {
        //
        // ... если переводчик умер, перейдём к следующему
        //
        deque();
        return;
    }

    //
    // Если с переводчиком всё окей, то сохраняем дату и время запуска последнего из переводчиков
    //
    m_lastTranslationDateTime = currentDateTime;

    //
    // Собственно запускаем перевод
    //
    translatorInfo.translator->translateImpl(
        translatorInfo.sourceText, translatorInfo.sourceLanguage, translatorInfo.targetLanguage);

    //
    // И готовимся к извлечению следующего переводчика из очереди
    //
    QTimer::singleShot(kDequeTimeoutMsec, [this] { deque(); });
}


// ****


TextTranslateHelper::TextTranslateHelper(QObject* _parent)
    : QObject(_parent)
{
}

void TextTranslateHelper::translate(const QString& _text, const QString& _sourceLanguage,
                                    const QString& _targetLanguage)
{
    if (_text.isEmpty() || _targetLanguage.isEmpty()) {
        return;
    }

    TranslationQueue::enque(this, _text, _sourceLanguage, _targetLanguage);
}

void TextTranslateHelper::translateAuto(const QString& _text, const QString& _targetLanguage)
{
    translate(_text, kAutoLanguage, _targetLanguage);
}

void TextTranslateHelper::translateToEnglish(const QString& _text)
{
    translateToEnglish(_text, kAutoLanguage);
}

void TextTranslateHelper::translateToEnglish(const QString& _text, const QString& _sourceLanguage)
{
    translate(_text, _sourceLanguage, kEnglishLanguage);
}

void TextTranslateHelper::translateFromEnglish(const QString& _text, const QString& _targetLanguage)
{
    translate(_text, kEnglishLanguage, _targetLanguage);
}

void TextTranslateHelper::translateImpl(const QString& _text, const QString& _sourceLanguage,
                                        const QString& _targetLanguage)
{
    //
    // Если это новый запрос на перевод, подготовим части для перевода
    //
    if (m_sourceTextParts.isEmpty()) {
        //
        // ... очистим предыдущий перевод
        //
        m_translationParts.clear();
        //
        // ... если нечего переводить, то прерываем операцию
        //
        if (_text.simplified().isEmpty()) {
            return;
        }
        //
        // ... подготовим части исходного текста для перевода
        //
        auto sourceText = _text;
        while (!sourceText.isEmpty()) {
            if (sourceText.length() < kMaximumCharactersPerRequest) {
                m_sourceTextParts.append(sourceText);
                sourceText.clear();
            } else {
                auto sourceTextPart = sourceText.left(kMaximumCharactersPerRequest);
                const QSet<QString> sentenceThreshold = { ".", "!", "?", "…", "\n" };
                while (!sourceTextPart.isEmpty()
                       && !sentenceThreshold.contains(sourceTextPart.right(1))) {
                    sourceTextPart.chop(1);
                }
                const int partLength = sourceTextPart.isEmpty() ? kMaximumCharactersPerRequest
                                                                : sourceTextPart.length();
                m_sourceTextParts.append(sourceText.left(partLength));
                sourceText.remove(0, partLength);
            }
        }
    }

    //
    // Подготовим текст для перевода
    //
    const auto textToTranslate = m_sourceTextParts.takeFirst();

    //
    // Готовим запрос для перевода
    //
    auto request = new NetworkRequest;
    //
    // Настраиваем прокси и используем маленький таймаут, чтобы не ждать долго и переходить к
    // следующим прокси, если одна долго не отвечает, чтобы пользователь долго не ждал
    //
    request->setProxy(currentProxy());
    request->setLoadingTimeout(4000);
    //
    connect(
        request,
        static_cast<void (NetworkRequest::*)(QByteArray, QUrl)>(&NetworkRequest::downloadComplete),
        this,
        [this, textToTranslate, _sourceLanguage, _targetLanguage](const QByteArray& _response) {
            auto translateWithNextProxy
                = [this, textToTranslate, _sourceLanguage, _targetLanguage] {
                      nextProxy();
                      translate(textToTranslate, _sourceLanguage, _targetLanguage);
                  };

            if (_response.isEmpty()) {
                translateWithNextProxy();
                return;
            }

            const auto translationsJson = QJsonDocument::fromJson(_response).array();
            if (translationsJson.isEmpty()) {
                qDebug() << _response;
                return;
                translateWithNextProxy();
                return;
            }

            for (const auto& sentencesJson : translationsJson.at(0).toArray()) {
                const auto sentenceJson = sentencesJson.toArray();
                const auto sourceSentence = sentenceJson.at(1).toString();
                const auto targetSentence = sentenceJson.at(0).toString();

                if (!sourceSentence.isEmpty() && !targetSentence.isEmpty()) {
                    m_translationParts.append({ sourceSentence, targetSentence });
                }
            }

            //
            // При необходимости переходим к переводу следующей части
            //
            if (!m_sourceTextParts.isEmpty()) {
                translateImpl({}, _sourceLanguage, _targetLanguage);
                return;
            }

            //
            // А если все части перевели, то уведомляем пользователя о завершении перевода
            //
            const auto sourceLanguage = _sourceLanguage == kAutoLanguage
                ? translationsJson.at(2).toString()
                : _sourceLanguage;
            emit translated(m_translationParts, sourceLanguage);
        });
    connect(request, &NetworkRequest::finished, request, &NetworkRequest::deleteLater);

    request->setRequestMethod(NetworkRequestMethod::Get);
    QUrl url(QStringLiteral("https://translate.googleapis.com/translate_a/single"));
    url.setQuery(QStringLiteral("client=gtx&ie=UTF-8&oe=UTF-8&dt=bd&dt=ex&dt=ld&dt=md&dt=rw&dt=rm&"
                                "dt=ss&dt=t&dt=at&dt=qc&sl=%1&tl=%2&hl=%3&q=%4")
                     .arg(_sourceLanguage, _targetLanguage, _targetLanguage,
                          QUrl::toPercentEncoding(textToTranslate)));
    request->loadAsync(url);
}

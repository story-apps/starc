#include "text_translate_helper.h"

#include <utils/tools/run_once.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QRegularExpression>

#include <NetworkRequest.h>
#include <NetworkRequestLoader.h>

namespace {
const QLatin1String kAutoLanguage("auto");

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
        this, [this, _text, _sourceLanguage, _targetLanguage](const QByteArray& _response) {
            auto translateWithNextProxy = [this, _text, _sourceLanguage, _targetLanguage] {
                nextProxy();
                translate(_text, _sourceLanguage, _targetLanguage);
            };

            const auto translationsJson = QJsonDocument::fromJson(_response).object();
            if (translationsJson.isEmpty() || !translationsJson.contains("sentences")) {
                translateWithNextProxy();
                return;
            }

            const auto sentencesJson = translationsJson["sentences"].toArray();
            if (sentencesJson.isEmpty()) {
                translateWithNextProxy();
                return;
            }

            QVector<Translation> translation;
            for (const auto& sentenceJson : sentencesJson) {
                translation.append({
                    sentenceJson["orig"].toString(),
                    sentenceJson["trans"].toString(),
                });
            }

            const auto sourceLanguage = _sourceLanguage == kAutoLanguage
                ? translationsJson["src"].toString()
                : _sourceLanguage;

            emit translated(translation, sourceLanguage);
        });
    connect(request, &NetworkRequest::finished, request, &NetworkRequest::deleteLater);

    //
    // Запрос на генерацию текста выполняем постом, чтобы не светить ключ
    //
    request->setRequestMethod(NetworkRequestMethod::Post);
    QByteArray requestData;
    requestData.append("sl=" + _sourceLanguage.toUtf8());
    requestData.append("&tl=" + _targetLanguage.toUtf8());
    requestData.append("&q=" + _text.toUtf8().toPercentEncoding());
    request->setRawRequestData(requestData, "application/x-www-form-urlencoded;charset=utf-8");
    const QUrl url("http://translate.google.com/translate_a/single?client=at&dt=t&dj=1");
    request->loadAsync(url);
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
    translate(_text, _sourceLanguage, QLatin1String("en"));
}

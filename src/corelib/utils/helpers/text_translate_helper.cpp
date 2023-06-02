#include "text_translate_helper.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <NetworkRequest.h>

namespace {
const QLatin1String kAutoLanguage("auto");
}


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
    connect(
        request,
        static_cast<void (NetworkRequest::*)(QByteArray, QUrl)>(&NetworkRequest::downloadComplete),
        this, [this, _text, _sourceLanguage](const QByteArray& _response) {
            const auto translationsJson = QJsonDocument::fromJson(_response).object();
            if (translationsJson.isEmpty() || !translationsJson.contains("sentences")) {
                //
                // FIXME: что делать, если перевод завершился неудачно?
                //
                emit textTranslated({}, {});
                return;
            }

            const auto sentencesJson = translationsJson["sentences"].toArray();
            if (sentencesJson.isEmpty()) {
                //
                // FIXME: что делать, если перевод завершился неудачно?
                //
                emit textTranslated({}, {});
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

            emit textTranslated(translation, sourceLanguage);
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
    const QUrl url("https://translate.google.com/translate_a/single?client=at&dt=t&dj=1");
    request->loadAsync(url);
}

void TextTranslateHelper::translateAuto(const QString& _text, const QString& _targetLanguage)
{
    translate(_text, kAutoLanguage, _targetLanguage);
}

void TextTranslateHelper::translateToEnglish(const QString& _text)
{
    translateAuto(_text, QLatin1String("en"));
}

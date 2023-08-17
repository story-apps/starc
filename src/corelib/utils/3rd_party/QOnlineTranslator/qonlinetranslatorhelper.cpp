#include "qonlinetranslatorhelper.h"


QOnlineTranslatorHelper::QOnlineTranslatorHelper(QObject* _parent)
    : QObject(_parent)
{
}

void QOnlineTranslatorHelper::translate(const QString& _text, QOnlineTranslator::Engine _engine,
                                        QOnlineTranslator::Language _translationLang,
                                        QOnlineTranslator::Language _sourceLang)
{
    auto translator = new QOnlineTranslator(this);
    connect(translator, &QOnlineTranslator::finished, this,
            [this, translator, _text, _engine, _translationLang, _sourceLang] {
                if (translator->translation().isEmpty()) {
                    translator->translate(_text, QOnlineTranslator::Engine(_engine + 1),
                                          _translationLang, _sourceLang);
                    return;
                }

                const QString translation = translator->translation();
                const QOnlineTranslator::Language sourceLanguage = translator->sourceLanguage();
                translator->deleteLater();
                emit finished(translation, sourceLanguage);
            });
    translator->translate(_text, _engine, _translationLang, _sourceLang);
}

#pragma once

#include "qonlinetranslator.h"


class QOnlineTranslatorHelper : public QObject
{
    Q_OBJECT

public:
    explicit QOnlineTranslatorHelper(QObject* _parent = nullptr);

    void translate(const QString& _text,
                   QOnlineTranslator::Engine _engine = QOnlineTranslator::Google,
                   QOnlineTranslator::Language _translationLang = QOnlineTranslator::Auto,
                   QOnlineTranslator::Language _sourceLang = QOnlineTranslator::Auto);

signals:
    void finished(const QString& _translation, QOnlineTranslator::Language _sourceLang);
};

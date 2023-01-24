#pragma once

#include <QObject>

#include <corelib_global.h>


class CORE_LIBRARY_EXPORT TextTranslateHelper : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Данные перевода
     */
    struct Translation {
        QString original;
        QString translation;
    };

public:
    explicit TextTranslateHelper(QObject* _parent = nullptr);

    /**
     * @brief Переверсти текст с исходного языка на заданный
     */
    void translate(const QString& _text, const QString& _sourceLanguage,
                   const QString& _targetLanguage);

    /**
     * @brief Перевести текст на заданный, автоматически определив исходный язык
     */
    void translateAuto(const QString& _text, const QString& _targetLanguage);

    /**
     * @brief Переверсти текст на английский, автоматически определив исходный язык
     */
    void translateToEnglish(const QString& _text);

signals:
    /**
     * @brief Текст был переведён
     */
    void textTranslated(const QVector<TextTranslateHelper::Translation>& _translation,
                        const QString& _sourceLanguage);
};

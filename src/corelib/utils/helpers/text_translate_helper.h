#pragma once

#include <QObject>

#include <corelib_global.h>


/**
 * @brief Переводчик гугл
 * @note Максимально можно делать 40 запросов в минуту и 5 запросов в секунду
 */
class CORE_LIBRARY_EXPORT TextTranslateHelper : public QObject
{
    Q_OBJECT

    friend class TranslationQueue;

public:
    /**
     * @brief Переведённые фразы
     */
    struct TranslationPart {
        QString source;
        QString translation;
    };

    /**
     * @brief Данные перевода
     */
    struct Translation {
        bool isValid() const
        {
            return !sourceLanguage.isEmpty() && !translationLanguage.isEmpty() && !text.isEmpty();
        }

        QString sourceLanguage;
        QString translationLanguage;
        QVector<TranslationPart> text;
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
    void translateToEnglish(const QString& _text, const QString& _sourceLanguage);

    /**
     * @brief Перевести с английского на заданный язык
     */
    void translateFromEnglish(const QString& _text, const QString& _targetLanguage);

signals:
    /**
     * @brief Текст был переведён
     */
    void translated(const TextTranslateHelper::Translation& _translation);

private:
    /**
     * @brief Реализация метода перевода текста
     */
    void translateImpl(const QString& _text, const QString& _sourceLanguage,
                       const QString& _targetLanguage);

private:
    /**
     * @brief Части исходного текста (разбитые в соответствии с ограничениями на перевод)
     */
    QVector<QString> m_sourceTextParts;

    /**
     * @brief Перевод текста
     */
    Translation m_translation;
};

#pragma once

#include <domain/document_object.h>

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVector>


namespace ManagementLayer {

/**
 * Local bridge between Story Architect's AI assistant and `codex app-server`.
 *
 * The bridge intentionally uses the user's existing Codex login. It does not
 * store an API key or write directly to a STARC project database.
 */
class CodexServiceManager : public QObject
{
    Q_OBJECT

public:
    explicit CodexServiceManager(QObject* _parent = nullptr);
    ~CodexServiceManager() override;

public slots:
    void aiRephraseText(const QString& _source, const QString& _style);
    void aiExpandText(const QString& _source);
    void aiShortenText(const QString& _source);
    void aiInsertText(const QString& _after, const QString& _before);
    void aiSummarizeText(const QString& _source);
    void aiTranslateText(const QString& _source, const QString& _languageCode);
    void aiTranslateDocument(const QVector<QString>& _texts, const QString& _languageCode,
                             Domain::DocumentObjectType _type, int _wordsRequired);
    void aiGenerateSynopsis(const QVector<QString>& _scenes, int _maxWordsPerScene,
                            int _wordsRequired);
    void aiGenerateNovel(const QVector<QString>& _scenes, int _wordsRequired);
    void aiGenerateScript(const QVector<QString>& _chapters, int _wordsRequired);
    void aiGenerateText(const QString& _promptPrefix, const QString& _prompt,
                        const QString& _promptSuffix);
    void cancelCurrentTask();

signals:
    void textRephrased(const QString& _text);
    void textExpanded(const QString& _text);
    void textShortened(const QString& _text);
    void textInserted(const QString& _text);
    void textSummarizeed(const QString& _text);
    void textTranslated(const QString& _text);
    void documentTranslated(const QVector<QString>& _texts);
    void synopsisGenerated(const QString& _text);
    void novelGenerated(const QString& _text);
    void scriptGenerated(const QString& _text);
    void textGenerated(const QString& _text);

    /** Emitted for a skill run that creates storyboard artifacts in the workspace. */
    void storyboardGenerated(const QString& _summary);
    void activityChanged(const QString& _activity);
    void busyChanged(bool _busy);
    void taskCancelled();
    void errorOccurred(const QString& _message);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

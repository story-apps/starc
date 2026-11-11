#pragma once

#include <corelib_global.h>
#include "ai_project_context.h"
#include <QSet>
#include <QUrl>
#include <memory>

#include <QJsonArray>
#include <QObject>
#include <QSqlDatabase>
#include <QTimer>
#include <QUuid>
#include <functional>

namespace Ui {

/** Local, project-scoped conversations. Document contents are read only at send time and
 * never stored in the transcript. Requests retain their original conversation identity. */
class CORE_LIBRARY_EXPORT AiChatSession : public QObject
{
    Q_OBJECT
public:
    explicit AiChatSession(const QString& _databasePath, QObject* _parent = nullptr);
    ~AiChatSession() override;
    static AiChatSession* instance();

    enum class ContextScope { Document, Project };
    ContextScope contextScope() const;
    void setContextScope(ContextScope _scope);
    // The provider checks availability first (false), then captures content once (true).
    void setProjectDocumentsProvider(std::function<QVector<AiContextDocument>(bool)> _provider);
    QString preparationStatus() const;
    bool openSource(const QUrl& _url);
    void setProject(const QUuid& _project, const QString& _account);
    void setAccount(const QString& _account);
    void setDocument(const QUuid& _document, const QString& _name,
                     std::function<QString()> _content);
    QJsonArray chats() const;
    QJsonArray messages() const;
    QString currentChat() const;
    QString documentName() const;
    bool createChat();
    bool renameChat(const QString& _title);
    QString documentText() const;
    QString documentId() const;
    bool startTool(const QString& _message, const QString& _action, int _replyCount = 1);
    bool isToolRequest(const QString& _action) const;
    bool toolTargetsCurrentDocument() const;
    bool finishToolReply(const QString& _action, const QString& _text);
    bool selectChat(const QString& _id);
    bool deleteChat();
    bool clearChat();
    bool send(const QString& _text);
    bool hasPendingReply() const;
    bool isWaitingForCurrentChat() const;
    // Returns false only for a legacy generation explicitly requested by an editor.
    bool finishReply(const QString& _text);
    void failReply(const QString& _error, bool _releaseTransport = true);
    bool beginLegacyRequest();
    void setBackendAvailable(bool _available);
    bool canSend() const;

signals:
    void changed();
    void errorOccurred(const QString& _error);
    void replyRequested(const QString& _prompt);
    void projectDocumentRequested(const QUuid& _project, const QUuid& _document);
    void sourceRequested(const QUuid& _document, const QUuid& _scene);

private:
    bool append(const QString& _chat, const QString& _role, const QString& _text,
                const QString& _document = {});
    bool ensureContext(bool* _added = nullptr);
    bool check(bool _success);
    void updateAutomaticTitle(const QString& _message);
    void restoreScope();
    QString nextSelectionTimestamp() const;
    void prepareProjectContext();
    void dispatchContextRequest(const QString& _question, const QJsonArray& _history);
    void dispatchRetrieval(const QJsonObject& _result, const QString& _instruction = {});
    void rememberSources(const QJsonObject& _result);
    QJsonArray sourceCoverage() const;
    QString resolveCitations(const QString& _text);

    ContextScope m_scope = ContextScope::Document;
    std::function<QVector<AiContextDocument>(bool)> m_projectDocuments;
    std::shared_ptr<AiProjectContext> m_projectIndex;
    std::shared_ptr<AiProjectContext> m_pendingIndex;
    QTimer m_preparationTimer;
    QSet<QString> m_requestedDocuments;
    QVector<AiContextDocument> m_preparingDocuments;
    int m_preparingIndex = -1;
    QString m_preparationStatus;
    QString m_question, m_basePrompt, m_pendingProject, m_searchNotes;
    QString m_completeDocument;
    QJsonArray m_requestHistory, m_evidencePages;
    QHash<QString, QJsonObject> m_pendingSources;
    int m_retrievalRounds = 0;
    bool m_pendingProjectScope = false;
    bool m_citationRepair = false;
    bool m_protocolRepair = false;

    QSqlDatabase m_database;
    QString m_connection;
    QString m_project;
    QString m_account;
    QString m_chat;
    QString m_document;
    QString m_documentName;
    std::function<QString()> m_content;
    QString m_pendingChat;
    bool m_backendAvailable = false;
    enum class RequestKind { None, Preparing, Chat, Legacy, Tool };
    RequestKind m_requestKind = RequestKind::None;
    QTimer m_replyTimer;
    QString m_toolAction;
    QString m_toolDocument;
    QString m_toolDocumentName;
    QString m_toolContent;
    int m_toolRepliesRemaining = 0;
    QString m_toolProject;
    QString m_toolAccount;
    bool m_toolDispatched = false;
};

} // namespace Ui

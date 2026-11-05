#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>

#include <QUuid>

namespace Domain {
struct CursorInfo;
}

namespace Ui {

/**
 * @brief Единый индикатор подключения и синхронизации текущего облачного проекта
 */
class ConnectionStatusToolBar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ConnectionStatusToolBar(QWidget* _parent = nullptr);
    ~ConnectionStatusToolBar() override;

    void setConnectionAvailable(bool _available);
    void setCollaborators(const QVector<Domain::CursorInfo>& _collaborators);
    QSize sizeHint() const override;

    /**
     * @brief Сбросить состояние при открытии/закрытии проекта
     */
    void setCloudProjectOpen(bool _open);
    void setSyncError(const QString& _error);
    void setUploadError(const QString& _error);

    /**
     * @brief Правки остаются ожидающими до подтверждения сервера
     */
    void setDocumentPending(const QUuid& _uuid);
    void setUploadStarted(const QUuid& _uuid);
    void setUploadFinished(const QUuid& _uuid, bool _hasPendingChanges);
    void removeDocument(const QUuid& _uuid);
    void setDocumentClosed(const QUuid& _uuid);

    /**
     * @brief Подписка на документ или загрузка служебного документа с ещё неизвестным UUID
     * @note Сервер не отвечает на повторную подписку в рамках одного соединения.
     */
    void setDownloadStarted(const QUuid& _uuid);
    void setDownloadStarted(int _documentType);
    void setDownloadFinished(const QUuid& _uuid, int _documentType, bool _receivedChanges = true);

signals:
    void checkConnectionPressed();
    void collaboratorClicked(const QString& _cursorId);

protected:
    bool event(QEvent* _event) override;
    bool eventFilter(QObject* _watched, QEvent* _event) override;
    void keyPressEvent(QKeyEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void paintEventPostprocess(QPainter& _painter) override;
    void updateTranslations() override;
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

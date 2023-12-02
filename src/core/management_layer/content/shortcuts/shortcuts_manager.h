#pragma once

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Управляющий сочетаниями клавиш приложения
 */
class ShortcutsManager : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutsManager(QObject* _parent = nullptr);
    ~ShortcutsManager() override;

    QVector<QString> shortcutNames() const;
    QKeySequence shortcutByName(const QString& _name) const;
    void setShortcutByName(const QString& _name, const QKeySequence& _shortcut);

    QKeySequence importShortcut() const;
    void setImportShortcut(const QKeySequence& _shortcut);
    Q_SIGNAL void importShortcutChanged(const QKeySequence& _key);

    QKeySequence currentDocumentExportShortcut() const;
    void setExportShortcut(const QKeySequence& _shortcut);
    Q_SIGNAL void exportShortcutChanged(const QKeySequence& _key);

    void updateTranslations();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

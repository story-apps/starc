#pragma once

#include <QObject>


namespace ManagementLayer
{

/**
 * @brief Управляющий импортом документов
 */
class ImportManager : public QObject
{
    Q_OBJECT

public:
    ImportManager(QObject* _parent, QWidget* _parentWidget);
    ~ImportManager() override;

    /**
     * @brief Запустить процесс импорта данных
     */
    void import();

signals:
    /**
     * @brief Сценарий загружен
     */
    void screenplayImported(const QString& _name, const QString& _titlePage,
        const QString& _synopsis, const QString& _outline, const QString& _screenplay);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

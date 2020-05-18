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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

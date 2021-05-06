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

    /**
     * @brief Импортировать все возможные данные из указанного файла
     */
    void import(const QString& _filePath);

signals:
    /**
     * @brief Персонаж загружен
     */
    void characterImported(const QString& _name, const QString& _content);

    /**
     * @brief Локация загружена
     */
    void locationImported(const QString& _name, const QString& _content);

    /**
     * @brief Сценарий загружен
     */
    void screenplayImported(const QString& _name, const QString& _titlePage,
        const QString& _synopsis, const QString& _treatment, const QString& _screenplay);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

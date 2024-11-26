#pragma once

#include <QObject>

namespace BusinessLayer {
class AbstractModel;
}

namespace ManagementLayer {

/**
 * @brief Управляющий экспортом документов
 */
class ExportManager : public QObject
{
    Q_OBJECT
public:
    explicit ExportManager(QObject* _parent, QWidget* _parentWidget);
    ~ExportManager() override;

    /**
     * @brief Можно ли экспортировать данный документ
     */
    bool canExportDocument(BusinessLayer::AbstractModel* _model) const;

    /**
     * @brief Экспортировать заданный документ
     */
    void exportDocument(const QVector<QPair<QString, BusinessLayer::AbstractModel*>>& _models,
                        int _currentModelIndex);

    /**
     * @brief Экспортировать заданный документ в заданный файл
     */
    void exportDocument(BusinessLayer::AbstractModel* _model, const QString& _filePath);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

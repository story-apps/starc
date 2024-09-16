#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

#include <QObject>

class QHBoxLayout;
class QVBoxLayout;

namespace BusinessLayer {
enum class ExportFileFormat;
struct ExportOptions;
} // namespace BusinessLayer

namespace Ui {

/**
 * @brief Абстрактный диалог экспорта
 */
class AbstractExportDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit AbstractExportDialog(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                                  const QString& _uuidKey, QWidget* _parent = nullptr);
    ~AbstractExportDialog() override;

    /**
     * @brief Нужно ли открыть экспортированный документ после экспорта
     */
    bool openDocumentAfterExport() const;

    /**
     * @brief Получить опции экспорта
     */
    virtual BusinessLayer::ExportOptions& exportOptions() const;

signals:
    /**
     * @brief Пользователь хочет экспортировать сценарий с заданными параметрами
     */
    void exportRequested();

    /**
     * @brief Пользователь передумал импортировать данные
     */
    void canceled();

protected:
    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновить диалог
     */
    virtual void updateDialog() const;

    /**
     * @brief Обновить видимость параметров
     */
    virtual void updateParametersVisibility() const = 0;

    /**
     * @brief Обновить состояние кнопки "Экспортировать"
     */
    virtual void updateExportEnabled() const;

    /**
     * @brief Должа ли быть активна кнопка "Экспортировать"
     */
    virtual bool isExportEnabled() const;

    /**
     * @brief Получить формат файла в соответствии со строкой комбобокса
     */
    BusinessLayer::ExportFileFormat currentFileFormat() const;

    /**
     * @brief Получить номер текущей строки комбобокса
     */
    int currentFileFormatRow() const;

    /**
     * @brief Установить текущий формат файла в комбобокс
     */
    void setCurrentFileFormat(int _row) const;

    /**
     * @brief Задать видимость параметра "Водяной знак"
     */
    void setWatermarkVisible(bool _isVisible) const;

    /**
     * @brief Получить компоновщик правого контента
     */
    QVBoxLayout* leftLayout() const;

    /**
     * @brief Получить компоновщик правого контента
     * @note Уже имеет два виджета (fileFormat и watermark) и stratch
     */
    QVBoxLayout* rightLayout() const;

    /**
     * @brief Получить компоновщик нижнего контента
     * @note Уже имеет чекбокс (openDocumentAfterExport) и две кнопки (cancelButton и exportButton)
     */
    QHBoxLayout* bottomLayout() const;

    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Уникальный ключ для сохранения настроек
     */
    QString settingsKey(const QString& _parameter) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

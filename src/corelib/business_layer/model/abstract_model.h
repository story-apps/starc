#pragma once

#include <corelib_global.h>

#include <QAbstractItemModel>

namespace Domain {
    class DocumentObject;
}


namespace BusinessLayer {

class AbstractImageWrapper;

/**
 * @brief Абстрактная модель для работы над документами
 */
class CORE_LIBRARY_EXPORT AbstractModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AbstractModel(const QVector<QString>& _tags, QObject* _parent = nullptr);
    ~AbstractModel() override;

    /**
     * @brief Получить документ модели
     */
    Domain::DocumentObject* document() const;

    /**
     * @brief Задать документ со структурой
     */
    void setDocument(Domain::DocumentObject* _document);

    /**
     * @brief Задать название документа
     */
    virtual void setDocumentName(const QString& _name);

    /**
     * @brief Установить загрузчик изображений
     */
    void setImageWrapper(AbstractImageWrapper* _image);

    /**
     * @brief Очистить все загруженные данные
     */
    void clear();

    /**
     * @brief Применить заданное изменение для модели
     */
    virtual void applyPatch(const QByteArray& _patch);

    /**
     * @brief Отменить последнее изменение текущего пользователя
     */
    void undo();

    /**
     * @brief Отменить заданное изменение
     */
    void undoChange(const QByteArray& _undo, const QByteArray& _redo);

    /**
     * @brief Повторить отменённое действие
     */
    void redo();

    /**
     * @brief Реализация базовых вещей для древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount( const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex &_parent = {}) const override;
    QVariant data(const QModelIndex &_index, int _role) const override;
    /** @} */

signals:
    /**
     * @brief Изменилось название документа модели
     */
    void documentNameChanged(const QString& _name);

    /**
     * @brief Модель хочет отменить изменение с заданным индексом изменения в истории
     */
    void undoRequested(int _undoStep);

    /**
     * @brief Данные изменились
     */
    void contentsChanged(const QByteArray& _undo, const QByteArray& _redo);

protected:
    /**
     * @brief Настроить документ
     */
    virtual void initDocument() = 0;

    /**
     * @brief Очистить документ
     */
    virtual void clearDocument() = 0;

    /**
     * @brief Сформировать xml-содержимое документа
     */
    virtual QByteArray toXml() const = 0;

    /**
     * @brief Получить обёртку для работы с изображениями
     */
    AbstractImageWrapper* imageWrapper() const;

    /**
     * @brief Установить данные в документ
     */
    void updateDocumentContent();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

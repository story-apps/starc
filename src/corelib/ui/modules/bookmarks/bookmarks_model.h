#pragma once

#include <QAbstractListModel>

#include <corelib_global.h>


namespace BusinessLayer {

enum class TextParagraphType;
class TextModel;

/**
 * @brief Модель закладок текстового документа
 */
class CORE_LIBRARY_EXPORT BookmarksModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        BookmarkColorRole = Qt::UserRole + 1,
        BookmarkNameRole,
        BookmarkItemTextRole,
    };

public:
    explicit BookmarksModel(QObject* _parent = nullptr);
    ~BookmarksModel() override;

    /**
     * @brief Искать закладки только в блоках заданных в списке
     * @note Если список пустой, то закладки будут искаться везде
     */
    void setParagraphTypesFiler(const QVector<TextParagraphType>& _types);

    /**
     * @brief Задать модель текста сценария
     */
    void setTextModel(BusinessLayer::TextModel* _model);

    /**
     * @brief Получить индекс элемента из модели сценария, в котором находится закладка
     */
    QModelIndex mapToModel(const QModelIndex& _index);

    /**
     * @brief Получить индекс закладки из индекса элемента модели сценария
     */
    QModelIndex mapFromModel(const QModelIndex& _index);

    /**
     * @brief Задать название закладки
     */
    void setName(const QModelIndex& _index, const QString& _name);

    /**
     * @brief Задать цвет закладки
     */
    void setColor(const QModelIndex& _index, const QColor& _color);

    /**
     * @brief Удалить выбранные элементы
     */
    void remove(const QModelIndexList& _indexes);

    /**
     * @brief Реализация модели списка
     */
    int rowCount(const QModelIndex& _parent = {}) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

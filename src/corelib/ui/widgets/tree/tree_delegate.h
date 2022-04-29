#pragma once

#include <QStyledItemDelegate>

#include <corelib_global.h>


/**
 * @brief Базовый делегат отрисовки элементов дерева
 */
class CORE_LIBRARY_EXPORT TreeDelegate : public QStyledItemDelegate
{
public:
    explicit TreeDelegate(QObject* _parent = nullptr);

    /**
     * @brief Задать дополнительный отступ слева
     */
    void setAdditionalLeftMargin(qreal _margin);

    /**
     * @brief Задать иконку для отображения в конце элемента при наведении мыши
     */
    void setHoverTrailingIcon(const QString _icon);

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option,
               const QModelIndex& _index) const override;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

    /**
     * @brief Вспомогательные методы, чтобы запоминать для какого индекса был создан редактор
     */
    void setEditorData(QWidget* _editor, const QModelIndex& _index) const override;
    void updateEditorGeometry(QWidget* _editor, const QStyleOptionViewItem& _option,
                              const QModelIndex& _index) const override;
    void destroyEditor(QWidget* editor, const QModelIndex& index) const override;

protected:
    /**
     * @brief Дополнительный отступ слева
     */
    qreal m_additionalLeftMargin = 0.0;

    /**
     * @brief Иконка показываемая в конце элемента, при наведении
     */
    QString m_hoverTrailingIcon;

    /**
     * @brief Индекс элемента, который в данный момент редактируется
     */
    mutable QModelIndex m_editorActiveFor;
};

/**
 * @brief Делегат для редактирования шорткатов в дереве
 */
class CORE_LIBRARY_EXPORT KeySequenceDelegate : public TreeDelegate
{
    Q_OBJECT

public:
    explicit KeySequenceDelegate(QObject* _parent = nullptr);

    QWidget* createEditor(QWidget* _parent, const QStyleOptionViewItem& _option,
                          const QModelIndex& _index) const override;
    void setEditorData(QWidget* _editor, const QModelIndex& _index) const override;
    void setModelData(QWidget* _editor, QAbstractItemModel* _model,
                      const QModelIndex& _index) const override;
};

/**
 * @brief Делегат для редактировнаия текста в дереве
 */
class CORE_LIBRARY_EXPORT TextFieldItemDelegate : public TreeDelegate
{
    Q_OBJECT

public:
    explicit TextFieldItemDelegate(QObject* _parent = nullptr);

    /**
     * @brief Установить текст, который будет использоваться как лейбл текстового поля
     */
    void setLabel(const QString& _label);

    QWidget* createEditor(QWidget* _parent, const QStyleOptionViewItem& _option,
                          const QModelIndex& _index) const override;
    void setEditorData(QWidget* _editor, const QModelIndex& _index) const override;
    void setModelData(QWidget* _editor, QAbstractItemModel* _model,
                      const QModelIndex& _index) const override;

private:
    /**
     * @brief Лейбл текстового поля
     */
    QString m_label;
};

/**
 * @brief Делегат для редактировнаия выпадающего списка в дереве
 */
class CORE_LIBRARY_EXPORT ComboBoxItemDelegate : public TreeDelegate
{
    Q_OBJECT

public:
    explicit ComboBoxItemDelegate(QObject* _parent = nullptr, QAbstractItemModel* _model = nullptr);

    /**
     * @brief Установить текст, который будет использоваться как лейбл комбобокса
     */
    void setLabel(const QString& _label);

    QWidget* createEditor(QWidget* _parent, const QStyleOptionViewItem& _option,
                          const QModelIndex& _index) const override;
    void setEditorData(QWidget* _editor, const QModelIndex& _index) const override;
    void setModelData(QWidget* _editor, QAbstractItemModel* _model,
                      const QModelIndex& _index) const override;

private:
    /**
     * @brief Лейбл комбобокса
     */
    QString m_label;

    /**
     * @brief Модель элементов выпадающего списка
     */
    QAbstractItemModel* m_model = nullptr;
};

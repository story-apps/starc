#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/widgets/widget/widget.h>

namespace BusinessLayer {
class SimpleTextModel;
}


namespace Ui {

/**
 * @brief Представление редактора текстового документа
 */
class SimpleTextView : public Widget, public IDocumentView
{
    Q_OBJECT

public:
    explicit SimpleTextView(QWidget* _parent = nullptr);
    ~SimpleTextView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    /** @{ */
    QWidget* asQWidget() override;
    void toggleFullScreen(bool _isFullScreen) override;
    QVector<QAction*> options() const override;
    void setEditingMode(ManagementLayer::DocumentEditingMode _mode) override;
    void setCursors(const QVector<Domain::CursorInfo>& _cursors) override;
    /** @{ */

    /**
     * @brief Настроить редактор сценария в соответствии с параметрами заданными в настройках
     */
    void reconfigure(const QStringList& _changedSettingsKeys);

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadViewSettings();
    void saveViewSettings();

    /**
     * @brief Установить модель текста
     */
    void setModel(BusinessLayer::SimpleTextModel* _model);

    /**
     * @brief Получить индекс элемента модели в текущей позиции курсора
     */
    QModelIndex currentModelIndex() const;

    /**
     * @brief Поставить курсор в позицию элемента с заданным индексом модели текста
     */
    void setCurrentModelIndex(const QModelIndex& _index);

    /**
     * @brief Позиция курсора
     */
    int cursorPosition() const;
    void setCursorPosition(int _position);

    /**
     * @brief Положение прокрутки
     */
    int verticalScroll() const;
    void setverticalScroll(int _value);

signals:
    /**
     * @brief Изменился индекс текущего элемента модели в текстовом документе (перестился курсор)
     */
    void currentModelIndexChanged(const QModelIndex& _index);

    /**
     * @brief Нажатия пользователя в контекстном меню для активации действия
     */
    void addBookmarkRequested();
    void editBookmarkRequested();

    /**
     * @brief Уведомления, что пользователь осуществил действие в панели редактирования закладки
     */
    void createBookmarkRequested(const QString& _text, const QColor& _color);
    void changeBookmarkRequested(const QModelIndex& _index, const QString& _text,
                                 const QColor& _color);

    /**
     * @brief Пользователь хочет удалить закладку
     */
    void removeBookmarkRequested();

    /**
     * @brief Изменилась позиция курсора
     */
    void cursorChanged(const QByteArray& _cursorData);

protected:
    /**
     * @brief Фильтруем события для корректировки панелей
     */
    bool eventFilter(QObject* _target, QEvent* _event) override;

    /**
     * @brief Переопределяем для корректировки положения тулбара действий над проектами
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

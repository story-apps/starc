#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


namespace BusinessLayer {
class StructureModelItem;
}

namespace Ui {

/**
 * @brief Представление проекта
 */
class ProjectView : public StackWidget
{
    Q_OBJECT

public:
    explicit ProjectView(QWidget* _parent = nullptr);
    ~ProjectView() override;

    /**
     * @brief Показать умолчальную страницу
     */
    void showDefaultPage();

    /**
     * @brief Показать страницу с информацией о том, что документ загружается
     */
    void showDocumentLoadingPage();

    /**
     * @brief Показать страницу с информацией о том, что это ещё не реализовано
     */
    void showNotImplementedPage();

    /**
     * @brief Текущий редактор
     */
    QWidget* currentEditor() const;

    /**
     * @brief Отобразить заданный редактор
     */
    void showEditor(QWidget* _widget);

    /**
     * @brief Добавить заданный редактор без отображения
     */
    void addEditor(QWidget* _widget);

    /**
     * @brief Активно ли представление в данный момент
     */
    void setActive(bool _active);

    /**
     * @brief Задать драфты документа для отображения
     */
    void setDocumentDraft(const QVector<BusinessLayer::StructureModelItem*>& _drafts);

    /**
     * @brief Настроить видимость списка драфтов документа
     */
    void setDraftsVisible(bool _visible);

    /**
     * @brief Задать текущий драфт
     */
    int currentDraft() const;
    void setCurrentDraft(int _index);

signals:
    /**
     * @brief Пользователь нажал кнопку создания нового проекта
     */
    void createNewItemPressed();

    /**
     * @brief Пользователь хочет отобразить драфт с заданным индексом
     * @note 0 - текущий драфт, последующие индексы идут увеличнными на единицу
     */
    void showDraftPressed(int _versionIndex);

    /**
     * @brief Пользователь вызвал контекстное меню драфта
     */
    void showDraftContextMenuPressed(int _versionIndex);

protected:
    /**
     * @brief Отслеживаем изменение размера, чтобы скорректировать размер оверлея
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
    /**
     * @brief Перекрываем реализацию базового класса, чтобы клиенты не использовали её, а
     * использовали альтернативный метод для установки редактора документа
     */
    void setCurrentWidget(QWidget* _widget);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

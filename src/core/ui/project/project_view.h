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
     * @brief Показать страницу с информацией о том, что это ещё не реализовано
     */
    void showNotImplementedPage();

    /**
     * @brief Отобразить заданный редактор
     */
    void showEditor(QWidget* _widget);

    /**
     * @brief Сфокусировать текущий редактор
     */
    void focusEditor();

    /**
     * @brief Активно ли представление в данный момент
     */
    void setActive(bool _active);

    /**
     * @brief Задать версии документа для отображения
     */
    void setDocumentVersions(const QVector<BusinessLayer::StructureModelItem*>& _versions);

    /**
     * @brief Настроить видимость списка версий документа
     */
    void setVersionsVisible(bool _visible);

    /**
     * @brief Задать текущую версию
     */
    int currentVersion() const;
    void setCurrentVersion(int _index);

signals:
    /**
     * @brief Пользователь нажал кнопку создания нового проекта
     */
    void createNewItemPressed();

    /**
     * @brief Пользователь хочет отобразить версию с заданным индексом
     * @note 0 - текущая версия, последующие индексы идут увеличнными на единицу
     */
    void showVersionPressed(int _versionIndex);

    /**
     * @brief Пользователь вызвал контекстное меню версии
     */
    void showVersionContextMenuPressed(int _versionIndex);

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

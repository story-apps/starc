#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Панель писательского спринта
 */
class WritingSprintPanel : public Widget
{
    Q_OBJECT

public:
    explicit WritingSprintPanel(QWidget* _parent = nullptr);
    ~WritingSprintPanel() override;

    /**
     * @brief Показать панель работы со спринтом
     */
    void showPanel();

    /**
     * @brief Показать поздравительную открытку
     */
    void setResult(int _words);

signals:
    /**
     * @brief Пользователь запустил спринт
     */
    void sprintStarted();

    /**
     * @brief Спринт завершён
     */
    void sprintFinished();

    /**
     * @brief Пользователь прервал спринт
     */
    void sprintStopped();

protected:
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    void paintEvent(QPaintEvent* _event) override;

    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    void mousePressEvent(QMouseEvent* _event) override;

    void updateTranslations() override;

    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Представление конструктора обложек
 */
class CORE_LIBRARY_EXPORT CoverGeneratorView : public Widget
{
    Q_OBJECT

public:
    explicit CoverGeneratorView(QWidget* _parent = nullptr);
    ~CoverGeneratorView() override;

protected:
    /**
     * @brief Переопределяем, чтобы отлавливать события изменения размера области постера
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

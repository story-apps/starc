#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Сайдбар с инструментами ИИ помощника
 */
class CORE_LIBRARY_EXPORT TextGenerationView : public Widget
{
    Q_OBJECT

public:
    explicit TextGenerationView(QWidget* _parent = nullptr);
    ~TextGenerationView() override;

    /**
     * @brief Задать возможность редактирования
     */
    bool isReadOnly() const;
    void setReadOnly(bool _readOnly);

protected:
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

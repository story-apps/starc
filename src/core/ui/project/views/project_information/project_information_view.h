#pragma once

#include "../view_plugin_global.h"

#include <ui/widgets/widget/widget.h>

#include <interfaces/ui/i_document_plugin.h>


namespace Ui
{

class VIEW_PLUGIN_EXPORT ProjectInformationView : public Widget
{
    Q_OBJECT

public:
    explicit ProjectInformationView(QWidget* _parent = nullptr);
    ~ProjectInformationView() override;

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

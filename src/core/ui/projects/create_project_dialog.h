#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui
{

/**
 * @brief Диалог создания нового проекта
 */
class CreateProjectDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateProjectDialog(QWidget* _parent = nullptr);
    ~CreateProjectDialog() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

}

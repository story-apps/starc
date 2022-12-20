#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
enum class DocumentObjectType;
}


namespace Ui {

/**
 * @brief Виджет с информацией о новом документе
 */
class CreateDocumentDialogOption : public Widget
{
    Q_OBJECT

public:
    explicit CreateDocumentDialogOption(const Domain::DocumentObjectType& _documentType,
                                        QWidget* _parent = nullptr);
    ~CreateDocumentDialogOption() override;

    const Domain::DocumentObjectType& documentType() const;

    bool isChecked() const;
    void setChecked(bool _checked);

    QSize sizeHint() const override;

signals:
    void checkedChanged(bool _checked);

protected:
    void paintEvent(QPaintEvent* _event) override;

    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

    void updateTranslations() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

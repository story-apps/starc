#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет прогресбар
 */
class CORE_LIBRARY_EXPORT ProgressBar : public Widget
{
    Q_OBJECT

public:
    explicit ProgressBar(QWidget* _parent);
    ~ProgressBar() override;

    qreal progress() const;
    void setProgress(qreal _progress);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

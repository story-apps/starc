#pragma once

#include "floating_tool_bar.h"

#include <functional>

class FloatingToolbarAnimator;

namespace Ui {

/**
 * @brief Панель временного просмотра сравнения документов
 */
class CORE_LIBRARY_EXPORT ComparisonToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ComparisonToolbar(FloatingToolBar* _toolbar, FloatingToolbarAnimator* _animator,
                               const QString& _title, const std::function<void()>& _close);

    void setSavingEnabled(bool _enabled);

signals:
    void saveRequested();

protected:
    bool eventFilter(QObject* _watched, QEvent* _event) override;
    void updateTranslations() override;
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    void updateGeometryAndTitle();

    FloatingToolBar* m_toolbar = nullptr;
    QAction* m_back = nullptr;
    QAction* m_title = nullptr;
    QAction* m_save = nullptr;
    QString m_fullTitle;
};

} // namespace Ui

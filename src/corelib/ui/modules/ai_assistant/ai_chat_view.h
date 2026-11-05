#pragma once

#include <ui/widgets/widget/widget.h>
#include <QScopedPointer>

namespace Ui {

class AiChatView : public Widget
{
    Q_OBJECT
public:
    explicit AiChatView(QWidget* _parent = nullptr);
    ~AiChatView() override;
    void setAvailableWords(int _words);
    void setInsertionAvailable(bool _available);

signals:
    void toolsRequested();
    void insertRequested(const QString& _text);

protected:
    bool eventFilter(QObject* _watched, QEvent* _event) override;
    void updateTranslations() override;
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    void refresh();
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

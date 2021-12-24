#pragma once

#include <ui/widgets/text_field/text_field.h>


/**
 * @brief Редактор горячих клавиш
 */
class CORE_LIBRARY_EXPORT KeySequenceEdit : public TextField
{
    Q_OBJECT

public:
    explicit KeySequenceEdit(QWidget* _parent = nullptr);
    ~KeySequenceEdit() override;

    QKeySequence keySequence() const;
    void setKeySequence(const QKeySequence& keySequence);

    void clear();

signals:
    void editingFinished();
    void keySequenceChanged(const QKeySequence& keySequence);

protected:
    bool event(QEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void timerEvent(QTimerEvent* e) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

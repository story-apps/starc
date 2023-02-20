#include <ui/design_system/design_system.h>
#include <ui/widgets/text_edit/completer/completer.h>
#include <ui/widgets/text_field/text_field.h>

#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QStringListModel>

class CompletableTextField : public TextField
{
public:
    explicit CompletableTextField(QWidget* _parent = nullptr)
        : TextField(_parent)
    {
    }

protected:
    bool event(QEvent* _event) override
    {
        bool needComplete = false;
        switch (_event->type()) {
        case QEvent::FocusIn: {
            needComplete = true;
            break;
        }
        default: {
            break;
        }
        }
        if (needComplete) {
            complete(new QStringListModel({ "One", "Two", "Three" }), text());
        }

        return TextField::event(_event);
    }
};

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    TextField w;
    w.setBackgroundColor(Qt::white);
    w.setTextColor(Qt::black);
    w.setLabel("Label");
    w.resize(400, 110);
    w.setCustomMargins({ 20, 20, 20, 20 });
    w.setCompleterActive(true);
    w.completer()->setModel(new QStringListModel({ "One", "Two", "Three" }));
    w.show();

    //    QObject::connect(&w, &CompletableTextField::textChanged, &w, [&w] {
    //        w.complete(new QStringListModel({ "One", "Two", "Three" }), w.text());
    //    });

    return a.exec();
}

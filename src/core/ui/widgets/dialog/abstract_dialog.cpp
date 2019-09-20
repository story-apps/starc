#include "abstract_dialog.h"

#include "dialog_content.h"

#include <ui/widgets/label/label.h>

#include <QApplication>
#include <QGridLayout>
#include <QVariantAnimation>


class AbstractDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    DialogContent* content = nullptr;
    H6Label* title = nullptr;
    QGridLayout* contentsLayout = nullptr;
};

AbstractDialog::Implementation::Implementation(QWidget* _parent)
    : content(new DialogContent(_parent)),
      title(new H6Label(_parent)),
      contentsLayout(new QGridLayout)
{
    content->setBackgroundColor(Qt::white);
    content->setTextColor(Qt::black);

    title->setBackgroundColor(Qt::white);
    title->setTextColor(Qt::black);

    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(title);
    layout->addLayout(contentsLayout);

}


// ****


AbstractDialog::AbstractDialog(QWidget* _parent)
    : QWidget(_parent),
      d(new Implementation(this))
{
    if (_parent == nullptr) {
        setParent(QApplication::topLevelWidgets().first());
    }

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);
    layout->addWidget(d->content, 1, 1);
    layout->setRowStretch(2, 1);
    layout->setColumnStretch(2, 1);
}

void AbstractDialog::showDialog()
{
    if (parentWidget() == nullptr) {
        return;
    }

//    if (_buttons.isEmpty()) {
//        return;
//    }

    //
    // Конфигурируем геометрию диалога
    //
    move(0, 0);
    resize(parentWidget()->size());


    //
    // Запускаем отображение
    //
//    d->animateShow(size());
    setFocus();
    show();
}

void AbstractDialog::setTitle(const QString& _title)
{
    d->title->setText(_title);
}

QGridLayout* AbstractDialog::contentsLayout() const
{
    return d->contentsLayout;
}

AbstractDialog::~AbstractDialog() = default;

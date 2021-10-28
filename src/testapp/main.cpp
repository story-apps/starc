#include <ui/design_system/design_system.h>
#include <ui/widgets/image_cropper/image_cropper.h>

#include <QApplication>
#include <QDebug>

#include <NetworkRequest.h>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    ImageCropper w;
    w.setBackgroundColor(Qt::white);
    w.setTextColor(Qt::red);
    w.setProportionFixed(true);
    w.setProportion({ 4, 3 });
    w.show();

    return a.exec();
}

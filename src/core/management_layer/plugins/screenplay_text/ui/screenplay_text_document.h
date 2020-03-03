#pragma once

#include <QTextDocument>


namespace BusinessLayer {
    class ScreenplayTextModel;
}

namespace Ui
{

class ScreenplayTextDocument : public QTextDocument
{
    Q_OBJECT

public:
    explicit ScreenplayTextDocument(QObject* _parent = nullptr);
    ~ScreenplayTextDocument() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model);

signals:

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

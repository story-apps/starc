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

    /**
     * @brief Получить позицию элемента в заданном индексе
     */
    int itemPosition(const QModelIndex& _index);

private:
    /**
     * @brief Обновить содержимое модели, при изменение текста документа
     */
    void updateModelOnContentChange(int _position, int _charsRemoved, int _charsAdded);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

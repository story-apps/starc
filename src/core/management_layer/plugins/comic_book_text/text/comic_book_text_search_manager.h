#pragma once

#include <QObject>

class Widget;

namespace Ui {
class ComicBookTextEdit;
}

namespace BusinessLayer {

class ComicBookTextSearchManager : public QObject
{
    Q_OBJECT

public:
    ComicBookTextSearchManager(QWidget* _parent, Ui::ComicBookTextEdit* _textEdit);
    ~ComicBookTextSearchManager() override;

    /**
     * @brief Получить панель поиска
     */
    Widget* toolbar() const;

signals:
    /**
     * @brief Запрос на скрытие панели поиска
     */
    void hideToolbarRequested();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

#pragma once

#include <QObject>

class Widget;

namespace Ui {
class NovelOutlineEdit;
}

namespace BusinessLayer {

class NovelOutlineSearchManager : public QObject
{
    Q_OBJECT

public:
    NovelOutlineSearchManager(QWidget* _parent, Ui::NovelOutlineEdit* _textEdit);
    ~NovelOutlineSearchManager() override;

    /**
     * @brief Получить панель поиска
     */
    Widget* toolbar() const;

    /**
     * @brief Настроить режим редактирования
     */
    void setReadOnly(bool _readOnly);

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

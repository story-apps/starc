#pragma once

#include <QObject>

class Widget;

namespace Ui {
class NovelTextEdit;
}

namespace BusinessLayer {

class NovelTextSearchManager : public QObject
{
    Q_OBJECT

public:
    NovelTextSearchManager(QWidget* _parent, Ui::NovelTextEdit* _textEdit);
    ~NovelTextSearchManager() override;

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

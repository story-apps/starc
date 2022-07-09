#pragma once

#include <QObject>

class Widget;

namespace Ui {
class AudioplayTextEdit;
}

namespace BusinessLayer {

class AudioplayTextSearchManager : public QObject
{
    Q_OBJECT

public:
    AudioplayTextSearchManager(QWidget* _parent, Ui::AudioplayTextEdit* _textEdit);
    ~AudioplayTextSearchManager() override;

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

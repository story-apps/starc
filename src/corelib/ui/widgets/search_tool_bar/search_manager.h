#pragma once

#include <ui/widgets/widget/widget.h>
#include "../text_edit/base/base_text_edit.h"

#include <QObject>

class Widget;

namespace BusinessLayer {

class CORE_LIBRARY_EXPORT SearchManager : public QObject
{
    Q_OBJECT

public:
    explicit SearchManager(QWidget* _parent, BaseTextEdit* _textEdit);
    ~SearchManager() override;

    /**
     * @brief Получить панель поиска
     */
    virtual Widget* toolbar() const;

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

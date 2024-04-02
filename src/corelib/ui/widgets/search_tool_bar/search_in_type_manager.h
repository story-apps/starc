#pragma once

#include <ui/widgets/search_tool_bar/search_manager.h>

namespace BusinessLayer {

class CORE_LIBRARY_EXPORT SearchInTypeManager : public BusinessLayer::SearchManager
{
    Q_OBJECT

public:
    explicit SearchInTypeManager(QWidget* _parent, BaseTextEdit* _textEdit);
    ~SearchInTypeManager() override;

    /**
     * @brief Получить панель поиска
     */
    Widget* toolbar() const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

#pragma once

#include <QObject>

namespace BusinessLayer {
enum class TextParagraphType;
}


namespace Ui
{
class TextEdit;

/**
 * @brief Управляющий горячими клавишами редактора текста
 */
class TextEditShortcutsManager : public QObject
{
    Q_OBJECT

public:
    TextEditShortcutsManager() = delete;
    explicit TextEditShortcutsManager(TextEdit* _parent = nullptr);
    ~TextEditShortcutsManager() override;

    /**
     * @brief Задать контекст для горячих клавиш
     */
    void setShortcutsContext(QWidget* _context);

    /**
     * @brief Считать значения горячих клавиш из настроек
     */
    void reconfigure();

    /**
     * @brief Получить шорткат для блока
     */
    QString shortcut(BusinessLayer::TextParagraphType _forBlockType) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

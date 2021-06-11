#pragma once

#include <QObject>

namespace BusinessLayer {
enum class TextParagraphType;
}


namespace Ui {
class SimpleTextEdit;

/**
 * @brief Управляющий горячими клавишами редактора текста
 */
class SimpleTextEditShortcutsManager : public QObject
{
    Q_OBJECT

public:
    SimpleTextEditShortcutsManager() = delete;
    explicit SimpleTextEditShortcutsManager(SimpleTextEdit* _parent = nullptr);
    ~SimpleTextEditShortcutsManager() override;

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

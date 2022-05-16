#pragma once

#include <QObject>

namespace BusinessLayer {
enum class TextParagraphType;
}


namespace Ui {
class StageplayTextEdit;

/**
 * @brief Управляющий горячими клавишами редактора сценария
 */
class StageplayTextEditShortcutsManager : public QObject
{
    Q_OBJECT

public:
    StageplayTextEditShortcutsManager() = delete;
    explicit StageplayTextEditShortcutsManager(StageplayTextEdit* _parent = nullptr);
    ~StageplayTextEditShortcutsManager() override;

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

#pragma once

#include <QObject>

namespace BusinessLayer {
enum class ScreenplayParagraphType;
}


namespace Ui {
class ScreenplayTextEdit;

/**
 * @brief Управляющий горячими клавишами редактора сценария
 */
class ScreenplayTextEditShortcutsManager : public QObject
{
    Q_OBJECT

public:
    ScreenplayTextEditShortcutsManager() = delete;
    explicit ScreenplayTextEditShortcutsManager(ScreenplayTextEdit* _parent = nullptr);
    ~ScreenplayTextEditShortcutsManager() override;

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
    QString shortcut(BusinessLayer::ScreenplayParagraphType _forBlockType) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

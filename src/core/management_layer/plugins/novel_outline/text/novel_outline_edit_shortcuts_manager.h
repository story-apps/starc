#pragma once

#include <QObject>

namespace BusinessLayer {
enum class TextParagraphType;
}


namespace Ui {
class NovelOutlineEdit;

/**
 * @brief Управляющий горячими клавишами редактора сценария
 */
class NovelOutlineEditShortcutsManager : public QObject
{
    Q_OBJECT

public:
    NovelOutlineEditShortcutsManager() = delete;
    explicit NovelOutlineEditShortcutsManager(NovelOutlineEdit* _parent = nullptr);
    ~NovelOutlineEditShortcutsManager() override;

    /**
     * @brief Задать контекст для горячих клавиш
     */
    void setShortcutsContext(QWidget* _context);

    /**
     * @brief Считать значения горячих клавиш из настроек
     */
    void reconfigure();

    /**
     * @brief Включить/выключить работу шорткатов
     */
    void setEnabled(bool _enabled);

    /**
     * @brief Получить шорткат для блока
     */
    QString shortcut(BusinessLayer::TextParagraphType _forBlockType) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

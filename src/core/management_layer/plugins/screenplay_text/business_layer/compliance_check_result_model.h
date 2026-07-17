#pragma once

#include <QSortFilterProxyModel>


namespace BusinessLayer {

/**
 * @brief Перечисление типов элементов модели результатов проверки правил
 */
enum class ComplianceCheckResultModelItemType {
    //
    // Само правило
    //
    Rule,
    //
    // Вложенный элемент правила (персонаж / локация)
    Item,
    //
    // КОнкретная сцена
    //
    Scene,
};

/**
 * @brief Роли данных в модели
 */
enum ComplianceCheckResultModelItemDataRole {
    TypeRole = Qt::UserRole + 1,
    RuleStatusRole,
    TitleRole,
    SubtitleRole,
    SceneUuidRole,
    SceneNumberRole,
    SceneHeadingRole,
    SceneDurationRole,
    SceneEighthsRole,
};

} // namespace BusinessLayer

#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Доступные типы продвигаемых модулей
 */
enum class ModulePromoType {
    Undefined,
    CharactersRelations,
    CharacterInformation,
    CharacterDialogues,
    LocationsMap,
    LocationInformation,
    LocationScenes,
    WorldsMap,
    WoldInformation,
    ExtendedStatistics,
    IndexCards,
    StoryTimeline,
    ScreenplayBreakdown,
    ImagesGallery,
    MindMaps,
    Presentation,
    SeriesPlan,
    SeriesStatistics,
    CloudSync,
};

/**
 * @brief Виджет продвигающий платную опцию
 */
class CORE_LIBRARY_EXPORT ModulePromoWidget : public Widget
{
    Q_OBJECT

public:
    explicit ModulePromoWidget(QWidget* _parent = nullptr);
    ~ModulePromoWidget() override;

    /**
     * @brief Настроить виджет в соответствии с заданной подпиской и типом модуля
     */
    /** @{ */
    void setupProVersionContent(ModulePromoType _forType);
    void setupCloudVersionContent(ModulePromoType _forType);
    /** @} */

signals:
    /**
     * @brief Пользователь нажал кнопку закрытия виджета
     */
    void closePressed();

    /**
     * @brief Пользователь нажал кнопку оформления подписки
     */
    void purchasePressed();

protected:
    /**
     * @brief Переопределяем, чтобы корректировать минимальную высоту виджета с превью модуля
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

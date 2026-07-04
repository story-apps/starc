#pragma once

#include <QObject>
#include <QUuid>

#include <chrono>
#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Типы правил
 */
enum class CORE_LIBRARY_EXPORT ComplianceRuleType {
    Undefined,
    //
    // RCode 2026-05-22
    //
    TotalDuration, //!< Хронометраж серии
    ScenesCount, //!< Количество сцен в серии
    SceneDuration, //!< Хронометраж сцены
    CharacterShouldSpeakInEveryScene, //!< Реплики персонажа
    SceneCharactersCount, //!< Количество персонажей в сцене
    PrimaryLocationsPercent, //!< Процент сцен в основных локациях
    SecondaryLocationsCount, //!< Количество дополнительных локаций
    SecondaryLocationsSceneCount, //!< Количество сцен в дополнительных локациях
    SecondaryLocationsNightSceneCount, //!< Количество ночных сцен в дополнительных локациях
    //
};

/**
 * @brief Правило для описания конкретного требования к сценарию
 */
struct CORE_LIBRARY_EXPORT ComplianceRule {
    /**
     * @brief Тип правила
     */
    ComplianceRuleType type = ComplianceRuleType::Undefined;

    /**
     * @brief Минимальный порог
     */
    int minimumValue = 0;

    /**
     * @brief Максимальный порог
     */
    int maximumValue = 0;

    /**
     * @brief Текстовое значение для правила
     */
    QString textValue;
};

/**
 * @brief Тип элемента результата проверки
 */
enum class CORE_LIBRARY_EXPORT ComplianceCheckResultItemType {
    Undefined,
    Scene,
    Character,
    Location,
};

/**
 * @brief Конкретный персонаж в конкретной сцене из результатов проверки
 */
struct CORE_LIBRARY_EXPORT ComplianceCheckResultItemSceneCharacter {
    bool isValid() const;

    QString name;
    bool isFirstAppearance = false;
    int totalDialogues = 0;
};

/**
 * @brief Конкретная сцена из результатов проверки
 */
struct CORE_LIBRARY_EXPORT ComplianceCheckResultItemScene {
    bool isValid() const;
    int durationInSeconds() const;
    ComplianceCheckResultItemSceneCharacter& character(const QString& _name);
    ComplianceCheckResultItemSceneCharacter character(const QString& _name) const;

    QUuid uuid;
    QString number;
    QString heading;
    std::chrono::milliseconds duration;
    QVector<ComplianceCheckResultItemSceneCharacter> characters;
};

/**
 * @brief Элемент результата проверки
 */
struct CORE_LIBRARY_EXPORT ComplianceCheckResultItem {
    ComplianceCheckResultItemType type = ComplianceCheckResultItemType::Undefined;
    QString title;
    QString subtitle;
    QVector<ComplianceCheckResultItemScene> scenes;
};

/**
 * @brief Тип результата проверки
 */
enum class CORE_LIBRARY_EXPORT ComplianceCheckResultStatus {
    Undefined,
    Passed,
    Warning,
    Failed,
};

/**
 * @brief Результат проверки правила
 */
struct CORE_LIBRARY_EXPORT ComplianceCheckResult {
    ComplianceCheckResultStatus status = ComplianceCheckResultStatus::Undefined;
    QString title;
    QString subtitle;
    QVector<ComplianceCheckResultItem> items;
};

/**
 * @brief Фасад для управления проверяющим требования
 */
class CORE_LIBRARY_EXPORT ComplianceChecker : public QObject
{
    Q_OBJECT

public:
    explicit ComplianceChecker(QObject* _parent = nullptr);
    ~ComplianceChecker() override;

    /**
     * @brief Задать данные сценария
     */
    void setScreenplay(const QByteArray& _information, const QByteArray& _screenplay);

    /**
     * @brief Задать список правил для проверки
     */
    void setRules(const QVector<ComplianceRule>& _rules);

    /**
     * @brief Запустить проверку
     */
    void startChecking();

signals:
    /**
     * @brief Проверка закончена с указанными результатами
     */
    void checkingFinished(const QVector<ComplianceCheckResult>& _results);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

#pragma once

#include "compliance_checker.h"


namespace BusinessLayer {

struct ComplianceRule;
struct ComplianceCheckResult;

/**
 * @brief Проверялка для правил соответствия требованиям к сценариям
 */
class ComplianceCheckerImpl : public QObject
{
    Q_OBJECT

public:
    explicit ComplianceCheckerImpl(QObject* _parent = nullptr);
    ~ComplianceCheckerImpl() override;

    /**
     * @brief Инициилизировать данные класса
     * @note Делаем это отдельным методом, чтобы инициилизация выполнилась уже в отдельном потоке
     */
    void init();

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

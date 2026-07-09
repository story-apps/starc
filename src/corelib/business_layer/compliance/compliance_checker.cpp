#include "compliance_checker.h"

#include "compliance_checker_impl.h"

#include <QThread>


namespace BusinessLayer {

bool ComplianceCheckResultItemSceneCharacter::isValid() const
{
    return !name.isEmpty();
}

// **

bool ComplianceCheckResultItemScene::isValid() const
{
    return !uuid.isNull();
}

int ComplianceCheckResultItemScene::durationInSeconds() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

ComplianceCheckResultItemSceneCharacter& ComplianceCheckResultItemScene::character(
    const QString& _name)
{
    for (auto& character : characters) {
        if (character.name == _name) {
            return character;
        }
    }

    characters.append({ _name });
    return characters.last();
}

ComplianceCheckResultItemSceneCharacter ComplianceCheckResultItemScene::character(
    const QString& _name) const
{
    for (const auto& character : characters) {
        if (character.name == _name) {
            return character;
        }
    }

    return {};
}


// ****


class ComplianceChecker::Implementation
{
public:
    Implementation(ComplianceChecker* _q);
    ~Implementation();

    /**
     * @brief Запустить проверяющего, если он не запущен
     */
    void startChecker();

    /**
     * @brief Остановить проверяющего
     */
    void stopChecker();


    ComplianceChecker* q = nullptr;

    //
    // Держим кэш данных для проверки, чтобы не запускать проверки лишний раз, если нет правил
    //
    QByteArray information;
    QByteArray screenplay;
    QVector<ComplianceRule> rules;

    BusinessLayer::ComplianceCheckerImpl* checker = nullptr;
    QThread* checkerThread = nullptr;
};

ComplianceChecker::Implementation::Implementation(ComplianceChecker* _q)
    : q(_q)
    , checkerThread(new QThread(q))
{
}

ComplianceChecker::Implementation::~Implementation()
{
    stopChecker();
}

void ComplianceChecker::Implementation::startChecker()
{
    if (checkerThread->isRunning()) {
        return;
    }

    if (rules.isEmpty()) {
        return;
    }

    checker = new BusinessLayer::ComplianceCheckerImpl;
    checker->moveToThread(checkerThread);
    connect(checkerThread, &QThread::started, checker, &ComplianceCheckerImpl::init);
    connect(checkerThread, &QThread::started, q, [this] {
        QMetaObject::invokeMethod(q, [this] {
            checker->setScreenplay(information, screenplay);
            checker->setRules(rules);
            checker->startChecking();
        });
    });
    connect(checkerThread, &QThread::finished, checker, [this] {
        delete checker;
        checker = nullptr;
    });
    checkerThread->start();

    connect(checker, &ComplianceCheckerImpl::checkingFinished, q,
            &ComplianceChecker::checkingFinished);
}

void ComplianceChecker::Implementation::stopChecker()
{
    if (checkerThread->isFinished()) {
        return;
    }

    checkerThread->quit();
    checkerThread->wait();
}


// ****


ComplianceChecker::ComplianceChecker(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

ComplianceChecker::~ComplianceChecker() = default;

void ComplianceChecker::setScreenplay(const QByteArray& _information, const QByteArray& _screenplay)
{
    d->information = _information;
    d->screenplay = _screenplay;

    if (d->checker == nullptr) {
        return;
    }

    QMetaObject::invokeMethod(this, [this, _information, _screenplay] {
        d->checker->setScreenplay(_information, _screenplay);
        d->checker->startChecking();
    });
}

void ComplianceChecker::setRules(const QVector<ComplianceRule>& _rules)
{
    d->rules = _rules;
    if (d->rules.isEmpty()) {
        d->stopChecker();
        return;
    }

    if (d->checker == nullptr) {
        d->startChecker();
        return;
    }

    QMetaObject::invokeMethod(this, [this, _rules] {
        d->checker->setRules(_rules);
        d->checker->startChecking();
    });
}

} // namespace BusinessLayer

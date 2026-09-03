#include "compliance_checker.h"

#include "compliance_checker_impl.h"

#include <QPointer>
#include <QThread>


namespace BusinessLayer {

bool operator==(const ComplianceRule& _lhs, const ComplianceRule& _rhs)
{
    return _lhs.type == _rhs.type && _lhs.name == _rhs.name && _lhs.isStrict == _rhs.isStrict
        && _lhs.minimumValue == _rhs.minimumValue && _lhs.maximumValue == _rhs.maximumValue
        && _lhs.textValues == _rhs.textValues;
}

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

    QPointer<BusinessLayer::ComplianceCheckerImpl> checker;
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
        emit q->screenplayChanged(information, screenplay);
        emit q->rulesChanged(rules);
    });

    connect(q, &ComplianceChecker::screenplayChanged, checker,
            &ComplianceCheckerImpl::setScreenplay);
    connect(q, &ComplianceChecker::rulesChanged, checker, &ComplianceCheckerImpl::setRules);
    connect(checker, &ComplianceCheckerImpl::checkingFinished, q,
            &ComplianceChecker::checkingFinished);

    connect(checkerThread, &QThread::finished, checker, &ComplianceChecker::deleteLater);

    checkerThread->start();
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
    qRegisterMetaType<QVector<ComplianceRule>>("QVector<ComplianceRule>");
    qRegisterMetaType<QVector<ComplianceCheckResult>>("QVector<ComplianceCheckResult>");
}

ComplianceChecker::~ComplianceChecker() = default;

void ComplianceChecker::setScreenplay(const QByteArray& _information, const QByteArray& _screenplay)
{
    //
    // Не делаем проверок на идентичность данных, т.к. они могут быть большими и это нам ни к чему,
    // пусть проще отдельный поток молотит эти данные, так что вообще не паримся о них
    //
    d->information = _information;
    d->screenplay = _screenplay;
    emit screenplayChanged(d->information, d->screenplay);
}

void ComplianceChecker::setRules(const QVector<ComplianceRule>& _rules)
{
    if (d->rules == _rules) {
        return;
    }

    d->rules = _rules;
    emit rulesChanged(d->rules);


    //
    // Если больше нет правил, то останавливаем чекер
    //
    if (d->rules.isEmpty()) {
        d->stopChecker();
        return;
    }

    //
    // Если правила есть и чекер не был запущен, то запускаем его
    //
    if (d->checker == nullptr) {
        d->startChecker();
        return;
    }
}

} // namespace BusinessLayer

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


    ComplianceChecker* q = nullptr;

    BusinessLayer::ComplianceCheckerImpl* checker = nullptr;
    QThread* checkerThread = nullptr;
};

ComplianceChecker::Implementation::Implementation(ComplianceChecker* _q)
    : q(_q)
    , checker(new BusinessLayer::ComplianceCheckerImpl)
    , checkerThread(new QThread(q))
{
    checker->moveToThread(checkerThread);
    connect(checkerThread, &QThread::started, checker, &ComplianceCheckerImpl::init);
    connect(checkerThread, &QThread::finished, checker, &QObject::deleteLater);
    checkerThread->start();
}

ComplianceChecker::Implementation::~Implementation()
{
    checkerThread->quit();
    checkerThread->wait();
}


// ****


ComplianceChecker::ComplianceChecker(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
    connect(d->checker, &ComplianceCheckerImpl::checkingFinished, this,
            &ComplianceChecker::checkingFinished);
}

ComplianceChecker::~ComplianceChecker() = default;

void ComplianceChecker::setScreenplay(const QByteArray& _information, const QByteArray& _screenplay)
{
    QMetaObject::invokeMethod(this, [this, _information, _screenplay] {
        d->checker->setScreenplay(_information, _screenplay);
    });
}

void ComplianceChecker::setRules(const QVector<ComplianceRule>& _rules)
{
    QMetaObject::invokeMethod(this, [this, _rules] { d->checker->setRules(_rules); });
}

void ComplianceChecker::startChecking()
{
    QMetaObject::invokeMethod(this, [this] { d->checker->startChecking(); });
}

} // namespace BusinessLayer

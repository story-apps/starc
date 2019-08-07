#include "onboarding_view_model.h"

namespace {
    const int kLastStepIndex = 3;
}


class OnboardingViewModel::Implementation
{
public:
    int stepIndex = 0;
};

OnboardingViewModel::OnboardingViewModel(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

OnboardingViewModel::~OnboardingViewModel() = default;

int OnboardingViewModel::stepIndex() const
{
    return d->stepIndex;
}

void OnboardingViewModel::setStepIndex(int _index)
{
    if (_index < 0
        || d->stepIndex == _index) {
        return;
    }

    d->stepIndex = _index;
    if (d->stepIndex < kLastStepIndex) {
        emit stepIndexChanged(d->stepIndex);
    } else {
        emit onboardingFinished();
    }
}

void OnboardingViewModel::finish()
{
    setStepIndex(kLastStepIndex);
}

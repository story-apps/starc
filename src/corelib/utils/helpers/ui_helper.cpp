#include "ui_helper.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/base/base_text_edit.h>
#include <utils/helpers/color_helper.h>

#include <QScrollArea>
#include <QToolTip>
#include <QVBoxLayout>


void UiHelper::initColorsFor(Button* _button, ButtonRole _role)
{
    auto backgroundColor = Ui::DesignSystem::color().onBackground();
    auto textColor = Ui::DesignSystem::color().onBackground();
    switch (_role) {
    case DialogDefault: {
        backgroundColor = ColorHelper::transparent(Ui::DesignSystem::color().onBackground(),
                                                   Ui::DesignSystem::inactiveTextOpacity());
        textColor = backgroundColor;
        break;
    }
    case DialogAccept: {
        backgroundColor = Ui::DesignSystem::color().accent();
        textColor = backgroundColor;
        break;
    }
    case DialogCritical: {
        backgroundColor = Ui::DesignSystem::color().error();
        textColor = backgroundColor;
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }

    _button->setBackgroundColor(backgroundColor);
    _button->setTextColor(textColor);
}

QHBoxLayout* UiHelper::makeHBoxLayout()
{
    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    return layout;
}

QVBoxLayout* UiHelper::makeVBoxLayout()
{
    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    return layout;
}

void UiHelper::initSpellingFor(SpellCheckTextEdit* _edit)
{
    initSpellingFor(QVector<SpellCheckTextEdit*>{ _edit });
}

void UiHelper::initSpellingFor(const QVector<SpellCheckTextEdit*>& _edits)
{
    const auto useSpellChecker
        = settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool();
    const auto spellingLanguage
        = settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString();

    for (auto edit : _edits) {
        edit->setUseSpellChecker(useSpellChecker);
        if (useSpellChecker) {
            edit->setSpellCheckLanguage(spellingLanguage);
        }
    }
}

void UiHelper::initOptionsFor(BaseTextEdit* _edit)
{
    initOptionsFor(QVector<BaseTextEdit*>{ _edit });
}

void UiHelper::initOptionsFor(const QVector<BaseTextEdit*>& _edits)
{
    const auto correctDoubleCapitals
        = settingsValue(DataStorageLayer::kApplicationCorrectDoubleCapitalsKey).toBool();
    const auto capitalizeSingleILetter
        = settingsValue(DataStorageLayer::kApplicationCapitalizeSingleILetterKey).toBool();
    const bool replaceThreeDots
        = settingsValue(DataStorageLayer::kApplicationReplaceThreeDotsWithEllipsisKey).toBool();
    const auto useSmartQuotes
        = settingsValue(DataStorageLayer::kApplicationSmartQuotesKey).toBool();
    const auto replaceTwoDashes
        = settingsValue(DataStorageLayer::kApplicationReplaceTwoDashesWithEmDashKey).toBool();
    const auto avoidMultipleSpaces
        = settingsValue(DataStorageLayer::kApplicationAvoidMultipleSpacesKey).toBool();

    for (auto edit : _edits) {
        edit->setCorrectDoubleCapitals(correctDoubleCapitals);
        edit->setCapitalizeSingleILetter(capitalizeSingleILetter);
        edit->setReplaceThreeDots(replaceThreeDots);
        edit->setUseSmartQuotes(useSmartQuotes);
        edit->setReplaceTwoDashes(replaceTwoDashes);
        edit->setAvoidMultipleSpaces(avoidMultipleSpaces);
    }
}

void UiHelper::setFocusPolicyRecursively(QWidget* _widget, Qt::FocusPolicy _policy)
{
    _widget->setFocusPolicy(_policy);
    const auto childWidgets = _widget->findChildren<QWidget*>();
    for (auto childWidget : childWidgets) {
        childWidget->setFocusPolicy(_policy);
    }
}

QScrollArea* UiHelper::createScrollArea(QWidget* _parent, bool _withGridLayout)
{
    auto content = new QScrollArea(_parent);
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    setupScrolling(content);

    auto contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    if (_withGridLayout) {
        auto layout = new QGridLayout(contentWidget);
        layout->setContentsMargins({});
        layout->setSpacing(0);
    } else {
        auto layout = new QVBoxLayout(contentWidget);
        layout->setContentsMargins({});
        layout->setSpacing(0);
    }

    return content;
}

QScrollArea* UiHelper::createScrollAreaWithGridLayout(QWidget* _parent)
{
    const bool withGridLayout = true;
    return createScrollArea(_parent, withGridLayout);
}

void UiHelper::setupScrolling(QAbstractScrollArea* _scrollArea, bool _addHorizontalScrollBar)
{
    _scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto connectScrollBars = [](ScrollBar* _target, QScrollBar* _source) {
        QObject::connect(_source, &QScrollBar::valueChanged, _target, &ScrollBar::setValue);
        QObject::connect(_source, &QScrollBar::rangeChanged, _target,
                         [_target](int _min, int _max) {
                             _target->setRange(_min, _max);
                             _target->setVisible(_min < _max);
                         });
        QObject::connect(_target, &ScrollBar::valueChanged, _source, &QScrollBar::setValue);
    };

    const bool stickToParent = true;
    auto verticalScrollBar = new ScrollBar(_scrollArea, stickToParent);
    verticalScrollBar->setOrientation(Qt::Vertical);
    verticalScrollBar->setRange(0, 0);
    connectScrollBars(verticalScrollBar, _scrollArea->verticalScrollBar());

    if (_addHorizontalScrollBar) {
        auto horizontalScrollBar = new ScrollBar(_scrollArea, stickToParent);
        horizontalScrollBar->setOrientation(Qt::Horizontal);
        horizontalScrollBar->setRange(0, 0);
        connectScrollBars(horizontalScrollBar, _scrollArea->horizontalScrollBar());
    }
}

void UiHelper::showToolTip(const QString& _text)
{
    QToolTip::showText(QCursor::pos(), _text, nullptr, {}, 1600);
}

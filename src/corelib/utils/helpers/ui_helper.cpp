#include "ui_helper.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_edit/spell_check/spell_check_text_edit.h>

#include <QScrollArea>
#include <QToolTip>
#include <QVBoxLayout>


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

void UiHelper::setFocusPolicyRecursively(QWidget* _widget, Qt::FocusPolicy _policy)
{
    _widget->setFocusPolicy(_policy);
    const auto childWidgets = _widget->findChildren<QWidget*>();
    for (auto childWidget : childWidgets) {
        childWidget->setFocusPolicy(_policy);
    }
}

QScrollArea* UiHelper::createScrollArea(QWidget* _parent)
{
    auto content = new QScrollArea(_parent);
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    auto contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    auto layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addStretch();

    return content;
}

void UiHelper::showToolTip(const QString& _text)
{
    QToolTip::showText(QCursor::pos(), _text, nullptr, {}, 1600);
}

#include "novel_export_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/text_field/text_field.h>

#include <QSettings>
#include <QVBoxLayout>


namespace Ui {

namespace {

const QString kFormatKey = "format";
const QString kIncludeOutlineKey = "include-outline";
const QString kIncludeFootersKey = "include-footer";
const QString kScenesToPrintKey = "scenes-to-print";

static const QVector<BusinessLayer::ExportFileFormat> kNovelExportFormats({
    BusinessLayer::ExportFileFormat::Pdf,
    BusinessLayer::ExportFileFormat::Docx,
    BusinessLayer::ExportFileFormat::Markdown,
});

} // namespace

class NovelExportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список сцен для печати
     */
    QVector<QString> scenesToPrint() const;

    CheckBox* includeOutline = nullptr;
    CheckBox* includeFooters = nullptr;
    TextField* ornamentalBreak = nullptr;

    BusinessLayer::NovelExportOptions options;
};

NovelExportDialog::Implementation::Implementation(QWidget* _parent)
    : includeOutline(new CheckBox(_parent))
    , includeFooters(new CheckBox(_parent))
    , ornamentalBreak(new TextField(_parent))
{
    ornamentalBreak->setSpellCheckPolicy(SpellCheckPolicy::Manual);
}

QVector<QString> NovelExportDialog::Implementation::scenesToPrint() const
{
    QVector<QString> scenesToPrint;
    const auto scenesRanges = ornamentalBreak->text().split(',', Qt::SkipEmptyParts);
    for (const auto& scenesRange : scenesRanges) {
        if (scenesRange.contains('-')) {
            const auto range = scenesRange.split('-', Qt::SkipEmptyParts);
            if (range.size() == 2) {
                int fromScene = range.constFirst().toInt();
                int toScene = range.constLast().toInt();
                if (fromScene > toScene) {
                    std::swap(fromScene, toScene);
                }
                for (int scene = fromScene; scene <= toScene; ++scene) {
                    scenesToPrint.append(QString::number(scene));
                }
            } else if (!range.isEmpty()) {
                scenesToPrint.append(range.constFirst());
            }
        } else {
            scenesToPrint.append(scenesRange);
        }
    }
    return scenesToPrint;
}


// ****


NovelExportDialog::NovelExportDialog(const QString& _uuidKey, QWidget* _parent)
    : ScriptExportDialog(kNovelExportFormats, _uuidKey, _parent)
    , d(new Implementation(this))
{
    leftLayout()->insertWidget(2, d->includeOutline);
    rightLayout()->insertWidget(2, d->includeFooters);
    rightLayout()->insertWidget(5, d->ornamentalBreak);

    connect(d->includeOutline, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (_checked) {
            setIncludeScript(false);
        }
    });
    //
    connect(d->includeOutline, &CheckBox::checkedChanged, this, &NovelExportDialog::updateDialog);

    QSettings settings;
    setCurrentFileFormat(settings.value(uniqueKey(kFormatKey), 0).toInt());
    d->includeOutline->setChecked(settings.value(uniqueKey(kIncludeOutlineKey), false).toBool());
    d->includeFooters->setChecked(settings.value(uniqueKey(kIncludeFootersKey), false).toBool());
    d->ornamentalBreak->setText(settings.value(uniqueKey(kScenesToPrintKey)).toString());
}

NovelExportDialog::~NovelExportDialog()
{
    QSettings settings;
    settings.setValue(uniqueKey(kFormatKey), currentFileFormatRow());
    settings.setValue(uniqueKey(kIncludeOutlineKey), d->includeOutline->isChecked());
    settings.setValue(uniqueKey(kIncludeFootersKey), d->includeFooters->isChecked());
    settings.setValue(uniqueKey(kScenesToPrintKey), d->ornamentalBreak->text());
}

BusinessLayer::NovelExportOptions& NovelExportDialog::exportOptions() const
{
    d->options = ScriptExportDialog::exportOptions();

    d->options.includeOutline
        = d->includeOutline->isVisibleTo(this) && d->includeOutline->isChecked();
    d->options.includeFolders = true;
    d->options.includeFooters
        = d->includeFooters->isVisibleTo(this) && d->includeFooters->isChecked();
    d->options.ornamentalBreak
        = d->ornamentalBreak->isVisibleTo(this) ? d->ornamentalBreak->text() : "";
    return d->options;
}

void NovelExportDialog::updateTranslations()
{
    ScriptExportDialog::updateTranslations();
    setTitle(tr("Export novel"));
    d->includeOutline->setText(tr("Outline"));
    d->includeFooters->setText(tr("Include parts & chapters footers"));
    d->ornamentalBreak->setLabel(tr("Scenes' ornamental break"));
    d->ornamentalBreak->setHelper(tr("Keep empty, if you want to print scene headings instead"));
}

void NovelExportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    ScriptExportDialog::designSystemChangeEvent(_event);

    for (auto textField : std::vector<TextField*>{
             d->ornamentalBreak,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto checkBox : {
             d->includeOutline,
             d->includeFooters,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }
}

void NovelExportDialog::updateParametersVisibility() const
{
    ScriptExportDialog::updateParametersVisibility();
    d->ornamentalBreak->setVisible(isRightLayoutVisible());
}

bool NovelExportDialog::isRightLayoutVisible() const
{
    return ScriptExportDialog::isRightLayoutVisible()
        || (d->includeOutline->isVisibleTo(this) && d->includeOutline->isChecked());
}

void NovelExportDialog::processIncludeTextChanged(bool _checked) const
{
    if (_checked) {
        d->includeOutline->setChecked(false);
    }
}

bool NovelExportDialog::isExportEnabled() const
{
    return ScriptExportDialog::isExportEnabled()
        || (d->includeOutline->isVisibleTo(this) && d->includeOutline->isChecked());
}

bool NovelExportDialog::shouldIncludeText() const
{
    return (d->includeOutline->isVisibleTo(this) && d->includeOutline->isChecked())
        || ScriptExportDialog::shouldIncludeText();
}

} // namespace Ui

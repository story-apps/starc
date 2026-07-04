#include "compliance_check_result_view.h"

#include "compliance_check_result_delegate.h"

#include <business_layer/compliance/compliance_checker.h>
#include <business_layer/compliance_check_result_model.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>
#include <utils/helpers/ui_helper.h>

#include <QStandardItemModel>


namespace Ui {

class ComplianceCheckResultView::Implementation
{
public:
    Implementation(ComplianceCheckResultView* _q);


    ComplianceCheckResultView* q = nullptr;

    Widget* emptyPage = nullptr;

    QStandardItemModel* checkingResultsModel;
    Tree* checkingResultsPage = nullptr;
};

ComplianceCheckResultView::Implementation::Implementation(ComplianceCheckResultView* _q)
    : q(_q)
    , emptyPage(new Widget(q))
    , checkingResultsModel(new QStandardItemModel(q))
    , checkingResultsPage(new Tree(q))
{
    checkingResultsPage->setModel(checkingResultsModel);
    checkingResultsPage->setItemDelegate(new ComplianceCheckResultDelegate(checkingResultsPage));
}


// ****


ComplianceCheckResultView::ComplianceCheckResultView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setCurrentWidget(d->emptyPage);
    addWidget(d->checkingResultsPage);
}

ComplianceCheckResultView::~ComplianceCheckResultView() = default;

void ComplianceCheckResultView::setCheckResults(
    const QVector<BusinessLayer::ComplianceCheckResult>& _results)
{
    using namespace BusinessLayer;

    d->checkingResultsModel->clear();

    for (const auto& result : _results) {
        auto resultItem = new QStandardItem;
        resultItem->setData(static_cast<int>(ComplianceCheckResultModelItemType::Rule),
                            ComplianceCheckResultModelItemDataRole::TypeRole);
        resultItem->setData(static_cast<int>(result.status),
                            ComplianceCheckResultModelItemDataRole::RuleStatusRole);
        const auto icon = [status = result.status] {
            switch (status) {
            case BusinessLayer::ComplianceCheckResultStatus::Passed: {
                return u8"\U000F05E0";
            }
            case BusinessLayer::ComplianceCheckResultStatus::Warning: {
                return u8"\U000F0026";
            }
            default: {
                return u8"\U000F0159";
            }
            }
        }();
        resultItem->setData(icon, Qt::DecorationRole);
        resultItem->setData(result.title, ComplianceCheckResultModelItemDataRole::TitleRole);
        resultItem->setData(result.subtitle, ComplianceCheckResultModelItemDataRole::SubtitleRole);
        resultItem->setEditable(false);
        resultItem->setSelectable(false);

        if (!result.items.isEmpty()) {
            //
            // Если у нас только один элемент со сценами, просто вложим сцены в основной элемент
            //
            if (result.items.size() == 1
                && result.items.constFirst().type
                    == BusinessLayer::ComplianceCheckResultItemType::Scene) {
                for (const auto& resultScene : result.items.constFirst().scenes) {
                    auto resultItemScene = new QStandardItem;
                    resultItemScene->setData(
                        static_cast<int>(ComplianceCheckResultModelItemType::Scene),
                        ComplianceCheckResultModelItemDataRole::TypeRole);
                    resultItemScene->setData(u8"\U000f021a", Qt::DecorationRole);
                    resultItemScene->setData(resultScene.uuid,
                                             ComplianceCheckResultModelItemDataRole::SceneUuidRole);
                    resultItemScene->setData(
                        resultScene.number,
                        ComplianceCheckResultModelItemDataRole::SceneNumberRole);
                    resultItemScene->setData(
                        resultScene.heading,
                        ComplianceCheckResultModelItemDataRole::SceneHeadingRole);
                    resultItemScene->setData(
                        resultScene.durationInSeconds(),
                        ComplianceCheckResultModelItemDataRole::SceneDurationRole);

                    resultItemScene->setEditable(false);
                    resultItem->appendRow(resultItemScene);
                }
            }
            //
            // В противном случае показываем сцены внутри каждого родителя
            //
            else {
            }
        }

        d->checkingResultsModel->appendRow(resultItem);
    }

    setCurrentWidget(d->checkingResultsPage);
}

void ComplianceCheckResultView::updateTranslations()
{
}

void ComplianceCheckResultView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->emptyPage->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->checkingResultsPage->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->checkingResultsPage->setTextColor(Ui::DesignSystem::color().onPrimary());
}

} // namespace Ui

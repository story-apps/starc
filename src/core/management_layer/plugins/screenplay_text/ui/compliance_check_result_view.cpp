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

    QStandardItemModel* checkResultsModel = nullptr;
    Tree* checkResultsPage = nullptr;
};

ComplianceCheckResultView::Implementation::Implementation(ComplianceCheckResultView* _q)
    : q(_q)
    , emptyPage(new Widget(q))
    , checkResultsModel(new QStandardItemModel(q))
    , checkResultsPage(new Tree(q))
{
    checkResultsPage->setModel(checkResultsModel);
    checkResultsPage->setItemDelegate(new ComplianceCheckResultDelegate(checkResultsPage));
}


// ****


ComplianceCheckResultView::ComplianceCheckResultView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setCurrentWidget(d->emptyPage);
    addWidget(d->checkResultsPage);

    connect(d->checkResultsPage, &Tree::clicked, this, [this](const QModelIndex& _index) {
        const auto uuid
            = _index.data(BusinessLayer::ComplianceCheckResultModelItemDataRole::SceneUuidRole)
                  .toUuid();
        if (!uuid.isNull()) {
            emit sceneSelected(uuid);
        }
    });
}

ComplianceCheckResultView::~ComplianceCheckResultView() = default;

void ComplianceCheckResultView::setCheckResults(
    const QVector<BusinessLayer::ComplianceCheckResult>& _results)
{
    using namespace BusinessLayer;

    d->checkResultsModel->clear();

    //
    // Наполняем модель данными
    //
    for (const auto& result : _results) {
        //
        // ... результат проверки правила
        //
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
        //
        // ... если есть возможность, то добавляем детали
        //
        if (!result.items.isEmpty()) {
            auto fillItemScenes = [](QStandardItem* _item,
                                     const ComplianceCheckResultItem& _checkResultItem) {
                for (const auto& resultScene : _checkResultItem.scenes) {
                    auto resultSceneItem = new QStandardItem;
                    resultSceneItem->setData(
                        static_cast<int>(ComplianceCheckResultModelItemType::Scene),
                        ComplianceCheckResultModelItemDataRole::TypeRole);
                    resultSceneItem->setData(u8"\U000f021a", Qt::DecorationRole);
                    resultSceneItem->setData(resultScene.uuid,
                                             ComplianceCheckResultModelItemDataRole::SceneUuidRole);
                    resultSceneItem->setData(
                        resultScene.number,
                        ComplianceCheckResultModelItemDataRole::SceneNumberRole);
                    resultSceneItem->setData(
                        resultScene.heading,
                        ComplianceCheckResultModelItemDataRole::SceneHeadingRole);
                    resultSceneItem->setData(
                        resultScene.durationInSeconds(),
                        ComplianceCheckResultModelItemDataRole::SceneDurationRole);
                    resultSceneItem->setEditable(false);
                    _item->appendRow(resultSceneItem);
                }
            };

            //
            // Если у нас только один элемент со сценами, просто вложим сцены в основной элемент
            //
            if (result.items.size() == 1
                && result.items.constFirst().type
                    == BusinessLayer::ComplianceCheckResultItemType::Scene) {
                fillItemScenes(resultItem, result.items.constFirst());
            }
            //
            // В противном случае показываем сцены внутри каждого элемента результата проверки
            //
            else {
                for (const auto& resultChild : result.items) {
                    auto resultChildItem = new QStandardItem;
                    resultChildItem->setData(
                        static_cast<int>(ComplianceCheckResultModelItemType::Item),
                        ComplianceCheckResultModelItemDataRole::TypeRole);
                    const auto icon = [type = resultChild.type] {
                        switch (type) {
                        case BusinessLayer::ComplianceCheckResultItemType::Character: {
                            return u8"\U000f0004";
                        }
                        case BusinessLayer::ComplianceCheckResultItemType::Location: {
                            return u8"\U000f02dc";
                        }
                        default: {
                            return u8"\U000f021a";
                        }
                        }
                    }();
                    resultChildItem->setData(icon, Qt::DecorationRole);
                    resultChildItem->setData(resultChild.title,
                                             ComplianceCheckResultModelItemDataRole::TitleRole);
                    resultChildItem->setData(resultChild.subtitle,
                                             ComplianceCheckResultModelItemDataRole::SubtitleRole);
                    resultChildItem->setEditable(false);
                    resultChildItem->setSelectable(false);

                    fillItemScenes(resultChildItem, resultChild);

                    resultItem->appendRow(resultChildItem);
                }
            }
        }

        d->checkResultsModel->appendRow(resultItem);
    }

    setCurrentWidget(d->checkResultsPage);
}

void ComplianceCheckResultView::updateTranslations()
{
}

void ComplianceCheckResultView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->emptyPage->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->checkResultsPage->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->checkResultsPage->setTextColor(Ui::DesignSystem::color().onPrimary());
}

} // namespace Ui

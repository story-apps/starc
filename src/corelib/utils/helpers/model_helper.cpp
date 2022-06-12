#include "model_helper.h"

#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/audioplay_title_page_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/comic_book_title_page_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/stageplay_title_page_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>

using namespace BusinessLayer;


void ModelHelper::initTitlePageModel(BusinessLayer::SimpleTextModel* _model)
{
    if (_model && _model->rowCount() == 1) {
        const auto item = _model->itemForIndex(_model->index(0, 0));
        if (item->type() == BusinessLayer::TextModelItemType::Text) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            if (textItem->text().isEmpty()) {
                resetTitlePageModel(_model);
            }
        }
    }
}

void ModelHelper::resetTitlePageModel(BusinessLayer::SimpleTextModel* _model)
{
    if (!_model) {
        return;
    }

    QString titlePage;
    if (auto model = qobject_cast<BusinessLayer::ScreenplayTitlePageModel*>(_model)) {
        titlePage = TemplatesFacade::screenplayTemplate(model->informationModel()->templateId())
                        .titlePage();
    } else if (auto model = qobject_cast<BusinessLayer::ComicBookTitlePageModel*>(_model)) {
        titlePage = TemplatesFacade::comicBookTemplate(model->informationModel()->templateId())
                        .titlePage();
    } else if (auto model = qobject_cast<BusinessLayer::AudioplayTitlePageModel*>(_model)) {
        titlePage = TemplatesFacade::audioplayTemplate(model->informationModel()->templateId())
                        .titlePage();
    } else if (auto model = qobject_cast<BusinessLayer::StageplayTitlePageModel*>(_model)) {
        titlePage = TemplatesFacade::stageplayTemplate(model->informationModel()->templateId())
                        .titlePage();
    }

    _model->setDocumentContent(titlePage.toUtf8());
}

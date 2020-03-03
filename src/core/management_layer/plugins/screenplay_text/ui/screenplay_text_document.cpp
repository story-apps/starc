#include "screenplay_text_document.h"

#include "screenplay_text_cursor.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <utils/helpers/text_helper.h>


namespace Ui
{

class ScreenplayTextDocument::Implementation
{
public:
    BusinessLayer::ScreenplayTextModel* model = nullptr;
};


// ****


ScreenplayTextDocument::ScreenplayTextDocument(QObject *_parent)
    : QTextDocument(_parent),
      d(new Implementation)
{
}

ScreenplayTextDocument::~ScreenplayTextDocument() = default;

void ScreenplayTextDocument::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    QSignalBlocker signalBlocker(this);

    d->model = _model;
    clear();

    if (d->model == nullptr) {
        return;
    }

    //
    // Начинаем операцию вставки
    //
    ScreenplayTextCursor cursor(this);
    cursor.beginEditBlock();

    //
    // Последовательно формируем текст документа
    //
    bool isFirstParagraph = true;
    std::function<void(const QModelIndex&)> readDocumentFromModel;
    readDocumentFromModel = [model = d->model, &cursor, &isFirstParagraph, &readDocumentFromModel] (const QModelIndex& _parent) {
        for (int itemRow = 0; itemRow < model->rowCount(_parent); ++itemRow) {
            const auto itemIndex = model->index(itemRow, 0, _parent);
            const auto item = model->itemForIndex(itemIndex);
            switch (item->type()) {
                case BusinessLayer::ScreenplayTextModelItemType::Scene: {
                    break;
                }

                case BusinessLayer::ScreenplayTextModelItemType::Text: {
                    const auto textItem = static_cast<BusinessLayer::ScreenplayTextModelTextItem*>(item);

                    //
                    // Если это не первый абзац, вставим блок для него
                    //
                    if (!isFirstParagraph) {
                        cursor.insertBlock();
                    }
                    //
                    // ... в противном же случае, новый блок нет необходимости вставлять
                    //
                    else {
                        isFirstParagraph = false;
                    }

                    //
                    // Установим стиль блока
                    //
                    const auto currentStyle
                            = BusinessLayer::ScreenplayTemplateFacade::getTemplate().blockStyle(
                                  textItem->paragraphType());
                    cursor.setBlockFormat(currentStyle.blockFormat());
                    cursor.setBlockCharFormat(currentStyle.charFormat());
                    cursor.setCharFormat(currentStyle.charFormat());

                    //
                    // Вставим текст абзаца
                    //
                    const auto textToInsert = TextHelper::fromHtmlEscaped(textItem->text());
                    cursor.insertText(textToInsert);

                    break;
                }

                default: {
                    Q_ASSERT(false);
                    break;
                }
            }

            //
            // Считываем информацию о детях
            //
            readDocumentFromModel(itemIndex);
        }
    };
    readDocumentFromModel({});

    //
    // Завершаем операцию
    //
    cursor.endEditBlock();
}

} // namespace Ui

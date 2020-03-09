#include "screenplay_text_block_data.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>

#include <QDebug>
namespace Ui
{

ScreenplayTextBlockData::ScreenplayTextBlockData(BusinessLayer::ScreenplayTextModelTextItem* _item)
    : QTextBlockUserData(),
      m_item(_item)
{
}

ScreenplayTextBlockData::ScreenplayTextBlockData(const ScreenplayTextBlockData* _other)
    : QTextBlockUserData(),
      m_item(_other->m_item)
{

}

void ScreenplayTextBlockData::print()
{
    qDebug() << (int)m_item->paragraphType() << this;
}

BusinessLayer::ScreenplayTextModelTextItem* ScreenplayTextBlockData::item() const
{
    return m_item;
}

} // namespace Ui

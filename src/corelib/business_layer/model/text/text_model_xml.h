#pragma once

#include <business_layer/model/abstract_model_xml.h>


namespace BusinessLayer {
namespace xml {

//
// Папка и сцена
//
const QLatin1String kUuidAttribute("uuid");
const QLatin1String kPlotsAttribute("plots");
const QLatin1String kOmitedAttribute("omited");
const QLatin1String kNumberTag("number");
const QLatin1String kNumberValueAttribute("value");
const QLatin1String kNumberIsCustomAttribute("custom");
const QLatin1String kNumberIsEatNumberAttribute("eat");
const QLatin1String kNumberIsLockedAttribute("fixed");
const QLatin1String kQtyAttribute("qty");
const QLatin1String kColorTag("color");
const QLatin1String kTitleTag("title");
const QLatin1String kStoryDayTag("story_day");
const QLatin1String kStartDateTimeTag("start_date_time");
const QLatin1String kDescriptionTag("description");
const QLatin1String kStampTag("stamp");
const QLatin1String kTagsTag("tags");
const QLatin1String kTagTag("tag");
const QLatin1String kPlannedDurationTag("planned_duration");
const QLatin1String kResourcesTag("resources");
const QLatin1String kResourceTag("resource");

//
// Разделитель
//
const QLatin1String kSplitterTag("splitter");
const QLatin1String kTypeAttribute("type");

//
// Текст
//
const QLatin1String kParametersTag("params");
const QLatin1String kBookmarkTag("bm");
const QLatin1String kValueTag("v");
const QLatin1String kReviewMarksTag("rms");
const QLatin1String kReviewMarkTag("rm");
const QLatin1String kCommentTag("c");
const QLatin1String kFormatsTag("fms");
const QLatin1String kFormatTag("fm");
const QLatin1String kResourceMarksTag("rss");
const QLatin1String kResourceMarkTag("rs");
const QLatin1String kFromAttribute("from");
const QLatin1String kLengthAttribute("length");
const QLatin1String kColorAttribute("color");
const QLatin1String kBackgroundColorAttribute("bgcolor");
const QLatin1String kDoneAttribute("done");
const QLatin1String kAuthorAttribute("author");
const QLatin1String kEmailAttribute("email");
const QLatin1String kDateAttribute("date");
const QLatin1String kIsCommentRevisionAttribute("r");
const QLatin1String kIsCommentEditedAttribute("e");
const QLatin1String kBoldAttribute("bold");
const QLatin1String kItalicAttribute("italic");
const QLatin1String kUnderlineAttribute("underline");
const QLatin1String kStrikethroughAttribute("strikethrough");
const QLatin1String kFontFamilyAttribute("fontf");
const QLatin1String kFontSizeAttribute("fonts");
const QLatin1String kAlignAttribute("align");
const QLatin1String kInFirstColumnAttribute("ifc");

} // namespace xml
} // namespace BusinessLayer

#pragma once

#include <business_layer/model/abstract_model_xml.h>

namespace BusinessLayer {
namespace xml {

//
// Глава
//
const QString kChapterTag = QLatin1String("chapter");
const QString kUuidAttribute = QLatin1String("uuid");
const QString kNumberTag = QLatin1String("number");
const QString kNumberValueAttribute = QLatin1String("value");
const QString kStampTag = QLatin1String("stamp");

//
// Текст
//
const QString kTextParametersTag = QLatin1String("params");
const QString kValueTag = QLatin1String("v");
const QString kReviewMarksTag = QLatin1String("rms");
const QString kReviewMarkTag = QLatin1String("rm");
const QString kCommentTag = QLatin1String("c");
const QString kFormatsTag = QLatin1String("fms");
const QString kFormatTag = QLatin1String("fm");
const QString kFromAttribute = QLatin1String("from");
const QString kLengthAttribute = QLatin1String("length");
const QString kColorAttribute = QLatin1String("color");
const QString kBackgroundColorAttribute = QLatin1String("bgcolor");
const QString kDoneAttribute = QLatin1String("done");
const QString kAuthorAttribute = QLatin1String("author");
const QString kDateAttribute = QLatin1String("date");
const QString kFontAttribute = QLatin1String("font");
const QString kFontSizeAttribute = QLatin1String("fontsize");
const QString kBoldAttribute = QLatin1String("bold");
const QString kItalicAttribute = QLatin1String("italic");
const QString kUnderlineAttribute = QLatin1String("underline");
const QString kStrikethroughAttribute = QLatin1String("strikethrough");
const QString kAlignAttribute = QLatin1String("align");

} // namespace xml
} // namespace BusinessLayer

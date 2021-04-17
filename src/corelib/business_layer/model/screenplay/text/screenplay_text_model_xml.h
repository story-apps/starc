#pragma once

#include <business_layer/model/abstract_model_xml.h>


namespace BusinessLayer
{
namespace xml
{

//
// Папка и сцена
//
const QString kFolderTag = QLatin1String("folder");
const QString kSceneTag = QLatin1String("scene");
const QString kUuidAttribute = QLatin1String("uuid");
const QString kPlotsAttribute = QLatin1String("plots");
const QString kOmitedAttribute = QLatin1String("omited");
const QString kNumberTag = QLatin1String("number");
const QString kNumberValueAttribute = QLatin1String("value");
const QString kNumberGroupAttribute = QLatin1String("group");
const QString kNumberGroupIndexAttribute = QLatin1String("group_index");
const QString kStampTag = QLatin1String("stamp");
const QString kPlannedDurationTag = QLatin1String("planned_duration");

//
// Разделитель
//
const QString kSplitterTag = QLatin1String("splitter");
const QString kTypeAttribute = QLatin1String("type");

//
// Текст
//
const QString kBookmarkTag = QLatin1String("bm");
const QString kValueTag = QLatin1String("v");
const QString kReviewMarksTag = QLatin1String("rms");
const QString kReviewMarkTag = QLatin1String("rm");
const QString kCommentTag = QLatin1String("c");
const QString kFormatsTag = QLatin1String("fms");
const QString kFormatTag = QLatin1String("fm");
const QString kRevisionsTag = QLatin1String("revs");
const QString kRevisionTag = QLatin1String("rev");
const QString kFromAttribute = QLatin1String("from");
const QString kLengthAttribute = QLatin1String("length");
const QString kColorAttribute = QLatin1String("color");
const QString kBackgroundColorAttribute = QLatin1String("bgcolor");
const QString kDoneAttribute = QLatin1String("done");
const QString kAuthorAttribute = QLatin1String("author");
const QString kDateAttribute = QLatin1String("date");
const QString kBoldAttribute = QLatin1String("bold");
const QString kItalicAttribute = QLatin1String("italic");
const QString kUnderlineAttribute = QLatin1String("underline");
const QString kAlignAttribute = QLatin1String("align");
const QString kInFirstColumn = QLatin1String("ifc");

} // namespace xml
} // namespace BusinessLayer

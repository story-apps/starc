#include "file_helper.h"

#include <QString>


QString FileHelper::systemSavebleFileName(const QString& _fileName)
{

    QString result = _fileName;
#ifdef Q_OS_WIN
    result = result.replace("\"", "_").replace(":", "_").replace("?", "_");
#endif
    return result;
}

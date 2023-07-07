#include "abstract_report.h"

#include <utils/helpers/extension_helper.h>

#include <QString>


namespace BusinessLayer {

void AbstractReport::saveToFile(const QString& _filename) const
{
    if (_filename.endsWith(ExtensionHelper::pdf())) {
        saveToPdf(_filename);
    } else {
        saveToXlsx(_filename);
    }
}

} // namespace BusinessLayer

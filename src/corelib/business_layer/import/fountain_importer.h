#pragma once

#include "abstract_importer.h"

#include <QScopedPointer>


namespace BusinessLayer
{

/**
 * @brief Импортер сценария из файлов fountain
 */
class CORE_LIBRARY_EXPORT FountainImporter : public AbstractImporter
{
/*
              . .
             ` ' `
         .'''. ' .'''.
           .. ' ' ..
          '  '.'.'  '
          .'''.'.'''
         ' .''.'.''. '
           . . : . .
      {} _'___':'___'_ {}
      ||(_____________)||
      """"""(     )""""""
            _)   (_             .^-^.  ~""~
           (_______)~~"""~~     '._.'
       ~""~                     .' '.
                                '.,.'
                                   `'`'
 */

public:
    FountainImporter();
    ~FountainImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions& _options) const override;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ImportOptions& _options) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

#pragma once

#include "screenplay_abstract_importer.h"

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов fountain
 */
class CORE_LIBRARY_EXPORT ScreenplayFountainImporter : public ScreenplayAbstractImporter
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
    ScreenplayFountainImporter();
    ~ScreenplayFountainImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ScreenplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарии
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарий из заданного текста
     */
    Screenplay importScreenplay(const QString& _screenplayText) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

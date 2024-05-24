#pragma once

#include "abstract_stageplay_importer.h"

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов fountain
 */
class CORE_LIBRARY_EXPORT StageplayFountainImporter : public AbstractStageplayImporter
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
    StageplayFountainImporter();
    ~StageplayFountainImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const StageplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарии
     */
    QVector<Stageplay> importStageplays(const StageplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарий из заданного текста
     */
    Stageplay importStageplay(const QString& _stageplayText) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

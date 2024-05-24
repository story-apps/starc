#pragma once

#include "abstract_audioplay_importer.h"

#include <corelib/business_layer/import/abstract_fountain_importer.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов fountain
 */
class CORE_LIBRARY_EXPORT AudioplayFountainImporter : public AbstractAudioplayImporter,
                                                      public AbstractFountainImporter
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
    AudioplayFountainImporter();
    ~AudioplayFountainImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const AudioplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарии
     */
    QVector<Audioplay> importAudioplays(const AudioplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарий из заданного текста
     */
    Audioplay importAudioplay(const QString& _audioplayText) const;

    /**
     * @brief Предобработка конкретного блока (определяется его тип)
     */
    virtual void preprocessBlock(QVector<QString>& paragraphs, QXmlStreamWriter& _writer) const override;
};

} // namespace BusinessLayer

#pragma once

#include "abstract_stageplay_importer.h"

#include <business_layer/import/abstract_fountain_importer.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер пьес из файлов fountain
 */
class CORE_LIBRARY_EXPORT StageplayFountainImporter : public AbstractStageplayImporter,
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
    StageplayFountainImporter();
    ~StageplayFountainImporter() override;

    /**
     * @brief Импортировать пьесу
     */
    Stageplay importStageplay(const ImportOptions& _options) const override;

    /**
     * @brief Получить основной текст пьесы в формате xml из заданного текста
     */
    Stageplay stageplayText(const QString& _stageplayText) const;

protected:
    /**
     * @brief Получить имя персонажа
     */
    QString characterName(const QString& _text) const override;

    /**
     * @brief Получить название локации
     */
    QString locationName(const QString& _text) const override;

    /**
     * @brief Записать данные блока
     */
    void writeBlock(const QString& _paragraphText, TextParagraphType _type,
                    QXmlStreamWriter& _writer) const override;

    /**
     * @brief Постобработка предыдущего блока после его закрытия
     */
    void postProcessBlock(TextParagraphType _type, QXmlStreamWriter& _writer) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

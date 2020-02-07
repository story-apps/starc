#pragma once

#include "../abstract_model.h"

/*

  описать структуру хранения данных
    <script>
        <scene uuid="{uuid}" plots="{uuid},{uuid}..." omited="">
            <number value="" group="" groupIndex=""/>
            <stamp>CDATA</stamp>
            <duration_planned>CDATA</duration_planned>
            <!-- Блоки -->
            <content>
                <-- Блок текста -->
                <block_type>
                    <!-- Закладка -->
                    <bm color="">CDATA</bm>
                    <!-- Текст блока -->
                    <v>CDATA</v>
                    <!-- Редакторские комментарии -->
                    <rms>
                        <!-- Само выделение -->
                        <rm from="" length="" color="" bgcolor="" highlight="" done="">
                            <!-- Комментарии к нему -->
                            <c author="" date="">CDATA</c>
                            ...
                        </rm>
                        ...
                    </rms>
                    <!-- Форматирование -->
                    <fms>
                        <fm from="" length="" bold="" italic="" undeline=""/>
                        ...
                    </fms>
                    <!-- Ревизии -->
                    <revs>
                        <rev from="" length="" color=""/>
                        ...
                    </revs>
                </block_type>
                ...
                <-- Разделение контента на две колонки -->
                <split_start/>
                <split/>
                <split_end/>
                ...
            </content>
        </scene>
        ...
    </script>

  сохранение в xml
  наложение патчей
  формирование дочерних моделей
    для навигатора
    для редактора текста
        для аутлайна
        для сценария
  обновление данных из дочерних моделей

 */


namespace BusinessLayer
{

/**
 * @brief Модель текста сценария
 */
class ScreenplayTextModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplayTextModel(QObject* _parent = nullptr);
    ~ScreenplayTextModel() override;

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

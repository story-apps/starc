#include "scene_heading_handler.h"

#include "../ScenarioTextEdit.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockParsers.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>

#include <Domain/Place.h>
#include <Domain/Research.h>
#include <Domain/ScenarioDay.h>
#include <Domain/SceneTime.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/PlaceStorage.h>
#include <DataLayer/DataStorageLayer/ResearchStorage.h>
#include <DataLayer/DataStorageLayer/ScenarioDayStorage.h>
#include <DataLayer/DataStorageLayer/TimeStorage.h>

#include <QKeyEvent>
#include <QTextBlock>

using namespace Domain;
using namespace DataStorageLayer;
using namespace KeyProcessingLayer;
using namespace BusinessLogic;
using UserInterface::ScenarioTextEdit;

namespace {
    /**
     * @brief Более продвинутый метод определения текущей секции блока
     */
    SceneHeadingParser::Section section(const QString _blockText) {
        SceneHeadingParser::Section section = SceneHeadingParser::section(_blockText);
        if (section == SceneHeadingParser::SectionTime) {
            //
            // Возможно пользователь предпочитает обозначать локации и подлокации через минус,
            // поэтому проверяем нет ли уже сохранённых локаций такого рода, и если есть, и они
            // подходят под дополнение, то считаем текущую секцию за локацию
            //
            const bool FORCE = true;
            const QString locationFromBlock = SceneHeadingParser::locationName(_blockText, FORCE);
            foreach (DomainObject* object, StorageFacade::researchStorage()->locations()->toList()) {
                if (Research* location = dynamic_cast<Research*>(object)) {
                    if (location->name().startsWith(locationFromBlock, Qt::CaseInsensitive)) {
                        section = SceneHeadingParser::SectionLocation;
                        break;
                    }
                }
            }
        }
        return section;
    }
}


SceneHeadingHandler::SceneHeadingHandler(Ui::ScreenplayTextEdit* _editor) :
    StandardKeyHandler(_editor)
{
#ifdef MOBILE_OS
    //
    // Для мобильной версии автоматом вставляем пробел при выборе места действия из выпадающего списка
    //
    QObject::connect(_editor, &ScenarioTextEdit::completed, [this] {
        const auto block = editor()->textCursor().block();
        if (ScenarioBlockStyle::forBlock(block) == ScenarioBlockStyle::SceneHeading
            && ::section(block.text()) == SceneHeadingParser::SectionPlace) {
            editor()->insertPlainText(" ");
            handleOther();
        }
    });
#endif
}

void SceneHeadingHandler::handleEnter(QKeyEvent* _event)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());
    // ... текущая секция
    SceneHeadingParser::Section currentSection = ::section(cursorBackwardText);


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Вставить выбранный вариант
        //
        editor()->applyCompletion();

        //
        // Обновим курсор, т.к. после автозавершения он смещается
        //
        cursor = editor()->textCursor();

        //
        // Дописать необходимые символы
        //
        switch (currentSection) {
            case SceneHeadingParser::SectionPlace: {
                cursor.insertText(" ");
                break;
            }

            default: {
                break;
            }
        }

        //
        // Покажем подсказку, если это возможно
        //
        handleOther();

        //
        // Если нужно автоматически перепрыгиваем к следующему блоку
        //
        if (_event != nullptr // ... чтобы таб не переводил на новую строку
            && autoJumpToNextBlock()
            && currentSection == SceneHeadingParser::SectionTime) {
            //
            // Сохраним параметры сцены
            //
            storeSceneParameters();
            //
            // Переходим к следующему блоку
            //
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::SceneHeading));
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addScenarioBlock(ScenarioBlockStyle::SceneHeading);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем в соответствии с настройками
                //
                editor()->changeScenarioBlockType(changeForEnter(ScenarioBlockStyle::SceneHeading));
            } else {
                //! Текст не пуст

                //
                // Сохраним параметры сцены
                //
                storeSceneParameters();

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставка блока заголовка перед собой
                    //
                    editor()->addScenarioBlock(ScenarioBlockStyle::SceneHeading);

                    //
                    // Перенесём параметры из блока в котором они остались к текущему блоку
                    //
                    QTextCursor cursor = editor()->textCursor();
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*> (cursor.block().userData())) {
                        SceneHeadingBlockInfo* movedInfo = info->clone();
                        cursor.block().setUserData(nullptr);
                        cursor.movePosition(QTextCursor::NextBlock);
                        cursor.block().setUserData(movedInfo);
                    }
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // В режиме аутлайна вставка описания действия
                    //
                    if (editor()->outlineMode()) {
                        editor()->addScenarioBlock(ScenarioBlockStyle::SceneDescription);
                    }
                    //
                    // В противном случае, вставка блока описания действия
                    //
                    else {
                        editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::SceneHeading));
                    }
                } else {
                    //! Внутри блока

                    //
                    // Вставка блока описания действия
                    //
                    editor()->addScenarioBlock(ScenarioBlockStyle::Action);
                }
            }
        }
    }
}

void SceneHeadingHandler::handleTab(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Работаем аналогично нажатию ENTER
        //
        handleEnter();
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Ни чего не делаем
            //
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Если строка пуста, то сменить стиль на описание действия
                //
                editor()->changeScenarioBlockType(changeForTab(ScenarioBlockStyle::SceneHeading));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Если в секции локации, то добавление " - " и отображение подсказки
                    //
                    if (::section(cursorBackwardText) == SceneHeadingParser::SectionLocation) {
                        //
                        // Добавим необходимый текст в зависимости от того, что ввёл пользователь
                        //
                        if (cursorBackwardText.endsWith(" -")) {
                            cursor.insertText(" ");
                        } else if (cursorBackwardText.endsWith(" ")) {
                            cursor.insertText("- ");
                        } else {
                            cursor.insertText(" - ");
                        }

                        //
                        // Отображение подсказки
                        //
                        handleOther();
                    }

                    //
                    // В противном случае перейдём к блоку участников сцены
                    //
                    else {
                        //
                        // Сохраним параметры сцены
                        //
                        storeSceneParameters();

                        //
                        // А затем вставим блок
                        //
                        editor()->addScenarioBlock(jumpForTab(ScenarioBlockStyle::SceneHeading));
                    }
                } else {
                    //! Внутри блока

                    //
                    // Ни чего не делаем
                    //
                }
            }
        }
    }
}

void SceneHeadingHandler::handleOther(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void SceneHeadingHandler::handleInput(QInputMethodEvent* _event)
{
#ifndef Q_OS_ANDROID
    Q_UNUSED(_event)
#endif
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    const QTextCursor cursor = editor()->textCursor();
    int cursorPosition = cursor.positionInBlock();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
#ifdef Q_OS_ANDROID
    QString stringForInsert;
    if (!_event->preeditString().isEmpty()) {
        stringForInsert = _event->preeditString();
    } else {
        stringForInsert = _event->commitString();
    }
    currentBlockText.insert(cursorPosition, stringForInsert);
    cursorPosition += stringForInsert.length();
#endif
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursorPosition);

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void SceneHeadingHandler::complete(const QString& _currentBlockText, const QString& _cursorBackwardText)
{
    //
    // Текущая секция
    //
    SceneHeadingParser::Section currentSection = ::section(_cursorBackwardText);

    //
    // Получим модель подсказок для текущей секции и выведем пользователю
    //
    QAbstractItemModel* sectionModel = nullptr;
    //
    // ... в соответствии со введённым в секции текстом
    //
    QString sectionText;

    switch (currentSection) {
        case SceneHeadingParser::SectionPlace: {
            sectionModel = StorageFacade::placeStorage()->all();
            sectionText = SceneHeadingParser::placeName(_currentBlockText);
            break;
        }

        case SceneHeadingParser::SectionLocation: {
            sectionModel = StorageFacade::researchStorage()->locations();
            bool force = SceneHeadingParser::section(_cursorBackwardText) == SceneHeadingParser::SectionTime;
            sectionText = SceneHeadingParser::locationName(_currentBlockText, force);
            break;
        }

        case SceneHeadingParser::SectionScenarioDay: {
            sectionModel = StorageFacade::scenarioDayStorage()->all();
            sectionText = SceneHeadingParser::scenarioDayName(_currentBlockText);
            break;
        }

        case SceneHeadingParser::SectionTime: {
            //
            // Возможно пользователь предпочитает обозначать локации и подлокации через минус,
            // поэтому проверяем нет ли уже сохранённых локаций такого рода, и если есть, и они
            // подходят под дополнение, то используем их
            //
            bool useLocations = false;
            const bool FORCE = true;
            const QString locationFromBlock = SceneHeadingParser::locationName(_currentBlockText, FORCE);
            foreach (DomainObject* object, StorageFacade::researchStorage()->locations()->toList()) {
                if (Research* location = dynamic_cast<Research*>(object)) {
                    if (location->name().startsWith(locationFromBlock, Qt::CaseInsensitive)) {
                        useLocations = true;
                        break;
                    }
                }
            }
            if (useLocations) {
                sectionModel = StorageFacade::researchStorage()->locations();
                sectionText = locationFromBlock;
            }
            //
            // Во всех остальных случаях используем дополнение по времени действия
            //
            else {
                sectionModel = StorageFacade::timeStorage()->all();
                sectionText = SceneHeadingParser::timeName(_currentBlockText);
            }
            break;
        }

        default: {
            break;
        }
    }

    //
    // Дополним текст
    //
    editor()->complete(sectionModel, sectionText);
}

void SceneHeadingHandler::storeSceneParameters() const
{
    if (editor()->storeDataWhenEditing()) {
        //
        // Получим необходимые значения
        //
        // ... курсор в текущем положении
        const QTextCursor cursor = editor()->textCursor();
        // ... блок текста в котором находится курсор
        const QTextBlock currentBlock = cursor.block();
        // ... текст блока
        const QString currentBlockText = currentBlock.text();
        // ... текст до курсора
        const QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());

        //
        // Сохраняем время
        //
        const QString placeName = SceneHeadingParser::placeName(cursorBackwardText);
        StorageFacade::placeStorage()->storePlace(placeName);

        //
        // Сохраняем локацию
        //
        const QString locationName = SceneHeadingParser::locationName(cursorBackwardText);
        StorageFacade::researchStorage()->storeLocation(locationName);

        //
        // Сохраняем место
        //
        const QString timeName = SceneHeadingParser::timeName(cursorBackwardText);
        StorageFacade::timeStorage()->storeTime(timeName);

        //
        // Сохраняем сценарный день
        //
        const QString scenarioDayName = SceneHeadingParser::scenarioDayName(cursorBackwardText);
        StorageFacade::scenarioDayStorage()->storeScenarioDay(scenarioDayName);
    }
}

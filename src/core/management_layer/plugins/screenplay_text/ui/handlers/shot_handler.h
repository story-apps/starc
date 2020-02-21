#ifndef NOTEHANDLER_H
#define NOTEHANDLER_H

#include "standard_key_handler.h"


namespace KeyProcessingLayer
{
	/**
	 * @brief Класс выполняющий обработку нажатия клавиш в блоке примечание
	 */
	class NoteHandler : public StandardKeyHandler
	{
	public:
		NoteHandler(UserInterface::ScenarioTextEdit* _editor);

	protected:
		/**
		 * @brief Реализация интерфейса AbstractKeyHandler
		 */
		/** @{ */
		void handleEnter(QKeyEvent* _event = 0);
		void handleTab(QKeyEvent* _event = 0);
		/** @} */
	};
}

#endif // NOTEHANDLER_H

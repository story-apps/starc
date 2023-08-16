/*
 *  Copyright © 2018-2023 Hennadii Chernyshchyk <genaloner@gmail.com>
 *
 *  This file is part of QOnlineTranslator.
 *
 *  QOnlineTranslator is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  QOnlineTranslator is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with QOnlineTranslator. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef QEXAMPLE_H
#define QEXAMPLE_H

#include <QJsonObject>

/**
 * @brief Provides storage for example usage examples for a single type of speech
 *
 * Can be obtained from the QOnlineTranslator object, which contains translation example.
 *
 * Example:
 * @code
 * QOnlineTranslator translator;
 * // Obtain translation
 * 
 * QTextStream out(stdout);
 * for (auto it = translator.examples().cbegin(); it != translator.examples().cend(); ++it) {
 *     out << it.key() << ":" << endl; // Output the type of speech with a colon
 *     for (const auto &[example, description] : it.value()) {
 *         out << "  " << description << endl;
 *         out << "  " << example << endl;
 *         out << endl;
 *     }
 *     out << endl;
 * }
 * @endcode
 *
 * Possible output:
 * @code
 * // noun:
 * //   an opportunity for stating one's opinion or feelings.
 * //   the voters are entitled to have their say on the treaty
 * 
 * // verb:
 * //   utter words so as to convey information, an opinion, a feeling or intention, or an instruction.
 * //   "Thank you," he said
 * 
 * // exclamation:
 * //   used to express surprise or to draw attention to a remark or question.
 * //   say, did you notice any blood?
 * @endcode
 */
struct QExample {
    /**
     * @brief Example sentense
     */
    QString example;

    /**
     * @brief description for the example
     */
    QString description;

    /**
     * @brief Converts the object to JSON
     *
     * @return JSON representation
     */
    QJsonObject toJson() const;
};

#endif // QEXAMPLE_H

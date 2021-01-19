#include <QApplication>
#include <QDebug>

#include <ui/design_system/design_system.h>

#include <ui/widgets/chat/chat_message.h>
#include <ui/widgets/chat/chat_messages_view.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString str1 = "abcdddcba";
    QString str2 = "xyzddddxyz";

    enum class OperationType {
        Skip,
        Insert,
        Remove,
        Replace
    };
    struct Operation {
        OperationType type;
        QChar value;
    };
    auto applyOpearations = [] (const QString& source, QVector<Operation> operations) {
        QString result = source;
        int textIndex = 0;
        for (auto operation : operations) {
            switch (operation.type) {
                case OperationType::Skip: {
                    ++textIndex;
                    break;
                }

                case OperationType::Insert: {
                    result.insert(textIndex, operation.value);
                    ++textIndex;
                    break;
                }

                case OperationType::Remove: {
                    result.remove(textIndex, 1);
                    break;
                }

                case OperationType::Replace: {
                    result[textIndex] = operation.value;
                    ++textIndex;
                    break;
                }
            }
        }
        return result;
    };




    std::function<int(int,int,int)> min;
    min = [] (int x, int y, int z) { return std::min(std::min(x, y), z); };
    std::function<int(const QString&,const QString&,int,int)> editDist;
    editDist = [&editDist, &min](const QString& str1, const QString& str2, int m, int n) {
        // If first string is empty, the only option is to
        // insert all characters of second string into first
        if (m == 0)
            return n;

        // If second string is empty, the only option is to
        // remove all characters of first string
        if (n == 0)
            return m;

        // If last characters of two strings are same, nothing
        // much to do. Ignore last characters and get count for
        // remaining strings.
        if (str1[m - 1] == str2[n - 1])
            return editDist(str1, str2, m - 1, n - 1);

        // If last characters are not same, consider all three
        // operations on last character of first string,
        // recursively compute minimum cost for all three
        // operations and take minimum of three values.
        return 1
                + min(editDist(str1, str2, m, n - 1), // Insert
                      editDist(str1, str2, m - 1, n), // Remove
                      editDist(str1, str2, m - 1, n - 1) // Replace
                      );
    };

    qDebug() << editDist(str1, str2, str1.length(), str2.length());

    auto minimumDistance = [] (const QVector<Operation>& _x, const QVector<Operation>& _y, const QVector<Operation>& _z) {
        auto operationWeight = [] (const QVector<Operation>& _operations) {
            int weight = 0;
            for (const auto& operation : _operations) {
                weight += operation.type == OperationType::Skip ? 0 : 1;
            }
            return weight;
        };

        const auto xWeight = operationWeight(_x);
        const auto yWeight = operationWeight(_y);
        const auto zWeight = operationWeight(_z);
        if (xWeight < yWeight && xWeight < zWeight) {
            return _x;
        } else if (yWeight < xWeight && yWeight < zWeight) {
            return _y;
        } else {
            return _z;
        }
    };
    std::function<QVector<Operation>(const QString&,int,const QString&,int,const QVector<Operation>&)> editDistanceImpl;
    editDistanceImpl = [&editDistanceImpl, &minimumDistance] (const QString& source, int sourceIndex, const QString& target,
                            int targetIndex, const QVector<Operation>& _operations)
                            -> QVector<Operation>
    {
        auto operations = _operations;

        // If first string is empty, the only option is to
        // insert all characters of second string into first
        if (sourceIndex == 0) {
            for (int index = targetIndex - 1; index >= 0; --index) {
                operations.prepend({OperationType::Insert, target.at(index)});
            }
            return operations;
        }

        // If second string is empty, the only option is to
        // remove all characters of first string
        if (targetIndex == 0) {
            for (int index = sourceIndex - 1; index >= 0; --index) {
                operations.prepend({OperationType::Remove, {}});
            }
            return operations;
        }

        // If last characters of two strings are same, nothing
        // much to do. Ignore last characters and get count for
        // remaining strings.
        if (source.at(sourceIndex - 1) == target.at(targetIndex - 1)) {
            operations.prepend({OperationType::Skip, {}});
            return editDistanceImpl(source, sourceIndex - 1, target, targetIndex - 1, operations);
        }

        // If last characters are not same, consider all three
        // operations on last character of first string,
        // recursively compute minimum cost for all three
        // operations and take minimum of three values.
        auto operationsWithInsert = operations;
        operationsWithInsert.prepend({OperationType::Insert, target.at(targetIndex - 1)});
        operationsWithInsert = editDistanceImpl(source, sourceIndex, target, targetIndex - 1, operationsWithInsert);
        //
        auto operationsWithRemove = operations;
        operationsWithRemove.prepend({OperationType::Remove, {}});
        operationsWithRemove = editDistanceImpl(source, sourceIndex - 1, target, targetIndex, operationsWithRemove);
        //
        auto operationsWithReplace = operations;
        operationsWithReplace.prepend({OperationType::Replace, target.at(targetIndex - 1)});
        operationsWithReplace = editDistanceImpl(source, sourceIndex - 1, target, targetIndex - 1, operationsWithReplace);
        //
        return minimumDistance(operationsWithInsert,
                               operationsWithRemove,
                               operationsWithReplace);
    };
    auto editDistance = [&editDistanceImpl] (const QString& source, const QString& target) {
        return editDistanceImpl(source, source.length(), target, target.length(), {});
    };

    auto operations = editDistance(str1, str2);
    qDebug() << applyOpearations(str1, operations);


    return 1;



//    ChatMessagesView w;
//    w.show();

//    QVector<ChatMessage> messages
//            = {{ QDateTime(QDate(2020, 12, 13), QTime(14, 28, 11)),
//                 "Пока не понял. Где он вводит имайл?",
//                 User("Mikhail Galkin") },
//               { QDateTime(QDate(2020, 12, 13), QTime(14, 33, 11)),
//                 "на сайте или в приложении",
//                 User("Dimka Novikov") },
//               { QDateTime(QDate(2020, 12, 13), QTime(20, 4, 11)),
//                 "а с какой базой соединяться? с базой сайта или с апихой?",
//                 User("Mikhail Galkin") },
//               { QDateTime(QDate(2020, 12, 13), QTime(20, 7, 11)),
//                 "Так это будет суть одно",
//                 User("Dimka Novikov") },
//               { QDateTime(QDate(2020, 12, 14), QTime(11, 35, 11)),
//                 "Привет, бандит! Хотел напомнить оценить мне заказик один)))",
//                 User("Dimka Novikov") },
//               { QDateTime(QDate(2020, 12, 14), QTime(11, 39, 11)),
//                 "Привет. Ок. В выходные что-то отдыхалось так хорошо. Так что ничего не делал. Сегодня прикину оценку и по апихе потрещим.",
//                 User("Mikhail Galkin") },
//               { QDateTime(QDate(2020, 12, 14), QTime(11, 45, 11)),
//                 "У тебя как викенд?",
//                 User("Mikhail Galkin") },
//               { QDateTime(QDate(2020, 12, 14), QTime(12, 2, 11)),
//                 "Спасибо)",
//                 User("Dimka Novikov") },
//               { QDateTime(QDate(2020, 12, 14), QTime(12, 3, 11)),
//                 "??? за что?",
//                 User("Mikhail Galkin") },
//               { QDateTime(QDate(2020, 12, 14), QTime(12, 3, 11)),
//                 "Да я всё как-то делами домашними занимался, да с карапузом тусил)",
//                 User("Dimka Novikov") },
//               { QDateTime(QDate(2020, 12, 14), QTime(12, 3, 11)),
//                 "что прикинешь сегодня)",
//                 User("Dimka Novikov") },
//               { QDateTime(QDate(2020, 12, 14), QTime(12, 8, 11)),
//                 "на дачке нибось отдыхали?",
//                 User("Dimka Novikov") },
//               { QDateTime(QDate(2020, 12, 14), QTime(12, 13, 11)),
//                 "Не. На дачу после 20-ого поедем, на нг. в субботу в гости гоняли, в восресенье я по магазм ездил. Потом дома чилил. Хорошо зашли выходные. Даже не хватило.",
//                 User("Mikhail Galkin") }};
//    w.setMessages(messages);
//    w.setCurrectUser(User("Dimka Novikov"));

    return a.exec();
}

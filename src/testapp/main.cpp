#include <QApplication>

#include <ui/design_system/design_system.h>

#include <ui/widgets/chat/message.h>
#include <ui/widgets/chat/messages_view.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MessagesView w;
    w.show();

    QVector<Message> messages
            = {{ QDateTime(QDate(2020, 12, 13), QTime(14, 28, 11)),
                 "Пока не понял. Где он вводит имайл?",
                 User("Mikhail Galkin") },
               { QDateTime(QDate(2020, 12, 13), QTime(14, 33, 11)),
                 "на сайте или в приложении",
                 User("Dimka Novikov") },
               { QDateTime(QDate(2020, 12, 13), QTime(20, 4, 11)),
                 "а с какой базой соединяться? с базой сайта или с апихой?",
                 User("Mikhail Galkin") },
               { QDateTime(QDate(2020, 12, 13), QTime(20, 7, 11)),
                 "Так это будет суть одно",
                 User("Dimka Novikov") },
               { QDateTime(QDate(2020, 12, 14), QTime(11, 35, 11)),
                 "Привет, бандит! Хотел напомнить оценить мне заказик один)))",
                 User("Dimka Novikov") },
               { QDateTime(QDate(2020, 12, 14), QTime(11, 39, 11)),
                 "Привет. Ок. В выходные что-то отдыхалось так хорошо. Так что ничего не делал. Сегодня прикину оценку и по апихе потрещим.",
                 User("Mikhail Galkin") },
               { QDateTime(QDate(2020, 12, 14), QTime(11, 45, 11)),
                 "У тебя как викенд?",
                 User("Mikhail Galkin") },
               { QDateTime(QDate(2020, 12, 14), QTime(12, 2, 11)),
                 "Спасибо)",
                 User("Dimka Novikov") },
               { QDateTime(QDate(2020, 12, 14), QTime(12, 3, 11)),
                 "??? за что?",
                 User("Mikhail Galkin") },
               { QDateTime(QDate(2020, 12, 14), QTime(12, 3, 11)),
                 "Да я всё как-то делами домашними занимался, да с карапузом тусил)",
                 User("Dimka Novikov") },
               { QDateTime(QDate(2020, 12, 14), QTime(12, 3, 11)),
                 "что прикинешь сегодня)",
                 User("Dimka Novikov") },
               { QDateTime(QDate(2020, 12, 14), QTime(12, 8, 11)),
                 "на дачке нибось отдыхали?",
                 User("Dimka Novikov") },
               { QDateTime(QDate(2020, 12, 14), QTime(12, 13, 11)),
                 "Не. На дачу после 20-ого поедем, на нг. в субботу в гости гоняли, в восресенье я по магазм ездил. Потом дома чилил. Хорошо зашли выходные. Даже не хватило.",
                 User("Mikhail Galkin") }};
    w.setMessages(messages);
    w.setCurrectUser(User("Dimka Novikov"));

    return a.exec();
}

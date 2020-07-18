#pragma once

#include <QObject>


/**
 * @brief Модель комментариев к тексту сценария
 */
class ScreenplayTextCommentsModel : public QObject
{
    Q_OBJECT

public:
    explicit ScreenplayTextCommentsModel(QObject *parent = nullptr);

signals:

};

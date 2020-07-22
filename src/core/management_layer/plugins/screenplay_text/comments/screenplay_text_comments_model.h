#pragma once

#include <QAbstractListModel>



namespace BusinessLayer {

class ScreenplayTextModel;


/**
 * @brief Модель комментариев к тексту сценария
 */
class ScreenplayTextCommentsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        ReviewMarkAuthorEmail = Qt::UserRole + 1,
        ReviewMarkCreationDate,
        ReviewMarkComment,
        ReviewMarkIsDone
    };

public:
    explicit ScreenplayTextCommentsModel(QObject* _parent = nullptr);
    ~ScreenplayTextCommentsModel() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model);

    /**
     * @brief addReviewMark
     */
    void addReviewMark();

    /**
     * @brief Реализация модели списка
     */
    int rowCount(const QModelIndex &_parent = {}) const override;
    QVariant data(const QModelIndex &_index, int _role) const override;

signals:

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

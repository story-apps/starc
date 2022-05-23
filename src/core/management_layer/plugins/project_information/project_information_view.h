#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/widgets/image/image_card.h>


namespace Ui {

class ProjectInformationView : public Widget, public IDocumentView
{
    Q_OBJECT

public:
    explicit ProjectInformationView(QWidget* _parent = nullptr);
    ~ProjectInformationView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    QWidget* asQWidget() override;

    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);

    void setLogline(const QString& _logline);
    Q_SIGNAL void loglineChanged(const QString& _logline);

    void setCover(const QPixmap& _cover);
    Q_SIGNAL void coverChanged(const QPixmap& _cover);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

/**
 * @brief Класс карточки постера, открывающий доступ к генератору постеров
 */
class CoverImageCard : public ImageCard
{
    Q_OBJECT

public:
    explicit CoverImageCard(QWidget* _parent = nullptr);
    ~CoverImageCard() override;

signals:
    /**
     * @brief Пользователь хочет сгенерировать постер
     */
    void generateCoverPressed();

protected:
    /**
     * @brief Получить список действий контекстного меню
     */
    QVector<QAction*> contextMenuActions() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

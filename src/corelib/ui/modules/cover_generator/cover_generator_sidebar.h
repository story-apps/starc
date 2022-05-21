#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

struct CoverTextParameters {
    QString text;
    QFont font;
    QColor color;
    Qt::Alignment align;
};

class CORE_LIBRARY_EXPORT CoverGeneratorSidebar : public Widget
{
    Q_OBJECT

public:
    explicit CoverGeneratorSidebar(QWidget* _parent = nullptr);
    ~CoverGeneratorSidebar() override;

    /**
     * @brief Параметры постера
     */
    /** @{ **/
    CoverTextParameters top1Text() const;
    CoverTextParameters top2Text() const;
    CoverTextParameters beforeNameText() const;
    CoverTextParameters nameText() const;
    CoverTextParameters afterNameText() const;
    CoverTextParameters creditsText() const;
    CoverTextParameters releaseDateText() const;
    CoverTextParameters websiteText() const;
    //
    QPixmap backgroundImage() const;
    /** @} **/

signals:
    /**
     * @brief Изменились параметры постера
     */
    void coverParametersChanged();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

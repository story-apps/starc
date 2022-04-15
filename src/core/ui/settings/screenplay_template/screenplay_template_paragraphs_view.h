#pragma once

#include <ui/widgets/widget/widget.h>

namespace BusinessLayer {
enum class TextParagraphType;
}

namespace Domain {
enum class DocumentObjectType;
}


namespace Ui {

/**
 * @brief Представление параметров блоков шаблона
 */
class ScreenplayTemplateParagraphsView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTemplateParagraphsView(QWidget* _parent = nullptr);
    ~ScreenplayTemplateParagraphsView() override;

    /**
     * @brief Использовать миллиметры (true) ли дюймы (false) для отображения параметров
     */
    void setUseMm(bool _mm);

    /**
     * @brief  Сконфигурировать параметры редактора для шаблона заданного типа документа
     */
    void configureTemplateFor(Domain::DocumentObjectType _type);

    /**
     * @brief Текущий выбранный тип параграфа
     */
    BusinessLayer::TextParagraphType currentParagraphType() const;

    /**
     * @brief Выбрать первую вкладку списка типов
     */
    void selectFirstParagraphTypeTab();

    //
    // Параметры параграфа
    //
    bool isParagraphEnabled() const;
    void setParagraphEnabled(bool _enabled);
    QString fontFamily() const;
    void setFontFamily(const QString& _family);
    int fontSize() const;
    void setFontSize(int _size);
    bool isStartsFromNewPage() const;
    void setStartsFromNewPage(bool _starts);
    bool isUppercase() const;
    void setUppercase(bool _uppercase);
    bool isBold() const;
    void setBold(bool _bold);
    bool isItalic() const;
    void setItalic(bool _italic);
    bool isUnderline() const;
    void setUndeline(bool _underline);
    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment _alignment);
    qreal topIndent() const;
    void setTopIndent(qreal _indent);
    qreal bottomIndent() const;
    void setBottomIndent(qreal _indent);
    bool isVerticalIndentationInLines() const;
    void setVericalIndentationInLines(bool _inLines);
    qreal leftIndent() const;
    void setLeftIndent(qreal _indent);
    qreal rightIndent() const;
    void setRightIndent(qreal _indent);
    qreal leftIndentInTable() const;
    void setLeftIndentInTable(qreal _indent);
    qreal rightIndentInTable() const;
    void setRightIndentInTable(qreal _indent);
    int lineSpacingType() const;
    void setLineSpacingType(int _type);
    qreal lineSpacingValue() const;
    void setLineSpacingValue(qreal _value);
    bool showParagraphTitle() const;
    void setShowParagraphTitle(bool _show);
    QString customParagraphTitle() const;
    void setCustomParagraphTitle(const QString& _title);

signals:
    /**
     * @brief Пользователь хочет настроить параметры параграфа заданного типа
     */
    void currentParagraphTypeChanged(BusinessLayer::TextParagraphType _currentType,
                                     BusinessLayer::TextParagraphType _previousType);

    /**
     * @brief Изменились параметры текущего параграфа
     */
    void currentParagraphChanged();

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

} // namespace Ui

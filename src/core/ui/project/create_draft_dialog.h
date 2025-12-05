#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

namespace Domain {
enum class DocumentObjectType;
}


namespace Ui {

class CreateDraftDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateDraftDialog(QWidget* _parent = nullptr);
    ~CreateDraftDialog() override;

    /**
     * @brief Задать список драфтов из которых можно создать новый, а так же тип документа, чтобы
     *        понять, можно ли импортировать
     */
    void setDrafts(const QStringList& _drafts, int _selectDraftIndex,
                   Domain::DocumentObjectType _documentType);

    /**
     * @brief Папка, из которой будут выбираться проекты для импорта
     */
    QString importFilePath() const;
    void setImportFolder(const QString& _path);

    /**
     * @brief Редактировать драфт с заданными параметрами
     */
    void edit(const QString& _name, const QColor& _color, bool _readOnly, bool _comparison);

signals:
    /**
     * @brief Пользователь нажал кнопку создания нового драфта/сохранения редактируемого
     */
    void savePressed(const QString& _draftName, const QColor& _color, int _sourceDraftIndex,
                     bool _readOnly);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

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

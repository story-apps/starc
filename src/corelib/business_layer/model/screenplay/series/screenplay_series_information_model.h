#pragma once

#include "../../abstract_model.h"


namespace BusinessLayer {

struct ChronometerOptions;

/**
 * @brief Модель информации о сценарии сериала
 */
class CORE_LIBRARY_EXPORT ScreenplaySeriesInformationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplaySeriesInformationModel(QObject* _parent = nullptr);
    ~ScreenplaySeriesInformationModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;

    const QString& tagline() const;
    void setTagline(const QString& _tagline);
    Q_SIGNAL void taglineChanged(const QString& _logline);

    const QString& logline() const;
    void setLogline(const QString& _logline);
    Q_SIGNAL void loglineChanged(const QString& _logline);

    bool titlePageVisible() const;
    void setTitlePageVisible(bool _visible);
    Q_SIGNAL void titlePageVisibleChanged(bool _visible);

    bool synopsisVisible() const;
    void setSynopsisVisible(bool _visible);
    Q_SIGNAL void synopsisVisibleChanged(bool _visible);

    bool treatmentVisible() const;
    void setTreatmentVisible(bool _visible);
    Q_SIGNAL void treatmentVisibleChanged(bool _visible);

    bool screenplayTextVisible() const;
    void setScreenplayTextVisible(bool _visible);
    Q_SIGNAL void screenplayTextVisibleChanged(bool _visible);

    bool screenplayStatisticsVisible() const;
    void setScreenplayStatisticsVisible(bool _visible);
    Q_SIGNAL void screenplayStatisticsVisibleChanged(bool _visible);

    const QString& header() const;
    void setHeader(const QString& _header);
    Q_SIGNAL void headerChanged(const QString& _header);

    bool printHeaderOnTitlePage() const;
    void setPrintHeaderOnTitlePage(bool _print);
    Q_SIGNAL void printHeaderOnTitlePageChanged(bool _print);

    const QString& footer() const;
    void setFooter(const QString& _footer);
    Q_SIGNAL void footerChanged(const QString& _footer);

    bool printFooterOnTitlePage() const;
    void setPrintFooterOnTitlePage(bool _print);
    Q_SIGNAL void printFooterOnTitlePageChanged(bool _print);

    bool overrideCommonSettings() const;
    void setOverrideCommonSettings(bool _override);
    Q_SIGNAL void overrideCommonSettingsChanged(bool _override);

    QString templateId() const;
    void setTemplateId(const QString& _templateId);
    Q_SIGNAL void templateIdChanged(const QString& _templateId);

    bool showSceneNumbers() const;
    void setShowSceneNumbers(bool _show);
    Q_SIGNAL void showSceneNumbersChanged(bool _show);

    bool showSceneNumbersOnLeft() const;
    void setShowSceneNumbersOnLeft(bool _show);
    Q_SIGNAL void showSceneNumbersOnLeftChanged(bool _show);

    bool showSceneNumbersOnRight() const;
    void setShowSceneNumbersOnRight(bool _show);
    Q_SIGNAL void showSceneNumbersOnRightChanged(bool _show);

    bool showDialoguesNumbers() const;
    void setShowDialoguesNumbers(bool _show);
    Q_SIGNAL void showDialoguesNumbersChanged(bool _show);

    ChronometerOptions chronometerOptions() const;
    void setChronometerOptions(const ChronometerOptions& _options);
    Q_SIGNAL void chronometerOptionsChanged(const ChronometerOptions& _options);

    /**
     * @brief Список персонажей отсортированных в порядке важном для разбивки этого сценария
     */
    QVector<QString> charactersOrder() const;
    void setCharactersOrder(const QVector<QString>& _characters);
    Q_SIGNAL void charactersOrderChanged(const QVector<QString>& _characters);

    /**
     * @brief Список локаций отсортированных в порядке важном для разбивки этого сценария
     */
    QVector<QString> locationsOrder() const;
    void setLocationsOrder(const QVector<QString>& _locations);
    Q_SIGNAL void locationsOrderChanged(const QVector<QString>& _locations);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    ChangeCursor applyPatch(const QByteArray& _patch) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

/**
 * @brief Модель информации о сценарии
 */
class CORE_LIBRARY_EXPORT NovelInformationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit NovelInformationModel(QObject* _parent = nullptr);
    ~NovelInformationModel() override;

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

    bool outlineVisible() const;
    void setOutlineVisible(bool _visible);
    Q_SIGNAL void outlineVisibleChanged(bool _visible);

    bool novelTextVisible() const;
    void setNovelTextVisible(bool _visible);
    Q_SIGNAL void novelTextVisibleChanged(bool _visible);

    bool novelStatisticsVisible() const;
    void setNovelStatisticsVisible(bool _visible);
    Q_SIGNAL void novelStatisticsVisibleChanged(bool _visible);

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

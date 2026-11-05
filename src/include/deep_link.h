#pragma once

#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

#include <optional>

/** Browser and OS entry points share this strict, version-independent URL contract. */
struct DeepLink {
    int teamId = -1;
    int projectId = -1;
    QUuid documentGuid;

    // Also recognize malformed URLs here so they never reach the file importer.
    static bool isLink(const QString& value)
    {
        return value.startsWith(QStringLiteral("starc:"), Qt::CaseInsensitive)
            || value.startsWith(QStringLiteral("https:"), Qt::CaseInsensitive)
            || value.startsWith(QStringLiteral("http:"), Qt::CaseInsensitive);
    }

    static std::optional<DeepLink> parse(const QString& value)
    {
        if (value.size() > 4096) {
            return std::nullopt;
        }
        const QUrl url(value, QUrl::StrictMode);
        const bool native = url.scheme() == QLatin1String("starc")
            && url.host() == QLatin1String("open")
            && (url.path().isEmpty() || url.path() == QLatin1String("/"));
        const bool web = url.scheme() == QLatin1String("https")
            && url.host() == QLatin1String("starc.app")
            && (url.path() == QLatin1String("/open") || url.path() == QLatin1String("/open/"));
        if (!url.isValid() || (!native && !web) || !url.userInfo().isEmpty()
            || url.port() != -1 || url.hasFragment()) {
            return std::nullopt;
        }

        DeepLink link;
        QSet<QString> seen;
        const QRegularExpression idPattern(QStringLiteral("\\A[1-9][0-9]*\\z"));
        const QRegularExpression guidPattern(QStringLiteral(
            "\\A[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\\z"));
        for (const auto& item : QUrlQuery(url).queryItems(QUrl::FullyDecoded)) {
            if (seen.contains(item.first)) {
                return std::nullopt;
            }
            seen.insert(item.first);
            if (item.first == QLatin1String("teamId") || item.first == QLatin1String("projectId")) {
                bool ok = false;
                const int id = item.second.toInt(&ok);
                if (!ok || !idPattern.match(item.second).hasMatch()) {
                    return std::nullopt;
                }
                (item.first == QLatin1String("teamId") ? link.teamId : link.projectId) = id;
            } else if (item.first == QLatin1String("documentGuid")) {
                link.documentGuid = QUuid(item.second);
                if (!guidPattern.match(item.second).hasMatch() || link.documentGuid.isNull()) {
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
            }
        }
        // Document GUIDs are scoped to a project. A project need not belong to a team.
        if (!link.documentGuid.isNull() && link.projectId == -1) {
            return std::nullopt;
        }
        return link;
    }
};

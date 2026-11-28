#include "snow_shot/presentation/websiteurl.h"

namespace snow_shot::presentation {

QUrl localizedWebsiteUrl(const QUrl& address) {
    // Community uses the project repository, not a language-prefixed website.
    // Absolute URLs (including legal destinations) must never be rewritten.
    const QUrl project(QStringLiteral(SNOW_SHOT_PROJECT_URL));
    if (address.isEmpty())
        return project;
    if (!address.isRelative())
        return address;
    QUrl base = project;
    if (!base.path().endsWith(QLatin1Char('/')))
        base.setPath(base.path() + QLatin1Char('/'));
    QUrl relative = address;
    if (relative.path().startsWith(QLatin1Char('/')))
        relative.setPath(relative.path(QUrl::FullyEncoded).mid(1), QUrl::StrictMode);
    return base.resolved(relative);
}

} // namespace snow_shot::presentation

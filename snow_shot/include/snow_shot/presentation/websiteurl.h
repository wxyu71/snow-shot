#ifndef SNOW_SHOT_PRESENTATION_WEBSITEURL_H
#define SNOW_SHOT_PRESENTATION_WEBSITEURL_H

#include <QUrl>

namespace snow_shot::presentation {

// Resolve a Community project link without rewriting explicit absolute destinations.
// An empty address selects the project repository.
[[nodiscard]] QUrl localizedWebsiteUrl(const QUrl& address = {});

} // namespace snow_shot::presentation

#endif // SNOW_SHOT_PRESENTATION_WEBSITEURL_H

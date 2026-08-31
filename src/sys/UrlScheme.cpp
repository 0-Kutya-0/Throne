#include "include/sys/UrlScheme.hpp"

#include "include/global/Configs.hpp"
#include "include/global/Logger.hpp"

void UrlScheme_RegisterIfNeeded() {
    const QString desired = UrlScheme_DesiredState();
    if (desired.isEmpty()) return;

    auto settings = Configs::dataManager->settingsRepo.get();
    const bool mirrorMatches = settings->url_scheme_mirror == desired;
    if (mirrorMatches && UrlScheme_IsCurrent()) return;
    if (mirrorMatches) LOG_WARN("url scheme registration points elsewhere (another install?), reclaiming it");

    UrlScheme_Apply();
    settings->url_scheme_mirror = desired;
    settings->Save();
}

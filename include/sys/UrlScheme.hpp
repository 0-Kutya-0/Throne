#pragma once

#include <QString>

// Portable install: the exe path moves, so registration re-runs at startup and is diffed against the settings mirror (url_scheme_mirror).

// Opaque per-platform state, revision-prefixed so that adding entries re-registers installs that never moved; empty when unsupported.
QString UrlScheme_DesiredState();

// Per-platform: whether the OS registration still points at this install; the mirror cannot see another install taking the shared entries over.
bool UrlScheme_IsCurrent();

void UrlScheme_Apply();

void UrlScheme_RegisterIfNeeded();

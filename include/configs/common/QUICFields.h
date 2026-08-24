#pragma once
#include "include/configs/baseConfig.h"

namespace Configs
{
    // sing-box embeds these flat into the hysteria/hysteria2/tuic outbounds, so they
    // build as top-level keys rather than a nested object. Empty / 0 means "no override":
    // Build() falls back to the matching global preset in SettingsRepo.
    class QUICFields : public baseConfig
    {
        public:
        QString idle_timeout;
        QString keep_alive_period;
        QString stream_receive_window;
        QString connection_receive_window;
        int max_concurrent_streams = 0;
        int initial_packet_size = 0;
        bool disable_path_mtu_discovery = false;
        bool disable_path_mtu_discovery_unspecified = true;

        // Tri-state combo helpers (index: 0 = Keep Default, 1 = On, 2 = Off).
        int getPathMtuState() const {
            if (disable_path_mtu_discovery) return 1;
            if (disable_path_mtu_discovery_unspecified) return 0;
            return 2;
        }
        void savePathMtuState(const int state) {
            disable_path_mtu_discovery = state == 1;
            disable_path_mtu_discovery_unspecified = state == 0;
        }

        // baseConfig overrides
        bool ParseFromLink(const QString& link) override;
        bool ParseFromJson(const QJsonObject& object) override;
        QString ExportToLink() override;
        QJsonObject ExportToJson() override;
        BuildResult Build() override;
    };
}

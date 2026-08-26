#include "include/ui/profile/edit_hysteria.h"
#include "include/global/Utils.hpp"

EditHysteria::EditHysteria(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::EditHysteria) {
    ui->setupUi(this);

    _protocol_version = ui->protocol_version;
    _obfuscation_type = ui->obfuscation_type;
    _realm_ip_version = ui->realm_ip_version;
    _realm_enabled = ui->realm_enabled;
    _realm_port_mapping = ui->realm_port_mapping;
}

EditHysteria::~EditHysteria() {
    delete ui;
}

void EditHysteria::onStart(std::shared_ptr<Configs::Profile> _ent) {
    this->ent = _ent;
    auto outbound = _ent->Hysteria();

    ui->protocol_version->setCurrentText(outbound->protocol_version);
    ui->server_ports->setText(outbound->server_ports.join(","));
    ui->hop_interval->setText(outbound->hop_interval);
    ui->hop_interval_max->setText(outbound->hop_interval_max);
    ui->up_mbps->setText(Int2String(outbound->up_mbps));
    ui->down_mbps->setText(Int2String(outbound->down_mbps));
    ui->obfs->setText(outbound->obfs);
    ui->auth_type->setCurrentText(outbound->auth_type);
    ui->auth->setText(outbound->auth);
    ui->recv_window->setText(Int2String(outbound->recv_window));
    ui->recv_window_conn->setText(Int2String(outbound->recv_window_conn));
    ui->disable_mtu_discovery->setChecked(outbound->disable_mtu_discovery);
    ui->password->setText(outbound->password);
    ui->min_packet_size->setText(Int2String(outbound->min_packet_size));
    ui->max_packet_size->setText(Int2String(outbound->max_packet_size));
    ui->obfuscation_type->setCurrentText(outbound->obfs_type);
    ui->bbr_profile->setCurrentText(outbound->bbr_profile);
    ui->password->setText(outbound->password);
    ui->disable_chrome_parrot->setChecked(outbound->disable_chrome_parrot);
    ui->realm_enabled->setChecked(outbound->realm_enabled);
    ui->realm_server_url->setText(outbound->realm_server_url);
    ui->realm_id->setText(outbound->realm_id);
    ui->realm_token->setText(outbound->realm_token);
    ui->realm_stun_servers->setText(outbound->realm_stun_servers.join(","));
    ui->realm_ip_version->setCurrentText(outbound->realm_ip_version > 0 ? Int2String(outbound->realm_ip_version) : QString());
    ui->realm_port_mapping->setChecked(outbound->realm_port_mapping);
    ui->realm_port_mapping_timeout->setText(outbound->realm_port_mapping_timeout);
    ui->realm_port_mapping_lifetime->setText(outbound->realm_port_mapping_lifetime);
    editHysteriaLayout(outbound->protocol_version, outbound->obfs_type);
}

bool EditHysteria::onEnd() {
    // The core only checks realm's required fields when it first dials, so a profile missing
    // one would pass config validation and then fail every connection.
    if (ui->realm_enabled->isChecked() && ui->protocol_version->currentText() == "2" &&
        (ui->realm_server_url->text().trimmed().isEmpty() || ui->realm_id->text().trimmed().isEmpty() ||
         SplitAndTrim(ui->realm_stun_servers->text(), ",", false).isEmpty())) {
        MessageBoxWarning(software_name, tr("Realm needs a URL, an ID and at least one STUN server."));
        return false;
    }

    auto outbound = ent->Hysteria();
    outbound->protocol_version = ui->protocol_version->currentText();
    outbound->server_ports = SplitAndTrim(ui->server_ports->text(), ",", false);
    outbound->hop_interval = ui->hop_interval->text();
    outbound->hop_interval_max = ui->hop_interval_max->text();
    outbound->up_mbps = ui->up_mbps->text().toInt();
    outbound->down_mbps = ui->down_mbps->text().toInt();
    outbound->obfs = ui->obfs->text();
    outbound->auth_type = ui->auth_type->currentText();
    outbound->auth = ui->auth->text();
    outbound->recv_window = ui->recv_window->text().toInt();
    outbound->recv_window_conn = ui->recv_window_conn->text().toInt();
    outbound->disable_mtu_discovery = ui->disable_mtu_discovery->isChecked();
    outbound->password = ui->password->text();
    outbound->min_packet_size = ui->min_packet_size->text().toInt();
    outbound->max_packet_size = ui->max_packet_size->text().toInt();
    outbound->obfs_type = ui->obfuscation_type->currentText();
    outbound->bbr_profile = ui->bbr_profile->currentText();
    outbound->disable_chrome_parrot = ui->disable_chrome_parrot->isChecked();
    outbound->realm_enabled = ui->realm_enabled->isChecked();
    outbound->realm_server_url = ui->realm_server_url->text().trimmed();
    outbound->realm_id = ui->realm_id->text().trimmed();
    outbound->realm_token = ui->realm_token->text();
    outbound->realm_stun_servers = SplitAndTrim(ui->realm_stun_servers->text(), ",", false);
    outbound->realm_ip_version = ui->realm_ip_version->currentText().toInt();
    outbound->realm_port_mapping = ui->realm_port_mapping->isChecked();
    outbound->realm_port_mapping_timeout = ui->realm_port_mapping_timeout->text().trimmed();
    outbound->realm_port_mapping_lifetime = ui->realm_port_mapping_lifetime->text().trimmed();
    if (outbound->RealmActive()) {
        // The parent dialog copies its (hidden) address/port fields over ours right after
        // this returns, and the core refuses a realm outbound that also carries a server.
        outbound->server_ports.clear();
        if (set_edit_text_serverAddress) set_edit_text_serverAddress("");
        if (set_edit_text_serverPort) set_edit_text_serverPort("");
    }
    return true;
}

void EditHysteria::editHysteriaLayout(const QString& version, const QString& obfs_type) {
    if (version == "1")
    {
        ui->auth_type->setVisible(true);
        ui->auth_type_l->setVisible(true);
        ui->auth->setVisible(true);
        ui->auth_l->setVisible(true);
        ui->recv_window_conn->setVisible(true);
        ui->recv_window_conn_l->setVisible(true);
        ui->recv_window->setVisible(true);
        ui->recv_window_l->setVisible(true);
        ui->disable_mtu_discovery->setVisible(true);
        ui->password->setVisible(false);
        ui->password_l->setVisible(false);
        ui->min_packet_size->setVisible(false);
        ui->min_packet_size_l->setVisible(false);
        ui->max_packet_size->setVisible(false);
        ui->max_packet_size_l->setVisible(false);
        ui->obfuscation_type->setVisible(false);
        ui->obfuscation_type_l->setVisible(false);
        ui->hop_interval_max->setVisible(false);
        ui->hop_interval_max_l->setVisible(false);
        ui->bbr_profile->setVisible(false);
        ui->bbr_profile_l->setVisible(false);
    } else
    {
        ui->auth_type->setVisible(false);
        ui->auth_type_l->setVisible(false);
        ui->auth->setVisible(false);
        ui->auth_l->setVisible(false);
        ui->recv_window_conn->setVisible(false);
        ui->recv_window_conn_l->setVisible(false);
        ui->recv_window->setVisible(false);
        ui->recv_window_l->setVisible(false);
        ui->disable_mtu_discovery->setVisible(false);
        ui->password->setVisible(true);
        ui->password_l->setVisible(true);
        ui->obfuscation_type->setVisible(true);
        ui->obfuscation_type_l->setVisible(true);
        ui->hop_interval_max->setVisible(true);
        ui->hop_interval_max_l->setVisible(true);
        ui->bbr_profile->setVisible(true);
        ui->bbr_profile_l->setVisible(true);
        if (obfs_type == "gecko") {
            ui->min_packet_size->setVisible(true);
            ui->min_packet_size_l->setVisible(true);
            ui->max_packet_size->setVisible(true);
            ui->max_packet_size_l->setVisible(true);
        }
        if (obfs_type == "salamander") {
            ui->min_packet_size->setVisible(false);
            ui->min_packet_size_l->setVisible(false);
            ui->max_packet_size->setVisible(false);
            ui->max_packet_size_l->setVisible(false);
        }
    }

    const auto realm = version == "2" && ui->realm_enabled->isChecked();
    ui->disable_chrome_parrot->setVisible(version == "2");
    ui->realm_enabled->setVisible(version == "2");
    ui->realm_server_url->setVisible(realm);
    ui->realm_server_url_l->setVisible(realm);
    ui->realm_id->setVisible(realm);
    ui->realm_id_l->setVisible(realm);
    ui->realm_token->setVisible(realm);
    ui->realm_token_l->setVisible(realm);
    ui->realm_stun_servers->setVisible(realm);
    ui->realm_stun_servers_l->setVisible(realm);
    ui->realm_ip_version->setVisible(realm);
    ui->realm_ip_version_l->setVisible(realm);
    // Gateway mappings are IPv4-only, so the core rejects them alongside "ip_version": 6.
    const auto portMapping = realm && ui->realm_ip_version->currentText() != "6";
    ui->realm_port_mapping->setVisible(portMapping);
    const auto portMappingTiming = portMapping && ui->realm_port_mapping->isChecked();
    ui->realm_port_mapping_timeout->setVisible(portMappingTiming);
    ui->realm_port_mapping_timeout_l->setVisible(portMappingTiming);
    ui->realm_port_mapping_lifetime->setVisible(portMappingTiming);
    ui->realm_port_mapping_lifetime_l->setVisible(portMappingTiming);

    // realm carries the endpoint itself; port hopping needs a server port range it forbids.
    ui->server_ports->setVisible(!realm);
    ui->label->setVisible(!realm);
    ui->hop_interval->setVisible(!realm);
    ui->label_2->setVisible(!realm);
    if (realm) {
        ui->hop_interval_max->setVisible(false);
        ui->hop_interval_max_l->setVisible(false);
    }
}

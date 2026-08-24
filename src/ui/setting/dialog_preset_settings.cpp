#include "include/ui/setting/dialog_preset_settings.h"

#include "include/global/GuiUtils.hpp"
#include "include/global/Configs.hpp"
#include "include/ui/mainwindow_interface.h"

#include <QRegularExpression>
#include <QScreen>
#include <QValidator>

namespace {
    // 0 means "keep the core default" and is omitted from the config, so show it as blank.
    void LoadOptionalInt(QLineEdit *edit, const int value, QObject *parent) {
        edit->setText(value > 0 ? Int2String(value) : "");
        edit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]+$"), parent));
    }
}

DialogPresetSettings::DialogPresetSettings(QWidget *parent) : QDialog(parent), ui(new Ui::DialogPresetSettings) {
    ui->setupUi(this);
    ADD_ASTERISK(this);

    // Multiplex
    ui->mux_protocol->addItems({"h2mux", "smux", "yamux"});
    D_LOAD_COMBO_STRING(mux_protocol)
    D_LOAD_INT(mux_concurrency)
    D_LOAD_BOOL(mux_padding)
    D_LOAD_BOOL(mux_default_on)
    ui->xray_mux_concurrency->setText(Int2String(Configs::dataManager->settingsRepo->xray_mux_concurrency));
    ui->xray_default_mux->setChecked(Configs::dataManager->settingsRepo->xray_mux_default_on);

    // TLS
    ui->fragment_implementation->addItems({"built-in", "custom"});
    D_LOAD_COMBO_STRING(fragment_implementation)
    D_LOAD_STRING(fragment_size)
    D_LOAD_STRING(fragment_sleep)
    D_LOAD_BOOL(fragment_default_on)
    D_LOAD_BOOL(tls_tricks_default_on)
    ui->fragment_size->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]+(-[0-9]+)?$"), this));
    ui->fragment_sleep->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]+(-[0-9]+)?$"), this));
    // size/sleep only affect the custom implementation, so enable them only for it
    auto syncFragParams = [this](const QString &impl) {
        bool custom = impl == "custom";
        ui->fragment_size->setEnabled(custom);
        ui->fragment_sleep->setEnabled(custom);
        ui->fragment_size_l->setEnabled(custom);
        ui->fragment_sleep_l->setEnabled(custom);
    };
    connect(ui->fragment_implementation, &QComboBox::currentTextChanged, this, syncFragParams);
    syncFragParams(ui->fragment_implementation->currentText());
    ui->utlsFingerprint->addItems(Configs::tlsFingerprints);
    ui->utlsFingerprint->setCurrentText(Configs::dataManager->settingsRepo->utlsFingerprint);

    // HTTP/2 & QUIC
    D_LOAD_STRING(h2_idle_timeout)
    D_LOAD_STRING(h2_keep_alive_period)
    D_LOAD_STRING(h2_stream_receive_window)
    D_LOAD_STRING(h2_connection_receive_window)
    LoadOptionalInt(ui->h2_max_concurrent_streams, Configs::dataManager->settingsRepo->h2_max_concurrent_streams, this);
    LoadOptionalInt(ui->quic_initial_packet_size, Configs::dataManager->settingsRepo->quic_initial_packet_size, this);
    D_LOAD_BOOL(quic_disable_path_mtu_discovery)

    QSize want = sizeHint();
    if (const QScreen *scr = parent ? parent->screen() : screen()) {
        const QRect avail = scr->availableGeometry();
        want = want.boundedTo(QSize(avail.width() - 24, avail.height() - 72));
    }
    resize(want);
}

DialogPresetSettings::~DialogPresetSettings() {
    delete ui;
}

void DialogPresetSettings::accept() {
    // Multiplex
    D_SAVE_COMBO_STRING(mux_protocol)
    D_SAVE_INT(mux_concurrency)
    D_SAVE_BOOL(mux_padding)
    D_SAVE_BOOL(mux_default_on)
    Configs::dataManager->settingsRepo->xray_mux_concurrency = ui->xray_mux_concurrency->text().toInt();
    Configs::dataManager->settingsRepo->xray_mux_default_on = ui->xray_default_mux->isChecked();

    // TLS
    D_SAVE_COMBO_STRING(fragment_implementation)
    D_SAVE_STRING(fragment_size)
    D_SAVE_STRING(fragment_sleep)
    D_SAVE_BOOL(fragment_default_on)
    D_SAVE_BOOL(tls_tricks_default_on)
    Configs::dataManager->settingsRepo->utlsFingerprint = ui->utlsFingerprint->currentText();

    // HTTP/2 & QUIC
    D_SAVE_STRING(h2_idle_timeout)
    D_SAVE_STRING(h2_keep_alive_period)
    D_SAVE_STRING(h2_stream_receive_window)
    D_SAVE_STRING(h2_connection_receive_window)
    D_SAVE_INT(h2_max_concurrent_streams)
    D_SAVE_INT(quic_initial_packet_size)
    D_SAVE_BOOL(quic_disable_path_mtu_discovery)

    MW_dialog_message(MwMessage::UpdateSettings, {});
    QDialog::accept();
}

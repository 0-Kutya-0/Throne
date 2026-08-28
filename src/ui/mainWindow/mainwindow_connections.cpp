#include "include/ui/mainwindow.h"
#include "include/ui/utils/ConnectionsFilterHeader.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <memory>

namespace
{
    QString ProtocolText(const Stats::ConnectionMetadata& conn)
    {
        return conn.protocol.isEmpty() ? conn.network : conn.network + " (" + conn.protocol + ")";
    }
}

void MainWindow::setupConnectionList()
{
    connectionFilterHeader = new ConnectionsFilterHeader(ui->connections);
    ui->connections->setHorizontalHeader(connectionFilterHeader);

    ui->connections->horizontalHeader()->setHighlightSections(false);
    ui->connections->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->connections->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->connections->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->connections->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->connections->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->connections->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->connections->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->connections->verticalHeader()->hide();
    restoreConnectionSort();
    setupConnectionSortMenu();
    setupConnectionFilter();
    connect(ui->connections, &QTableWidget::cellClicked, this, [=,this](int row, int column)
    {
        auto selected = ui->connections->item(row, column);
        if (selected == nullptr) return;
        QApplication::clipboard()->setText(selected->text());
        QPoint pos = ui->connections->mapToGlobal(ui->connections->visualItemRect(selected).center());
        QToolTip::showText(pos, tr("Copied!"), this);
        auto r = ++toolTipID;
        QTimer::singleShot(1500, [=,this] {
            if (r != toolTipID)
            {
                return;
            }
            QToolTip::hideText();
        });
    });
}

void MainWindow::restoreConnectionSort()
{
    const auto* settings = Configs::dataManager->settingsRepo.get();
    const int stored = settings->connection_sort;
    if (stored < Stats::Default || stored > Stats::BySpeed) return;
    // Runs before setup_rpc() spawns the lister thread, so writing the pair unguarded is safe.
    Stats::connection_lister->restoreSort(static_cast<Stats::ConnectionSort>(stored), settings->connection_sort_asc);
}

void MainWindow::applyConnectionSort(Stats::ConnectionSort sort)
{
    Stats::connection_lister->setSort(sort);
    auto* settings = Configs::dataManager->settingsRepo.get();
    settings->connection_sort = Stats::connection_lister->getSort();
    settings->connection_sort_asc = Stats::connection_lister->isSortAscending();
    settings->Save();
    Stats::connection_lister->ForceUpdate();
}

void MainWindow::setupConnectionFilter()
{
    auto* btnFilter = new QToolButton(this);
    btnFilter->setIcon(QIcon(":/icon/filter.png"));
    btnFilter->setToolTip(tr("Enable Filter"));
    btnFilter->setCheckable(true);
    connect(btnFilter, &QToolButton::toggled, connectionFilterHeader, &ConnectionsFilterHeader::setFiltersVisible);
    connect(connectionFilterHeader, &ConnectionsFilterHeader::closeRequested, btnFilter, [btnFilter] { btnFilter->setChecked(false); });
    ui->stats_widget->setCornerWidget(btnFilter, Qt::TopRightCorner);

    // The corner widget spans the whole tab bar, so it stays put and only greys out away from the connections tab.
    auto syncEnabled = [=,this] { btnFilter->setEnabled(ui->stats_widget->currentWidget() == ui->connections_tab); };
    connect(ui->stats_widget, &QTabWidget::currentChanged, this, [syncEnabled](int) { syncEnabled(); });
    syncEnabled();

    connectionFilterDebounce = new QTimer(this);
    connectionFilterDebounce->setSingleShot(true);
    connectionFilterDebounce->setInterval(50);
    connect(connectionFilterDebounce, &QTimer::timeout, this, [this] { applyConnectionFilters(); });
    connect(connectionFilterHeader, &ConnectionsFilterHeader::filtersChanged, this, [this] { connectionFilterDebounce->start(); });
}

void MainWindow::applyConnectionFilters()
{
    connectionListMu.lock();
    ui->connections->setUpdatesEnabled(false);
    const bool active = connectionFilterHeader->hasActiveFilter();
    for (int row = 0; row < ui->connections->rowCount(); row++)
    {
        bool hide = false;
        if (active)
        {
            auto text = [this, row](int column)
            {
                const auto* item = ui->connections->item(row, column);
                return item == nullptr ? QString() : item->text();
            };
            hide = !connectionFilterHeader->accepts(text(0), text(1), text(2), text(3));
        }
        ui->connections->setRowHidden(row, hide);
    }
    ui->connections->setUpdatesEnabled(true);
    connectionListMu.unlock();
}

void MainWindow::setupConnectionSortMenu()
{
    auto* header = ui->connections->horizontalHeader();
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QWidget::customContextMenuRequested, this, [=,this](const QPoint& pos)
    {
        const int columnIndex = header->logicalIndexAt(pos);
        const bool isTraffic = columnIndex == 4;
        const bool isSpeed = columnIndex == 5;
        if (!isTraffic && !isSpeed) return;

        struct SortOption { Stats::ConnectionSort value; QString label; };
        const QList<SortOption> options = isTraffic
            ? QList<SortOption>{
                { Stats::ByTraffic, tr("Total") },
                { Stats::ByDownload, tr("Downloaded") },
                { Stats::ByUpload, tr("Uploaded") } }
            : QList<SortOption>{
                { Stats::BySpeed, tr("Total") },
                { Stats::ByDownloadSpeed, tr("Download Speed") },
                { Stats::ByUploadSpeed, tr("Upload Speed") } };

        QMenu menu(this);
        auto* sortByLabel = menu.addAction(tr("Sort By:"));
        sortByLabel->setEnabled(false);

        const auto current = Stats::connection_lister->getSort();
        for (const auto& opt : options)
        {
            auto* act = menu.addAction(opt.label);
            act->setData(static_cast<int>(opt.value));
            act->setCheckable(true);
            act->setChecked(current == opt.value);
        }

        auto* chosen = menu.exec(header->mapToGlobal(pos));
        if (chosen == nullptr || !chosen->data().isValid()) return;

        applyConnectionSort(static_cast<Stats::ConnectionSort>(chosen->data().toInt()));
    });
}

void MainWindow::UpdateConnectionList(const QMap<QString, Stats::ConnectionMetadata>& toUpdate, const QMap<QString, Stats::ConnectionMetadata>& toAdd)
{
    connectionListMu.lock();
    ui->connections->setUpdatesEnabled(false);
    for (int row=0;row<ui->connections->rowCount();row++)
    {
        const auto key = ui->connections->item(row, 0)->data(Stats::IDKEY).toString();
        if (!toUpdate.contains(key))
        {
            ui->connections->removeRow(row);
            row--;
            continue;
        }

        const auto conn = toUpdate[key];
        const auto dest = DisplayDest(conn.dest, conn.domain);
        const auto prot = ProtocolText(conn);
        ui->connections->item(row, 0)->setText(dest);

        ui->connections->item(row, 1)->setText(conn.process);

        ui->connections->item(row, 2)->setText(prot);

        ui->connections->item(row, 3)->setText(conn.outbound);

        ui->connections->item(row, 4)->setText(ReadableSize(conn.upload) + "↑" + " " + ReadableSize(conn.download) + "↓");

        ui->connections->item(row, 5)->setText(ReadableSize(conn.uploadSpeed) + "/s↑" + " " + ReadableSize(conn.downloadSpeed) + "/s↓");

        ui->connections->setRowHidden(row, !connectionFilterHeader->accepts(dest, conn.process, prot, conn.outbound));
    }
    int row = ui->connections->rowCount();
    for (const auto& conn : toAdd)
    {
        ui->connections->insertRow(row);
        auto f0 = std::make_unique<QTableWidgetItem>();
        f0->setData(Stats::IDKEY, conn.id);

        const auto dest = DisplayDest(conn.dest, conn.domain);
        const auto prot = ProtocolText(conn);

        auto f = f0->clone();
        f->setText(dest);
        ui->connections->setItem(row, 0, f);

        f = f0->clone();
        f->setText(conn.process);
        ui->connections->setItem(row, 1, f);

        f = f0->clone();
        f->setText(prot);
        ui->connections->setItem(row, 2, f);

        f = f0->clone();
        f->setText(conn.outbound);
        ui->connections->setItem(row, 3, f);

        f = f0->clone();
        f->setText(ReadableSize(conn.upload) + "↑" + " " + ReadableSize(conn.download) + "↓");
        ui->connections->setItem(row, 4, f);

        f = f0->clone();
        f->setText(ReadableSize(conn.uploadSpeed) + "/s↑" + " " + ReadableSize(conn.downloadSpeed) + "/s↓");
        ui->connections->setItem(row, 5, f);

        ui->connections->setRowHidden(row, !connectionFilterHeader->accepts(dest, conn.process, prot, conn.outbound));

        row++;
    }
    ui->connections->setUpdatesEnabled(true);
    connectionListMu.unlock();
}

void MainWindow::UpdateConnectionListWithRecreate(const QList<Stats::ConnectionMetadata>& connections)
{
    connectionListMu.lock();
    ui->connections->setUpdatesEnabled(false);
    ui->connections->setRowCount(0);
    int row=0;
    for (const auto& conn : connections)
    {
        ui->connections->insertRow(row);
        auto f0 = std::make_unique<QTableWidgetItem>();
        f0->setData(Stats::IDKEY, conn.id);

        const auto dest = DisplayDest(conn.dest, conn.domain);
        const auto prot = ProtocolText(conn);

        auto f = f0->clone();
        f->setText(dest);
        ui->connections->setItem(row, 0, f);

        f = f0->clone();
        f->setText(conn.process);
        ui->connections->setItem(row, 1, f);

        f = f0->clone();
        f->setText(prot);
        ui->connections->setItem(row, 2, f);

        f = f0->clone();
        f->setText(conn.outbound);
        ui->connections->setItem(row, 3, f);

        f = f0->clone();
        f->setText(ReadableSize(conn.upload) + "↑" + " " + ReadableSize(conn.download) + "↓");
        ui->connections->setItem(row, 4, f);

        f = f0->clone();
        f->setText(ReadableSize(conn.uploadSpeed) + "/s↑" + " " + ReadableSize(conn.downloadSpeed) + "/s↓");
        ui->connections->setItem(row, 5, f);

        ui->connections->setRowHidden(row, !connectionFilterHeader->accepts(dest, conn.process, prot, conn.outbound));

        row++;
    }
    ui->connections->setUpdatesEnabled(true);
    connectionListMu.unlock();
}

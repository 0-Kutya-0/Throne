#pragma once

#include <QHeaderView>
#include <QLineEdit>
#include <QVector>
#include <QScrollBar>
#include <QToolButton>

#include "include/ui/utils/ProfilesTableModel.h"

class ProfilesTableFilterHeader : public QHeaderView {
    Q_OBJECT
public:
    explicit ProfilesTableFilterHeader(QWidget *parent = nullptr)
        : QHeaderView(Qt::Horizontal, parent) {
        setSectionsClickable(true);
        setSortIndicatorShown(true);
        setDefaultAlignment(Qt::AlignHCenter | Qt::AlignTop);

        type_filter = new QLineEdit(this->viewport()); 
        type_filter->setPlaceholderText(tr("Filter..."));
        type_filter->setClearButtonEnabled(true);
        connect(type_filter, &QLineEdit::textChanged, [this](const QString &text) {
            emit typeFilterChanged(text);
        });

        address_filter = new QLineEdit(this->viewport());
        address_filter->setPlaceholderText(tr("Filter..."));
        address_filter->setClearButtonEnabled(true);
        connect(address_filter, &QLineEdit::textChanged, [this](const QString &text) {
            emit addressFilterChanged(text);
        });

        name_filter = new QLineEdit(this->viewport()); 
        name_filter->setPlaceholderText(tr("Filter..."));
        name_filter->setClearButtonEnabled(true);
        connect(name_filter, &QLineEdit::textChanged, [this](const QString &text) {
            emit nameFilterChanged(text);
        });

        test_filter = new QLineEdit(this->viewport()); 
        test_filter->setPlaceholderText(tr("Filter by country..."));
        test_filter->setClearButtonEnabled(true);
        connect(test_filter, &QLineEdit::textChanged, [this](const QString &text) {
            emit testFilterChanged(text);
        });

        connect(this, &QHeaderView::sectionResized, this, &ProfilesTableFilterHeader::adjustPositions);

        setFiltersVisible(false);
    }

    QSize sizeHint() const override {
        QSize s = QHeaderView::sizeHint();
        if (m_filtersVisible) {
            s.setHeight(s.height() + 32);
        }
        return s;
    }

protected:
    void updateGeometries() override {
        QHeaderView::updateGeometries();
        adjustPositions();
    }

public slots:
    void setFiltersVisible(bool visible) {
        m_filtersVisible = visible;

        if (!visible) {
            type_filter->clear();
            address_filter->clear();
            name_filter->clear();
            test_filter->clear();
        }

        if (auto btn = qobject_cast<QToolButton*>(sender())) {
            btn->setToolTip(QString("%1\n%2").arg(visible ? tr("Disable Filter") : tr("Enable Filter"), QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)));
        }
        
        type_filter->setVisible(visible);
        address_filter->setVisible(visible);
        name_filter->setVisible(visible);
        test_filter->setVisible(visible);

        emit geometriesChanged();
    }

    void adjustPositions() {
        if (!m_filtersVisible || !address_filter || !name_filter || !type_filter
            || !test_filter || count() < ProfilesTableModel::ColumnCount) {
	        return;
	    }

        const int editHeight = 24;
        const int topPos = height() - editHeight - 4;

        auto place = [&](QLineEdit *edit, int section) {
            edit->setGeometry(sectionViewportPosition(section) + 2, topPos, sectionSize(section) - 4, editHeight);
        };
        place(type_filter, ProfilesTableModel::ColType);
        place(address_filter, ProfilesTableModel::ColAddress);
        place(name_filter, ProfilesTableModel::ColName);
        place(test_filter, ProfilesTableModel::ColTestResult);
    }

signals:
    void typeFilterChanged(const QString &text);
    void addressFilterChanged(const QString &text);
    void nameFilterChanged(const QString &text);
    void testFilterChanged(const QString &text);

private:
    QLineEdit* type_filter;
    QLineEdit* address_filter;
    QLineEdit* name_filter;
    QLineEdit* test_filter;
    bool m_filtersVisible = false;
};

#pragma once

// Keeps dialogs from growing past the screen (issue #1629).
//
// At high UI scaling (e.g. 200% on a small tablet) our content-dense dialogs
// have a layout minimum taller/wider than the available screen. Qt then opens
// them oversized and QDialog::adjustPosition only clamps the *position*, pinning
// the top to the screen edge and pushing the OK/Cancel button box off the
// bottom where it can't be reached.
//
// makeDialogResponsive() fixes this generically at runtime:
//   * moves the dialog's content into a QScrollArea (scrollbars appear only
//     when the content doesn't fit), and
//   * caps the window's maximum size to the screen work area.
// Any QDialogButtonBox is pulled out of the scrolled content and pinned below
// the scroll area so the primary buttons stay visible at all times.

#include <QDialog>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLayout>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace responsive_detail {
    // Recursively remove `w` from the layout tree rooted at `lay`, deleting only
    // the wrapping QLayoutItem (the widget itself survives for re-parenting).
    inline bool removeWidgetFromLayout(QLayout *lay, QWidget *w) {
        if (!lay) return false;
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem *item = lay->itemAt(i);
            if (item->widget() == w) {
                delete lay->takeAt(i);
                return true;
            }
            if (item->layout() && removeWidgetFromLayout(item->layout(), w)) return true;
        }
        return false;
    }
}

// A QScrollArea that advertises its inner widget's preferred size as its own
// size hint. This lets a wrapped dialog's adjustSize()/resize() keep sizing to
// the content (as before), while the small minimumSizeHint still allows the
// window to shrink to the screen cap with scrollbars.
class SizeHintScrollArea : public QScrollArea {
public:
    explicit SizeHintScrollArea(QWidget *parent = nullptr) : QScrollArea(parent) {}

    QSize sizeHint() const override {
        QWidget *w = widget();
        if (!w) return QScrollArea::sizeHint();
        QSize s = w->sizeHint();
        if (!s.isValid()) s = w->minimumSizeHint();
        const int fw = frameWidth();
        return s + QSize(2 * fw, 2 * fw);
    }

    QSize minimumSizeHint() const override {
        // Deliberately small so the inner content's (possibly huge) minimum does
        // not force the dialog larger than the screen; scrollbars cover the rest.
        return QSize(120, 120);
    }
};

// Wrap `dlg`'s content in a scroll area and cap its size to the screen.
// Call once, at the end of the dialog's constructor (after setupUi and any
// content population). `screenAnchor` picks the screen to size against; when
// null the dialog's parent (usually the main window) is used.
inline void makeDialogResponsive(QDialog *dlg, QWidget *screenAnchor = nullptr) {
    if (!dlg) return;
    if (dlg->property("responsiveApplied").toBool()) return;
    QLayout *content = dlg->layout();
    if (!content) return; // nothing to wrap

    // Locate the button box before re-parenting, while it is still under `dlg`.
    QDialogButtonBox *bb = dlg->findChild<QDialogButtonBox *>();

    // Move the whole content layout onto an inner container. setLayout() steals
    // the layout off `dlg` (dlg->layout() becomes null) and re-parents children.
    auto *inner = new QWidget;
    inner->setObjectName(QStringLiteral("responsiveContent"));
    inner->setLayout(content);

    // Detach the button box from the (now inner) layout tree so it can be pinned.
    if (bb && bb->parentWidget() && bb->parentWidget()->layout()) {
        responsive_detail::removeWidgetFromLayout(bb->parentWidget()->layout(), bb);
    }

    auto *scroll = new SizeHintScrollArea(dlg);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(inner);

    // Fresh top-level layout for the dialog: [ scroll area ] over [ button box ].
    auto *outer = new QVBoxLayout(dlg);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll, 1);
    if (bb) outer->addWidget(bb, 0);

    // Cap the window to the screen work area, leaving headroom for the window
    // frame / title bar so the whole window (not just its client area) fits.
    QScreen *scr = nullptr;
    if (screenAnchor) scr = screenAnchor->screen();
    if (!scr && dlg->parentWidget()) scr = dlg->parentWidget()->screen();
    if (!scr) scr = dlg->screen();
    if (!scr) scr = QGuiApplication::primaryScreen();
    if (scr) {
        const QRect avail = scr->availableGeometry();
        const int capW = qMax(320, avail.width() - 24);
        const int capH = qMax(240, avail.height() - 72);
        dlg->setMaximumSize(capW, capH);
    }

    dlg->setProperty("responsiveApplied", true);
}

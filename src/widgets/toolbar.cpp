#include "toolbar.h"
#include "i18n.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QSize>
#include <QAction>

VideoToolbar::VideoToolbar(QWidget* parent) : QWidget(parent) {
    setStyleSheet("background:#16162a;padding:4px;");
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(8,4,8,4);
    lay->setSpacing(6);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(_("search"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    m_searchEdit->setStyleSheet(
        "QLineEdit{background:#1e1e2e;color:#e0e0e0;border:1px solid #2a2a4e;border-radius:4px;padding:4px 8px;font-size:11px;}"
        "QLineEdit:focus{border-color:#6c5ce7;}"
    );
    connect(m_searchEdit, &QLineEdit::textChanged, this, &VideoToolbar::searchChanged);
    lay->addWidget(m_searchEdit, 1);

    m_sortCombo = new QComboBox();
    m_sortCombo->addItem(_("sort_default"), "default");
    m_sortCombo->addItem(_("sort_0_9"), "0-9");
    m_sortCombo->addItem(_("sort_a_z"), "a-z");
    m_sortCombo->addItem(_("sort_z_a"), "z-a");
    m_sortCombo->setStyleSheet(
        "QComboBox{background:#1e1e2e;color:#e0e0e0;border:1px solid #2a2a4e;border-radius:4px;padding:4px;font-size:11px;}"
        "QComboBox:hover{background:#2a2a4e;}"
        "QComboBox::drop-down{border:none;width:16px;}"
        "QComboBox QAbstractItemView{background:#1a1a30;color:#fff;selection-background-color:#6c5ce7;border:none;}"
    );
    m_sortCombo->setFocusPolicy(Qt::ClickFocus);
    m_sortCombo->setCurrentIndex(1);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        emit sortChanged(m_sortCombo->currentData().toString());
    });
    lay->addWidget(m_sortCombo);

    m_filterCombo = new QComboBox();
    m_filterCombo->setFocusPolicy(Qt::ClickFocus);
    m_filterCombo->addItem(_("filter_all"), "all");
    m_filterCombo->addItem(_("filter_watched"), "watched");
    m_filterCombo->addItem(_("filter_unwatched"), "unwatched");
    m_filterCombo->setStyleSheet(m_sortCombo->styleSheet());
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        emit filterChanged(m_filterCombo->currentData().toString());
    });
    lay->addWidget(m_filterCombo);

    m_viewToggle = new QPushButton();
    m_viewToggle->setIcon(QIcon(":/icons/grid_view.svg"));
    m_viewToggle->setIconSize(QSize(16, 16));
    m_viewToggle->setText(_("view_list"));
    m_viewToggle->setStyleSheet(
        "QPushButton{background:#2a2a4e;color:#fff;border:none;border-radius:4px;padding:4px 10px;font-size:11px;}"
        "QPushButton:hover{background:#6c5ce7;}"
    );
    connect(m_viewToggle, &QPushButton::clicked, this, [this]() {
        m_gridMode = !m_gridMode;
        if (m_gridMode) {
            m_viewToggle->setIcon(QIcon(":/icons/list_view.svg"));
            m_viewToggle->setText(_("view_list"));
        } else {
            m_viewToggle->setIcon(QIcon(":/icons/grid_view.svg"));
            m_viewToggle->setText(_("view_grid"));
        }
        emit viewToggled(m_gridMode);
    });
    lay->addWidget(m_viewToggle);
}

void VideoToolbar::retranslate() {
    m_searchEdit->setPlaceholderText(_("search"));
    m_sortCombo->setItemText(0, _("sort_default"));
    m_sortCombo->setItemText(1, _("sort_0_9"));
    m_sortCombo->setItemText(2, _("sort_a_z"));
    m_sortCombo->setItemText(3, _("sort_z_a"));
    m_filterCombo->setItemText(0, _("filter_all"));
    m_filterCombo->setItemText(1, _("filter_watched"));
    m_filterCombo->setItemText(2, _("filter_unwatched"));
    if (m_gridMode) {
        m_viewToggle->setIcon(QIcon(":/icons/list_view.svg"));
        m_viewToggle->setText(_("view_list"));
    } else {
        m_viewToggle->setIcon(QIcon(":/icons/grid_view.svg"));
        m_viewToggle->setText(_("view_grid"));
    }
}

#include "ProjectPickerDialog.hpp"
#include <QFileDialog>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFont>
#include <QHBoxLayout>

ProjectPickerDialog::ProjectPickerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Open Project with Claude Code");
    setMinimumSize(480, 400);

    auto *layout = new QVBoxLayout(this);

    auto *titleLabel = new QLabel("Select a project directory:");
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    m_listWidget = new QListWidget;
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_listWidget, 1);

    auto *buttonLayout = new QHBoxLayout;
    m_browseButton = new QPushButton("Browse...");
    m_openButton = new QPushButton("Open");
    m_openButton->setEnabled(false);
    m_cancelButton = new QPushButton("Cancel");

    buttonLayout->addWidget(m_browseButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_openButton);
    buttonLayout->addWidget(m_cancelButton);
    layout->addLayout(buttonLayout);

    connect(m_browseButton, &QPushButton::clicked, this, &ProjectPickerDialog::browseForProject);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_openButton, &QPushButton::clicked, [this]() {
        auto items = m_listWidget->selectedItems();
        if (!items.isEmpty()) {
            m_selectedProject = items.first()->data(Qt::UserRole).toString();
            accept();
        }
    });
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &ProjectPickerDialog::onItemDoubleClicked);
    connect(m_listWidget, &QListWidget::itemSelectionChanged, [this]() {
        m_openButton->setEnabled(!m_listWidget->selectedItems().isEmpty());
    });

    populateRecentProjects();
    discoverClaudeProjects();
}

void ProjectPickerDialog::populateRecentProjects() {
    QSettings settings("pixelomer", "Shijima-Qt");
    QStringList recent = settings.value("recentProjects").toStringList();

    for (const QString &path : recent) {
        if (QDir(path).exists()) {
            addProjectEntry(path);
        }
    }
}

void ProjectPickerDialog::discoverClaudeProjects() {
    // Scan ~/.claude/projects/ for previously used Claude Code projects
    QString claudeProjectsDir = QDir::homePath() + "/.claude/projects";
    QDir dir(claudeProjectsDir);
    if (!dir.exists()) return;

    for (const QString &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        // Decode the path encoding: leading '-' stripped, '-' → '/'
        QString decoded = entry;
        if (decoded.startsWith('-')) {
            decoded = decoded.mid(1);
        }
        decoded.replace('-', '/');

        if (QDir(decoded).exists()) {
            addProjectEntry(decoded);
        }
    }
}

void ProjectPickerDialog::addProjectEntry(QString const& path) {
    QString canonical = QDir(path).canonicalPath();
    if (canonical.isEmpty() || m_addedPaths.contains(canonical)) return;
    m_addedPaths.insert(canonical);

    QString name = QFileInfo(canonical).fileName();
    auto *item = new QListWidgetItem;
    item->setText(name + "\n" + canonical);
    item->setData(Qt::UserRole, canonical);
    item->setToolTip(canonical);
    m_listWidget->addItem(item);
}

void ProjectPickerDialog::browseForProject() {
    QString dir = QFileDialog::getExistingDirectory(this,
        "Select Project Directory", QDir::homePath());
    if (!dir.isEmpty()) {
        m_selectedProject = QDir(dir).canonicalPath();

        // Add to recent projects
        QSettings settings("pixelomer", "Shijima-Qt");
        QStringList recent = settings.value("recentProjects").toStringList();
        recent.removeAll(m_selectedProject);
        recent.prepend(m_selectedProject);
        while (recent.size() > 20) recent.removeLast();
        settings.setValue("recentProjects", recent);

        accept();
    }
}

void ProjectPickerDialog::onItemDoubleClicked(QListWidgetItem *item) {
    m_selectedProject = item->data(Qt::UserRole).toString();

    // Add to recent projects
    QSettings settings("pixelomer", "Shijima-Qt");
    QStringList recent = settings.value("recentProjects").toStringList();
    recent.removeAll(m_selectedProject);
    recent.prepend(m_selectedProject);
    while (recent.size() > 20) recent.removeLast();
    settings.setValue("recentProjects", recent);

    accept();
}

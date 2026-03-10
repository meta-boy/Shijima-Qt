#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

class ProjectPickerDialog : public QDialog {
public:
    explicit ProjectPickerDialog(QWidget *parent = nullptr);
    QString selectedProject() const { return m_selectedProject; }

private:
    void populateRecentProjects();
    void discoverClaudeProjects();
    void addProjectEntry(QString const& path);
    void browseForProject();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onItemClicked(QListWidgetItem *item);

    QListWidget *m_listWidget;
    QPushButton *m_browseButton;
    QPushButton *m_openButton;
    QPushButton *m_cancelButton;
    QString m_selectedProject;
    QSet<QString> m_addedPaths;
};

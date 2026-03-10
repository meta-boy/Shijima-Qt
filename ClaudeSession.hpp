#pragma once

#include <QString>
#include <QSettings>
#include <QProcess>

class ClaudeSession {
public:
    explicit ClaudeSession(QString const& projectPath);
    ~ClaudeSession();

    void launchTerminal();
    void focusTerminal();
    bool isTerminalRunning() const;
    bool wasLaunched() const { return m_launched; }
    QString displayName() const;
    QString projectPath() const { return m_projectPath; }

    void setProjectPath(QString const& path);

private:
    QString m_projectPath;
    QString m_displayName;
    qint64 m_terminalPid = 0;
    bool m_launched = false;
    QString m_pidFile;
    QString m_scriptFile;

    QString terminalApp() const;
    void cleanupTempFiles();
    void readPidFile();
    QString createWrapperScript();
    void launchMacOS();
    void launchLinux();
    void focusMacOS();
    void focusLinux();
};

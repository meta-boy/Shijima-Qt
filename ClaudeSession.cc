#include "ClaudeSession.hpp"
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QCoreApplication>
#include <QFile>
#include <iostream>

#if defined(__APPLE__) || defined(__linux__)
#include <signal.h>
#endif

static quint64 s_sessionCounter = 0;

ClaudeSession::ClaudeSession(QString const& projectPath)
    : m_projectPath(projectPath)
{
    m_displayName = QFileInfo(projectPath).fileName();

    // Generate unique temp file paths for this session
    QString base = QDir::tempPath() + "/shijima-qt-"
        + QString::number(QCoreApplication::applicationPid()) + "-"
        + QString::number(s_sessionCounter++);
    m_pidFile = base + ".pid";
    m_scriptFile = base + ".sh";
}

ClaudeSession::~ClaudeSession() {
    cleanupTempFiles();
}

void ClaudeSession::cleanupTempFiles() {
    QFile::remove(m_pidFile);
    QFile::remove(m_scriptFile);
}

QString ClaudeSession::displayName() const {
    return m_displayName;
}

void ClaudeSession::setProjectPath(QString const& path) {
    m_projectPath = path;
    m_displayName = QFileInfo(path).fileName();
    m_terminalPid = 0;
    m_launched = false;
    cleanupTempFiles();
}

QString ClaudeSession::terminalApp() const {
    QSettings settings("pixelomer", "Shijima-Qt");
    return settings.value("terminalApp", "default").toString();
}

bool ClaudeSession::isTerminalRunning() const {
#if defined(__APPLE__) || defined(__linux__)
    if (m_terminalPid <= 0) return false;
    return kill(static_cast<pid_t>(m_terminalPid), 0) == 0;
#else
    return false;
#endif
}

QString ClaudeSession::createWrapperScript() {
    QString escapedPath = QString(m_projectPath).replace("'", "'\\''");
    QString escapedPidFile = QString(m_pidFile).replace("'", "'\\''");

    // The script writes its own PID, then execs claude.
    // After exec, the PID now belongs to the claude process.
    QString content = QString(
        "#!/bin/sh\n"
        "echo $$ > '%1'\n"
        "cd '%2' && exec claude\n"
    ).arg(escapedPidFile, escapedPath);

    QFile script(m_scriptFile);
    if (!script.open(QFile::WriteOnly | QFile::Text)) {
        std::cerr << "Failed to create wrapper script: "
                  << m_scriptFile.toStdString() << std::endl;
        return m_scriptFile;
    }
    script.write(content.toUtf8());
    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    return m_scriptFile;
}

void ClaudeSession::readPidFile() {
    // Poll for the PID file (the shell needs a moment to start and write it)
    for (int i = 0; i < 30; i++) {
        QThread::msleep(100);
        QFile pidFile(m_pidFile);
        if (pidFile.exists() && pidFile.open(QFile::ReadOnly)) {
            QString pidStr = pidFile.readAll().trimmed();
            pidFile.close();
            qint64 pid = pidStr.toLongLong();
            if (pid > 0) {
                m_terminalPid = pid;
                std::cout << "Tracked terminal process (pid " << pid << ") for: "
                          << m_projectPath.toStdString() << std::endl;
                return;
            }
        }
    }
    std::cerr << "Could not read PID file for: "
              << m_projectPath.toStdString() << std::endl;
}

void ClaudeSession::launchTerminal() {
    if (isTerminalRunning()) {
        focusTerminal();
        return;
    }

    // Clean up old temp files from previous launch
    cleanupTempFiles();
    m_terminalPid = 0;

#if defined(__APPLE__)
    launchMacOS();
#elif defined(__linux__)
    launchLinux();
#endif

    if (m_terminalPid > 0) {
        m_launched = true;
    }
}

void ClaudeSession::focusTerminal() {
#if defined(__APPLE__)
    focusMacOS();
#elif defined(__linux__)
    focusLinux();
#endif
}

#if defined(__APPLE__)
void ClaudeSession::launchMacOS() {
    QString app = terminalApp();
    QString script = createWrapperScript();
    bool usePidFile = false;

    if (app == "default") {
        // Tell Terminal.app to run our wrapper script
        QStringList args;
        args << "-e"
             << QString(
                    "tell application \"Terminal\"\n"
                    "  activate\n"
                    "  do script \"%1\"\n"
                    "end tell")
                    .arg(QString(script).replace("\"", "\\\""));
        QProcess osascriptProc;
        osascriptProc.start("osascript", args);
        osascriptProc.waitForFinished(5000);
        usePidFile = true;
    } else if (app == "iterm2") {
        QStringList args;
        args << "-e"
             << QString(
                    "tell application \"iTerm2\"\n"
                    "  activate\n"
                    "  set newWindow to (create window with default profile)\n"
                    "  tell current session of newWindow\n"
                    "    write text \"%1\"\n"
                    "  end tell\n"
                    "end tell")
                    .arg(QString(script).replace("\"", "\\\""));
        QProcess osascriptProc;
        osascriptProc.start("osascript", args);
        osascriptProc.waitForFinished(5000);
        usePidFile = true;
    } else if (app == "kitty") {
        qint64 pid = 0;
        QProcess::startDetached("kitty", {"--directory", m_projectPath, script}, QString(), &pid);
        m_terminalPid = pid;
        std::cout << "Launched kitty (pid " << pid << ") for: "
                  << m_projectPath.toStdString() << std::endl;
    } else if (app == "ghostty") {
        qint64 pid = 0;
        QProcess::startDetached("ghostty", {"-e", script,
            QString("--working-directory=%1").arg(m_projectPath)}, QString(), &pid);
        m_terminalPid = pid;
        std::cout << "Launched ghostty (pid " << pid << ") for: "
                  << m_projectPath.toStdString() << std::endl;
    } else if (app == "wezterm") {
        qint64 pid = 0;
        QProcess::startDetached("wezterm", {"start", "--cwd", m_projectPath, "--", script},
            QString(), &pid);
        m_terminalPid = pid;
        std::cout << "Launched wezterm (pid " << pid << ") for: "
                  << m_projectPath.toStdString() << std::endl;
    } else {
        // Custom command
        qint64 pid = 0;
        QProcess::startDetached(app, {script}, QString(), &pid);
        m_terminalPid = pid;
        std::cout << "Launched custom terminal (pid " << pid << ") for: "
                  << m_projectPath.toStdString() << std::endl;
    }

    if (usePidFile) {
        readPidFile();
    }
}

void ClaudeSession::focusMacOS() {
    QString app = terminalApp();
    QString appName;

    if (app == "default") appName = "Terminal";
    else if (app == "iterm2") appName = "iTerm2";
    else if (app == "kitty") appName = "kitty";
    else if (app == "ghostty") appName = "Ghostty";
    else if (app == "wezterm") appName = "WezTerm";
    else appName = QFileInfo(app).baseName();

    QStringList args;
    args << "-e"
         << QString("tell application \"%1\" to activate").arg(appName);
    QProcess::startDetached("osascript", args);
}
#endif

#if defined(__linux__)
void ClaudeSession::launchLinux() {
    QString app = terminalApp();
    QString script = createWrapperScript();
    qint64 pid = 0;
    bool started = false;
    bool usePidFile = false;

    if (app == "default") {
        // Try $TERMINAL, then x-terminal-emulator, then common terminals
        QString terminal = qEnvironmentVariable("TERMINAL");
        if (terminal.isEmpty()) {
            for (const char *candidate : {"x-terminal-emulator", "gnome-terminal",
                    "konsole", "xfce4-terminal", "kitty", "alacritty", "xterm"}) {
                if (QProcess::execute("which", {candidate}) == 0) {
                    terminal = candidate;
                    break;
                }
            }
        }
        if (terminal.isEmpty()) terminal = "xterm";
        started = QProcess::startDetached(terminal, {"-e", script}, QString(), &pid);
        usePidFile = true;
    } else if (app == "kitty") {
        started = QProcess::startDetached("kitty", {"--directory", m_projectPath, script},
            QString(), &pid);
    } else if (app == "ghostty") {
        started = QProcess::startDetached("ghostty", {"-e", script,
            QString("--working-directory=%1").arg(m_projectPath)}, QString(), &pid);
    } else if (app == "wezterm") {
        started = QProcess::startDetached("wezterm", {"start", "--cwd", m_projectPath, "--", script},
            QString(), &pid);
    } else {
        started = QProcess::startDetached(app, {"-e", script}, QString(), &pid);
        usePidFile = true;
    }

    if (started) {
        if (usePidFile) {
            readPidFile();
        } else {
            m_terminalPid = pid;
        }
        std::cout << "Launched terminal (pid " << m_terminalPid << ") for: "
                  << m_projectPath.toStdString() << std::endl;
    } else {
        std::cerr << "Failed to launch terminal for: "
                  << m_projectPath.toStdString() << std::endl;
    }
}

void ClaudeSession::focusLinux() {
    // Best effort: no standard way to focus by PID on Linux
}
#endif

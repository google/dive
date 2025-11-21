/*
 Copyright 2019 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSplashScreen>
#include <QStyleFactory>
#include <QTimer>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include "dive_core/common.h"
#include "dive_core/pm4_info.h"
#include "application_controller.h"
#include "main_window.h"
#include "utils/version_info.h"
#include "custom_metatypes.h"
#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"
#ifdef __linux__
#    include <dlfcn.h>
#endif

#if defined(_WIN32)
#    include <io.h>
#else
#    include <unistd.h>
#endif

constexpr int kSplashScreenDuration = 2000;  // 2s
constexpr int kStartDelay = 500;             // 0.5s

//--------------------------------------------------------------------------------------------------
class CrashHandler
{
public:
    static void Initialize(const char *argv0)
    {
        QString filename = "dive-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") +
                           ".log.txt";

        // Try to open in the executable directory
        // This might fail if the executable folder is not writable
        QString exe_dir = QFileInfo(argv0).absolutePath();
        QString exe_full_path = QDir(exe_dir).filePath(filename);

        // If we couldn't write next to the exe (permission denied), use temp folder.
        // Windows: %TEMP%
        // Linux: /tmp
        QString temp_dir = QDir::tempPath();
        QString temp_full_path = QDir(temp_dir).filePath(filename);

        SafeStrCopy(m_primary_path, exe_full_path.toLocal8Bit().constData());
        SafeStrCopy(m_fallback_path, temp_full_path.toLocal8Bit().constData());

        std::cout << "Crash handler initialized" << std::endl;
        std::cout << "  1. Primary Log Path:  " << exe_full_path.toStdString() << std::endl;
        std::cout << "  2. Fallback Log Path: " << temp_full_path.toStdString() << std::endl;
    }

    static void Writer(const char *data)
    {
        if (data == nullptr)
        {
            return;
        }

        // Avoid strlen for crash handler to be safe
        uint32_t len = 0;
        while (data[len] != '\0')
        {
            len++;
        }

        if (m_fd == kInvalidFd)
        {
            m_fd = SysOpen(m_primary_path);

            if (m_fd == kInvalidFd)
            {
                m_fd = SysOpen(m_fallback_path);
            }
        }

        if (m_fd != kInvalidFd)
        {
            SysWrite(m_fd, data, len);
        }
    }

private:
    static constexpr int kInvalidFd = -1;
    static constexpr int kMaxPath = 2048;

    inline static int m_fd = kInvalidFd;

    // Use char array to avoid potential allocation within the crash handler
    inline static char m_primary_path[kMaxPath] = { 0 };
    inline static char m_fallback_path[kMaxPath] = { 0 };

    template<size_t N> static void SafeStrCopy(char (&dest)[N], const char *src)
    {
        if (!src)
        {
            return;
        }

        size_t i = 0;
        for (; i < N - 1 && src[i] != '\0'; ++i)
        {
            dest[i] = src[i];
        }
        dest[i] = '\0';
    }

    static int SysOpen(const char *path)
    {
#if defined(_WIN32)
        constexpr int flags = _O_CREAT | _O_TRUNC | _O_WRONLY | _O_TEXT;
        constexpr int mode = _S_IREAD | _S_IWRITE;
        return _open(path, flags, mode);
#else
        constexpr int flags = O_CREAT | O_TRUNC | O_WRONLY;
        // 0: Indicates this is an octal number
        // 6: (Owner):  Read (4) + Write (2) = Read/Write
        // 6: (Group):  Read (4) + Write (2) = Read/Write
        // 4: (Others): Read (4) = Read Only
        constexpr int mode = 0664;
        return open(path, flags, mode);
#endif
    }

    static void SysWrite(int fd, const char *data, uint32_t len)
    {
#if defined(_WIN32)
        _write(fd, data, len);
#else
        [[maybe_unused]] ssize_t res = write(fd, data, len);
#endif
    }
};

//--------------------------------------------------------------------------------------------------
bool SetApplicationStyle(QString style_key)
{
    QStringList style_list = QStyleFactory::keys();
    for (uint32_t i = 0; i < (uint32_t)style_list.size(); ++i)
    {
        if (style_list[i] == style_key)
        {
            QApplication::setStyle(QStyleFactory::create(style_key));
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
void SetDarkMode(QApplication &app)
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(40, 40, 40));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    darkPalette.setColor(QPalette::Disabled, QPalette::Window, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(160, 160, 160));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::Button, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(160, 160, 160));
    darkPalette.setColor(QPalette::Disabled, QPalette::Link, QColor(160, 160, 160));
    darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::Light, QColor(53, 53, 53));

    QApplication::setPalette(darkPalette);
}

//--------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    absl::InitializeSymbolizer(argv[0]);

    CrashHandler::Initialize(argv[0]);

    absl::FailureSignalHandlerOptions options;
    options.writerfn = CrashHandler::Writer;
    absl::InstallFailureSignalHandler(options);

    // Check number of arguments
    bool exit_after_load = false;
    if (argc > 1 && strcmp(argv[1], "--exit-after-load") == 0)
    {
        exit_after_load = true;
        argc--;
        argv++;
    }
    if (argc != 1 && argc != 2)
        return 0;

    // Print version info if asked to on the command line.
    // This will only work on linux as we are a UI app on Windows.
    // On Windows, users can right-click the .exe and look at Properties/Details.
    if (argc > 1 && strcasecmp(argv[1], "--version") == 0)
    {
        std::cout << Dive::GetDiveDescription() << std::endl;
        return 0;
    }

    // Optional command arg loading method for fast iteration
    // Note: Set the style *before* QApplication constructor. This allows commandline
    // "-style <style>" style override to still work properly.

    // Try setting "Fusion" style. If not found, set "Windows".
    // And if that's not found, default to whatever style the factory provides.
    if (!SetApplicationStyle("Fusion"))
    {
        if (!SetApplicationStyle("Windows"))
        {
            if (!QStyleFactory::keys().empty())
            {
                SetApplicationStyle(QStyleFactory::keys()[0]);
            }
        }
    }

    Dive::RegisterCustomMetaType();

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/images/dive.ico"));
    SetDarkMode(app);

    // Load and apply the style sheet
    QFile style_sheet(":/stylesheet.qss");
    style_sheet.open(QFile::ReadOnly);
    QString style(style_sheet.readAll());
    app.setStyleSheet(style);

    // Display splash screen
    QSplashScreen *splash_screen = new QSplashScreen();
    splash_screen->setPixmap(QPixmap(":/images/dive.png"));
    splash_screen->show();

    // Initialize packet info query data structures needed for parsing
    Pm4InfoInit();

    ApplicationController controller;
    MainWindow           *main_window = new MainWindow(controller);
    if (exit_after_load)
    {
        QObject::connect(main_window, &MainWindow::FileLoaded, main_window, &MainWindow::close);
    }

    if (!main_window->InitializePlugins())
    {
        qDebug()
        << "Application: Plugin initialization failed. Application may proceed without plugins.";
    }

    if (argc == 2)
    {
        // This is executed async.
        main_window->LoadFile(argv[1], false, true);
    }

    QTimer::singleShot(kSplashScreenDuration, splash_screen, SLOT(close()));
    QTimer::singleShot(kStartDelay, main_window, SLOT(show()));

    return app.exec();
}

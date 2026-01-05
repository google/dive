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

#include <fcntl.h>

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>
#include <QSplashScreen>
#include <QStyleFactory>
#include <QTimer>
#include <cstdio>
#include <filesystem>
#include <iostream>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "application_controller.h"
#include "custom_metatypes.h"
#include "dive/os/terminal.h"
#include "dive/utils/version_info.h"
#include "dive_core/common.h"
#include "dive_core/pm4_info.h"
#include "main_window.h"
#include "ui/application_controller.h"
#include "ui/custom_metatypes.h"
#include "ui/dive_application.h"
#include "ui/main_window.h"
#include "utils/version_info.h"
#ifdef __linux__
#include <dlfcn.h>
#endif

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

constexpr int kSplashScreenDuration = 2000;  // 2s
constexpr int kStartDelay = 500;             // 0.5s
constexpr int kScreenshotDelay = 5000;       // 5s

ABSL_FLAG(std::string, test_output, "", "Output directory for tests.");
ABSL_FLAG(std::string, test_prefix, "", "Filename prefix for tests.");
ABSL_FLAG(std::string, test_scenario, "", "Execute test scenario.");

ABSL_FLAG(bool, native_style, false, "Use system provided style");
ABSL_FLAG(bool, maximize, false, "Launch application maximized");

// QApplication flags:
ABSL_RETIRED_FLAG(std::string, style, "", "Set the application GUI style");
ABSL_RETIRED_FLAG(std::string, stylesheet, "", "Set the application stylesheet");
ABSL_RETIRED_FLAG(bool, widgetcount, false, "Qt flag widgetcount");
ABSL_RETIRED_FLAG(bool, reverse, false, "Qt flag reverse");
ABSL_RETIRED_FLAG(std::string, qmljsdebugger, "", "Qt flag qmljsdebugger");

//--------------------------------------------------------------------------------------------------
class CrashHandler
{
 public:
    static void Initialize(const char* argv0)
    {
        QString filename =
            "dive-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".log.txt";

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

    static void Writer(const char* data)
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
    inline static char m_primary_path[kMaxPath] = {0};
    inline static char m_fallback_path[kMaxPath] = {0};

    template <size_t N>
    static void SafeStrCopy(char (&dest)[N], const char* src)
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

    static int SysOpen(const char* path)
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

    static void SysWrite(int fd, const char* data, uint32_t len)
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
auto SetupFlags(int argc, char** argv)
{
    absl::FlagsUsageConfig flags_usage_config;
    flags_usage_config.version_string = Dive::GetCompleteVersionString;
    absl::SetFlagsUsageConfig(flags_usage_config);
    absl::SetProgramUsageMessage("Dive GPU Profiler GUI");
    return absl::ParseCommandLine(argc, argv);
}

//--------------------------------------------------------------------------------------------------
std::optional<std::filesystem::path> GetTestSavePath(std::string_view filename)
{
    if (absl::GetFlag(FLAGS_test_output).empty())
    {
        return std::nullopt;
    }
    std::filesystem::path output_dir(absl::GetFlag(FLAGS_test_output));
    std::filesystem::create_directories(output_dir);
    if (!std::filesystem::is_directory(output_dir))
    {
        return std::nullopt;
    }
    std::string full_filename = absl::GetFlag(FLAGS_test_prefix);
    full_filename += filename;
    return output_dir / full_filename;
}

//--------------------------------------------------------------------------------------------------
bool ExecuteScenario(std::string_view scenario, MainWindow* main_window)
{
    if (scenario == "exit-after-load")
    {
        QObject::connect(main_window, &MainWindow::FileLoaded, main_window, &MainWindow::close);
        return true;
    }

    if (scenario == "screenshot")
    {
        auto savepath = GetTestSavePath("MainWindow.png");
        if (!savepath)
        {
            qDebug() << "Invalid screenshot path";
            return false;
        }
        auto take_screenshot = [main_window, savepath]() {
            QPixmap pixmap(main_window->size());
            main_window->render(&pixmap);
            pixmap.save(QString::fromStdString(savepath->string()));
            main_window->close();
        };
        QObject::connect(main_window, &MainWindow::FileLoaded, main_window,
                         [take_screenshot, main_window]() {
                             QTimer::singleShot(kScreenshotDelay, main_window, take_screenshot);
                         });
        return true;
    }

    qDebug() << "Test scenario " << QString::fromStdString(std::string(scenario)) << " not found";
    return false;
}

//--------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    Dive::AttachToTerminalOutputIfAvailable();
    std::vector<char*> positional_args = SetupFlags(argc, argv);

    absl::InitializeSymbolizer(argv[0]);

    CrashHandler::Initialize(argv[0]);

    absl::FailureSignalHandlerOptions options;
    options.writerfn = CrashHandler::Writer;
    absl::InstallFailureSignalHandler(options);

    const bool native_style = absl::GetFlag(FLAGS_native_style);
    if (!native_style)
    {
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
    }

    Dive::RegisterCustomMetaType();

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QScopedPointer<DiveApplication> app{new DiveApplication(argc, argv)};
    app->setWindowIcon(QIcon(":/images/dive.ico"));

    if (!native_style)
    {
        app->ApplyCustomStyle();
    }

    // Display splash screen
    QSplashScreen* splash_screen = new QSplashScreen();
    splash_screen->setPixmap(QPixmap(":/images/dive.png"));
    splash_screen->show();

    // Initialize packet info query data structures needed for parsing
    Pm4InfoInit();

    QScopedPointer<MainWindow> main_window{new MainWindow(app->GetController())};

    if (auto scenario = absl::GetFlag(FLAGS_test_scenario); !scenario.empty())
    {
        if (!ExecuteScenario(scenario, main_window.get()))
        {
            return EXIT_FAILURE;
        }
    }

    if (!app->GetController().InitializePlugins())
    {
        qDebug() << "Application: Plugin initialization failed. Application may proceed without "
                    "plugins.";
    }

    if (positional_args.size() == 2)
    {
        // This is executed async.
        main_window->LoadFile(positional_args.back(), false, true);
    }

    QTimer::singleShot(kSplashScreenDuration, splash_screen, SLOT(close()));

    if (absl::GetFlag(FLAGS_maximize))
    {
        QTimer::singleShot(kStartDelay, main_window.get(), &MainWindow::showMaximized);
    }
    else
    {
        QTimer::singleShot(kStartDelay, main_window.get(), &MainWindow::show);
    }

    return app->exec();
}

/*
 Copyright 2026 Google LLC

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

#include <gtest/gtest.h>

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QWidget>
#include <filesystem>

#include "dive/plugin/abi/idive_plugin.h"

// To run headless, use either the minimal or offscreen QPA:
//
//     QT_QPA_PLATFORM=offscreen ./build/pkg/host/dive
//
// - minimal doesn't do font rendering
// - offscreen uses font that's too big
//
// On Linux over SSH, consider using Xvfb
//
//     Xvfb :99 -screen 0 1024x768x24 &
//     DISPLAY=:99 ./build/pkg/host/dive

namespace Dive
{
namespace
{

template <typename T>
T FindTopLevelWidget(const QString& name)
{
    for (QWidget* widget : QApplication::topLevelWidgets())
    {
        if (widget->objectName() != name)
        {
            continue;
        }

        return qobject_cast<T>(widget);
    }
    return nullptr;
}

bool TakeScreenshot(QWidget* widget, const std::filesystem::path& file)
{
    QPixmap pixmap(widget->size());
    widget->render(&pixmap);
    return pixmap.save(QString::fromStdString(file.string()));
};

// When loaded, schedules the start of a gTest run after the program has finished initialization.
// This acts as a bridge between plugin loading and gtest execution given that we don't have access
// to main.
class AutomatedTestPlugin : public IDivePlugin
{
 public:
    ~AutomatedTestPlugin() override = default;

    std::string PluginName() const override { return "Automated Test"; }
    std::string PluginVersion() const override { return "1.0.0"; }

    bool Initialize(IDivePluginBridge& /*bridge*/) override
    {
        qDebug() << "AutomatedTestPlugin::Initialize";
        auto* main_window = FindTopLevelWidget<QWidget*>("MainWindow");

        // Plugin initialization is done before the main event loop starts. To ensure initialization
        // has complete, defer starting tests until the main event loop is running.
        // TODO: Why does qWaitForWindowActive break if I don't wait ~1000ms?
        qDebug() << "Scheduling test runs";
        QTimer::singleShot(1000, [] {
            qDebug() << "Running tests!";
            int argc = 1;
            std::array<char*, 1> argv = {"foo"};  // TODO bad!!
            testing::InitGoogleTest(&argc, argv.data());
            qApp->exit(RUN_ALL_TESTS());
        });

        qDebug() << "AutomatedTestPlugin::Initialize done";
        return true;
    }

    void Shutdown() override {}
};

TEST(DiveUiTest, CanCaptureGfxrWithTraceDialog)
{
    // Find the primary Dive UI window. Confusingly, MainWindow is a top-level widget (not a
    // top-level window).
    auto* main_window = FindTopLevelWidget<QWidget*>("MainWindow");
    ASSERT_NE(main_window, nullptr);
    ASSERT_TRUE(QTest::qWaitForWindowActive(main_window));
    EXPECT_TRUE(TakeScreenshot(main_window, "main_window.png"));

    // Open the Capture dialog by using the F5 keyboard shortcut
    auto* trace_dialog = main_window->findChild<QDialog*>("TraceDialog");
    ASSERT_NE(trace_dialog, nullptr);

    QTest::keyClick(main_window, Qt::Key_F5);
    ASSERT_TRUE(QTest::qWaitForWindowActive(trace_dialog));

    // Enter in the package name in the Executable text field. Confusingly, using the package combo
    // box doesn't work the same way.
    auto* executable_line_edit = trace_dialog->findChild<QLineEdit*>("Executable LineEdit");
    ASSERT_NE(executable_line_edit, nullptr);

    QTest::keyClicks(executable_line_edit, "com.google.bigwheels.project_cube_xr.debug");
    // TODO: Verify that the package is an element in the combo box.
    // The test hogs the main thread so we need to explicitly post any events.
    QCoreApplication::sendPostedEvents(executable_line_edit);

    // Click the "Start Application" button and wait for the app to start
    auto* start_application = trace_dialog->findChild<QPushButton*>("Start Application");
    ASSERT_NE(start_application, nullptr);
    ASSERT_TRUE(start_application->isEnabled());
    ASSERT_EQ(start_application->text(), QString("&Start Application"));

    // I can't seem to get QTest::mouseClick to work so just emit the click signal.
    start_application->click();
    ASSERT_TRUE(QTest::qWaitFor(
        [start_application] { return start_application->text() == "&Stop Application"; }));

    // Click the "Start GFXR Capture" button and wait for capture to complete
    auto* capture_button = trace_dialog->findChild<QPushButton*>("GFXR Capture Button");
    ASSERT_NE(capture_button, nullptr);
    ASSERT_TRUE(capture_button->isEnabled());
    ASSERT_EQ(capture_button->text(), QString("&Start GFXR Capture"));

    // The capture and retrieve buttons are the same, just the text changes.
    capture_button->click();
    ASSERT_TRUE(QTest::qWaitFor(
        [capture_button] { return capture_button->text() == "&Retrieve GFXR Capture"; }));

    // Click the "Retrieve GFXR Capture" button and wait for the message box with the status.
    // TODO QProgressDialog?
    capture_button->click();
    QMessageBox* message_box = nullptr;
    ASSERT_TRUE(QTest::qWaitFor([trace_dialog, &message_box] {
        message_box = trace_dialog->findChild<QMessageBox*>("TraceDialog MessageBox");
        return message_box != nullptr;
    }));

    ASSERT_TRUE(message_box->text().contains("Capture successfully saved at"));
    EXPECT_TRUE(TakeScreenshot(message_box, "message_box.png"));

    message_box->close();
    QCoreApplication::sendPostedEvents(message_box);

    // After retrieving the capture, the button should revert back to "Start GFXR Capture"
    ASSERT_TRUE(QTest::qWaitFor(
        [capture_button] { return capture_button->text() == "&Start GFXR Capture"; }));
    // After the capture is retrieved, it is loaded by the UI. During capturing loading, the entire
    // UI is disabled. We have to wait for loading to complete, signified by the button becoming
    // enabled again. Qt won't warn us if we try to click() a disabled button.
    ASSERT_TRUE(QTest::qWaitFor([start_application] {
        // The start and stop buttons are the same, just the text changes.
        return start_application->isEnabled();
    }));

    // Click the "Stop Application" button and wait for the app to stop.
    start_application->click();
    ASSERT_TRUE(QTest::qWaitFor(
        [start_application] { return start_application->text() == "&Start Application"; }));

    // Close the capture dialog and wait for it to close.
    trace_dialog->close();
    QCoreApplication::sendPostedEvents(trace_dialog);
    ASSERT_TRUE(QTest::qWaitFor([trace_dialog] { return !trace_dialog->isVisible(); }));
}

}  // namespace

extern "C" DIVE_PLUGIN_EXPORT IDivePlugin* CreateDivePluginInstance()
{
    return new AutomatedTestPlugin();
}
}  // namespace Dive

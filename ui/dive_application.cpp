/*
 Copyright 2025 Google LLC

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

#include "ui/dive_application.h"

#include <QDebug>
#include <QFile>
#include <QPalette>
#include <QScopedValueRollback>
#include <QString>

#include "ui/application_controller.h"

namespace
{

QPalette GetDarkPalette()
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

    return darkPalette;
}

}  // namespace

struct DiveApplication::Impl
{
    ApplicationController m_controller;

    std::optional<QString> m_style_sheet;
};

DiveApplication::DiveApplication(int& argc, char** argv) : QApplication(argc, argv) {}

DiveApplication::~DiveApplication()
{
    // For m_impl.~ImplPointer()
}

ApplicationController& DiveApplication::GetController() { return m_impl->m_controller; }

void DiveApplication::ApplyCustomStyle()
{
    QApplication::setPalette(GetDarkPalette());

    QFile style_sheet(":/stylesheet.qss");
    style_sheet.open(QFile::ReadOnly);
    m_impl->m_style_sheet = style_sheet.readAll();
    setStyleSheet(*m_impl->m_style_sheet);
}

bool DiveApplication::event(QEvent* e)
{
    if (e->type() == QEvent::ApplicationPaletteChange)
    {
        // Make sure we don't recursively calling setPalette.
        // Note event handling only happen on the UI thread.
        static bool guard = false;
        if (!guard && m_impl->m_style_sheet)
        {
            QScopedValueRollback guard_scope(guard, true);
            // Re-apply custom style.
            QApplication::setPalette(GetDarkPalette());
            setStyleSheet(*m_impl->m_style_sheet);
        }
    }
    return QApplication::event(e);
}

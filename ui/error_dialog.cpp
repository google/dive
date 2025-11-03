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

#include "error_dialog.h"

#include <QMessageBox>
#include <QString>

#include "capture_file_manager.h"

namespace
{

void OpenErrorMessageBox(QWidget* parent, const QString& title, const QString& text)
{
    auto message_box = new QMessageBox(parent);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setIcon(QMessageBox::Icon::Critical);
    message_box->setWindowTitle(title);
    message_box->setText(text);
    message_box->open();
}

void OpenErrorMessageBox(QWidget* parent, const LoadFileResult& result)
{
    QString title = QString("Unable to open file: ") + result.reference.ToQString();
    switch (result.status)
    {
    case LoadFileResult::Status::kSuccess:
        break;
    case LoadFileResult::Status::kUnknown:
        OpenErrorMessageBox(parent, title, QString());
        break;
    case LoadFileResult::Status::kFileIoError:
        OpenErrorMessageBox(parent, title, QString("File I/O error!"));
        break;
    case LoadFileResult::Status::kCorruptData:
        OpenErrorMessageBox(parent, title, QString("File corrupt!"));
        break;
    case LoadFileResult::Status::kVersionError:
        OpenErrorMessageBox(parent, title, QString("Incompatible version!"));
        break;
    case LoadFileResult::Status::kParseFailure:
        OpenErrorMessageBox(parent, title, QString("Error parsing file!"));
        break;
    case LoadFileResult::Status::kUnsupportedFile:
        OpenErrorMessageBox(parent, title, QString("File type not supported!"));
        break;
    case LoadFileResult::Status::kGfxaAssetMissing:
        OpenErrorMessageBox(parent,
                            title,
                            QString("Required .gfxa file: %1 not found!")
                            .arg(QString::fromStdString(result.components.gfxa.string())));
        break;
    }
}

}  // namespace

ErrorDialog::ErrorDialog(QWidget* parent) :
    QObject(parent),
    m_parent(parent)
{
}

void ErrorDialog::OnLoadingFailure(const LoadFileResult& result) const
{
    OpenErrorMessageBox(m_parent, result);
}

/*
 Copyright 2021 Google LLC

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
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "dive_core/data_core.h"

#include "text_file_view.h"

// -------------------------------------------------------------------------------------------------
static bool IsEmbeddedTextASCII(const char *s, size_t size)
{
    if (size > 0 && s[size - 1] == 0)
    {
        // Ignore the null terminiator.
        size = size - 1;
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (!(std::isprint(s[i]) || std::isspace(s[i])))
            return false;
    }
    return true;
}

static std::string ToHexDump(const char *s, size_t size)
{
    constexpr size_t chunk_size = 16;

    std::stringstream sst;
    for (size_t offset = 0; offset < size; offset += chunk_size)
    {
        sst << std::hex << std::setfill('0');
        sst << std::setw(8) << offset << ": ";
        char   ascii_part[chunk_size + 1] = {};
        size_t cutoff = std::min(size - offset, chunk_size);
        for (size_t i = 0; i < cutoff; ++i)
        {
            char c = s[offset + i];
            sst << std::setw(2) << (static_cast<uint32_t>(c) & 0xffu) << " ";
            ascii_part[i] = (std::isprint(c) ? c : '.');
        }
        if (chunk_size > cutoff)
        {
            sst << std::setfill(' ') << std::setw((chunk_size - cutoff) * 3) << ""
                << std::setfill('0');
        }

        sst << ascii_part << std::endl;
    }
    return sst.str();
}

// -------------------------------------------------------------------------------------------------
class TextFileWidgetItem : public QTreeWidgetItem
{
public:
    typedef decltype(std::declval<Dive::Pm4CaptureData>().GetNumText()) IndexType;

    TextFileWidgetItem(std::string name, IndexType index, QTreeWidget *view) :
        QTreeWidgetItem(view),
        m_name(name),
        m_index(index)
    {
    }
    std::string GetName() const { return m_name; }
    IndexType   GetIndex() const { return m_index; }

private:
    std::string m_name;
    IndexType   m_index;
};

// -------------------------------------------------------------------------------------------------
TextFileView::TextFileView(const Dive::DataCore &data_core) :
    m_data_core(data_core)
{
    QVBoxLayout *layout = new QVBoxLayout();
    m_text_list = new QTreeWidget();
    m_text_list->setColumnCount(3);
    m_text_list->setHeaderLabels(QStringList() << "Filename"
                                               << "Size"
                                               << "Type");
    m_text = new QPlainTextEdit();
    m_text->setReadOnly(true);
    // Hex dumps need monospace to look remotely readable
    QFont monospace("monospace");
    monospace.setStyleHint(QFont::Monospace);
    m_text->document()->setDefaultFont(monospace);

    layout->addWidget(m_text_list);
    layout->setStretchFactor(m_text_list, 1);
    layout->addWidget(m_text);
    layout->setStretchFactor(m_text, 5);
    setLayout(layout);

    QObject::connect(m_text_list,
                     SIGNAL(itemSelectionChanged()),
                     this,
                     SLOT(OnFileSelectionChanged()));
}

//--------------------------------------------------------------------------------------------------
void TextFileView::OnFileLoaded()
{
    Reset();
    const auto &capture = m_data_core.GetPm4CaptureData();

    if (!capture.GetRegisterInfo().GetRegisters().empty())
    {
        TextFileWidgetItem *
        treeItem = new TextFileWidgetItem("registers",
                                          std::numeric_limits<TextFileWidgetItem::IndexType>::max(),
                                          m_text_list);

        // Filename
        treeItem->setText(0, tr("registers"));
        // Size
        treeItem->setText(1, tr("N/A"));
        // Type
        treeItem->setText(2, tr("Structured"));
    }

    TextFileWidgetItem::IndexType num_text = capture.GetNumText();
    for (TextFileWidgetItem::IndexType i = 0; i < num_text; ++i)
    {
        const auto &text = capture.GetText(i);

        TextFileWidgetItem *treeItem = new TextFileWidgetItem(text.GetName(), i, m_text_list);

        // Filename
        treeItem->setText(0, QString::fromStdString(text.GetName()));
        // Size
        treeItem->setText(1, QString::number(text.GetSize()));
        // Type
        if (IsEmbeddedTextASCII(text.GetText(), text.GetSize()))
        {
            treeItem->setText(2, tr("Plain Text"));
        }
        else
        {
            treeItem->setText(2, tr("Binary"));
        }
    }
}

void TextFileView::Reset()
{
    m_text_list->clear();
    m_text->clear();
}

//--------------------------------------------------------------------------------------------------
void TextFileView::OnFileSelectionChanged()
{
    const TextFileWidgetItem *item_ptr = static_cast<const TextFileWidgetItem *>(
    m_text_list->currentItem());

    const auto &capture = m_data_core.GetPm4CaptureData();
    std::string text_data;
    if (item_ptr->GetIndex() < capture.GetNumText())
    {
        const auto &text = capture.GetText(item_ptr->GetIndex());
        if (IsEmbeddedTextASCII(text.GetText(), text.GetSize()))
        {
            text_data = std::string(text.GetText(), text.GetText() + text.GetSize());
            if (!text_data.empty() && text_data.back() == 0)
            {
                // Remove the null terminator.
                text_data.pop_back();
            }
        }
        else
        {
            text_data = ToHexDump(text.GetText(), text.GetSize());
        }
    }
    else if (item_ptr->GetName() == "registers")
    {
        std::stringstream sst;
        sst << std::hex;
        for (auto &kv : capture.GetRegisterInfo().GetRegisters())
        {
            sst << kv.first << ": 0x" << kv.second << "\n";
        }
        text_data = sst.str();
    }
    else if (item_ptr->GetName() == "wavestate")
    {
        std::stringstream sst;
        text_data = sst.str();
    }

    m_text->setPlainText(QString::fromStdString(text_data));
    m_text->moveCursor(QTextCursor::Start);
    m_text->setReadOnly(true);
}

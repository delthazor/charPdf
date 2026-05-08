#include "StringListEditor.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace CharEditor
{

StringListEditor::StringListEditor(Getter getterParam, Setter setterParam, Mode modeParam, QWidget* parent)
    : QWidget(parent), getter(std::move(getterParam)), setter(std::move(setterParam)), mode(modeParam)
{
    QVBoxLayout* root = new QVBoxLayout();
    setLayout(root);

    list = new QListWidget();
    root->addWidget(list, 1);

    QHBoxLayout* controls = new QHBoxLayout();
    root->addLayout(controls);

    input = new QLineEdit();
    controls->addWidget(input, 1);

    addBtn = new QPushButton("Add");
    controls->addWidget(addBtn);

    if (mode == Mode::SelectRowToEdit)
    {
        newItemBtn = new QPushButton("New item");
        controls->addWidget(newItemBtn);
        QObject::connect(newItemBtn, &QPushButton::clicked, this, &StringListEditor::EnterAddNewMode);
        QObject::connect(list, &QListWidget::currentRowChanged, this, &StringListEditor::OnListCurrentRowChanged);
    }

    removeBtn = new QPushButton("Remove");
    controls->addWidget(removeBtn);

    QObject::connect(addBtn, &QPushButton::clicked, this, &StringListEditor::CommitFromInput);
    QObject::connect(removeBtn, &QPushButton::clicked, this, &StringListEditor::RemoveSelected);
    QObject::connect(input, &QLineEdit::returnPressed, this, &StringListEditor::CommitFromInput);

    UpdatePrimaryButtonLabel();
}

void StringListEditor::UpdatePrimaryButtonLabel()
{
    if (!addBtn) { return; }
    if (mode != Mode::SelectRowToEdit)
    {
        addBtn->setText("Add");
        return;
    }
    addBtn->setText(list && list->currentRow() >= 0 ? "Save" : "Add");
}

void StringListEditor::OnListCurrentRowChanged(int row)
{
    if (mode != Mode::SelectRowToEdit) { return; }
    if (row < 0 || !list)
    {
        UpdatePrimaryButtonLabel();
        return;
    }
    QListWidgetItem* it = list->item(row);
    if (it)
    {
        const QSignalBlocker b(input);
        input->setText(it->text());
    }
    UpdatePrimaryButtonLabel();
}

void StringListEditor::EnterAddNewMode()
{
    if (mode != Mode::SelectRowToEdit || !list) { return; }
    list->clearSelection();
    list->setCurrentRow(-1);
    input->clear();
    UpdatePrimaryButtonLabel();
}

void StringListEditor::Refresh()
{
    const int prevRow = list ? list->currentRow() : -1;

    list->clear();
    const auto items = getter ? getter() : std::vector<std::string>();
    for (const auto& s : items)
    {
        list->addItem(QString::fromStdString(s));
    }

    if (mode == Mode::SelectRowToEdit && list)
    {
        if (prevRow >= 0 && prevRow < list->count())
        {
            list->setCurrentRow(prevRow);
        }
        else
        {
            list->clearSelection();
            list->setCurrentRow(-1);
            input->clear();
            UpdatePrimaryButtonLabel();
        }
    }
}

void StringListEditor::CommitFromInput()
{
    if (!setter || !getter) { return; }
    const QString text = input->text().trimmed();
    if (text.isEmpty()) { return; }

    auto items = getter();

    if (mode == Mode::SelectRowToEdit)
    {
        const int row = list ? list->currentRow() : -1;
        if (row >= 0 && static_cast<size_t>(row) < items.size())
        {
            items[static_cast<size_t>(row)] = text.toStdString();
            setter(items);
            Refresh();
            return;
        }
    }

    items.push_back(text.toStdString());
    setter(items);
    input->clear();
    if (mode == Mode::SelectRowToEdit) { EnterAddNewMode(); }
    Refresh();
}

void StringListEditor::RemoveSelected()
{
    if (!setter || !getter) { return; }
    const int row = list->currentRow();
    if (row < 0) { return; }

    auto items = getter();
    if (static_cast<size_t>(row) >= items.size()) { return; }
    items.erase(items.begin() + row);
    setter(items);
    if (mode == Mode::SelectRowToEdit) { EnterAddNewMode(); }
    Refresh();
}

}


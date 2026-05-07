#include "StringListEditor.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace CharEditor
{

StringListEditor::StringListEditor(Getter getterParam, Setter setterParam, QWidget* parent)
    : QWidget(parent), getter(std::move(getterParam)), setter(std::move(setterParam))
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

    removeBtn = new QPushButton("Remove");
    controls->addWidget(removeBtn);

    QObject::connect(addBtn, &QPushButton::clicked, this, &StringListEditor::AddFromInput);
    QObject::connect(removeBtn, &QPushButton::clicked, this, &StringListEditor::RemoveSelected);
    QObject::connect(input, &QLineEdit::returnPressed, this, &StringListEditor::AddFromInput);
}

void StringListEditor::Refresh()
{
    list->clear();
    const auto items = getter ? getter() : std::vector<std::string>();
    for (const auto& s : items)
    {
        list->addItem(QString::fromStdString(s));
    }
}

void StringListEditor::AddFromInput()
{
    if (!setter || !getter) { return; }
    const QString text = input->text().trimmed();
    if (text.isEmpty()) { return; }

    auto items = getter();
    items.push_back(text.toStdString());
    setter(items);
    input->clear();
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
    Refresh();
}

}


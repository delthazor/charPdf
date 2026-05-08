#pragma once

#include <QWidget>

#include <functional>
#include <string>
#include <vector>

class QListWidget;
class QLineEdit;
class QPushButton;

namespace CharEditor
{

class StringListEditor : public QWidget
{
  public:
    using Getter = std::function<std::vector<std::string>()>;
    using Setter = std::function<void(const std::vector<std::string>&)>;

    enum class Mode
    {
        AppendOnly,
        SelectRowToEdit
    };

    StringListEditor(Getter getter, Setter setter, Mode mode = Mode::AppendOnly, QWidget* parent = nullptr);

    void Refresh();

  private:
    void CommitFromInput();
    void RemoveSelected();
    void OnListCurrentRowChanged(int row);
    void EnterAddNewMode();

    void UpdatePrimaryButtonLabel();

    Getter getter;
    Setter setter;
    Mode mode = Mode::AppendOnly;

    QListWidget* list = nullptr;
    QLineEdit* input = nullptr;
    QPushButton* addBtn = nullptr;
    QPushButton* newItemBtn = nullptr;
    QPushButton* removeBtn = nullptr;
};

}


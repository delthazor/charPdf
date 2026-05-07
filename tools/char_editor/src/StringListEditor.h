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

    StringListEditor(Getter getter, Setter setter, QWidget* parent = nullptr);

    void Refresh();

  private:
    void AddFromInput();
    void RemoveSelected();

    Getter getter;
    Setter setter;

    QListWidget* list = nullptr;
    QLineEdit* input = nullptr;
    QPushButton* addBtn = nullptr;
    QPushButton* removeBtn = nullptr;
};

}


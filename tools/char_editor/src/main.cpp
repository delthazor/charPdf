#include <QApplication>

#include <string>

#include "MainWindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    CharEditor::MainWindow w(CharEditor::CharacterRepository(std::string("assets/cfg")));
    w.show();
    return app.exec();
}


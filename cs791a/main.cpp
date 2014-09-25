#include <QApplication>
#include "engine.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Engine e(argc, argv);

    return a.exec();
}

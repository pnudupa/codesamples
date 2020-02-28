#include <QApplication>

#include "shadowrenderwindow.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    ShadowRenderWindow renderWindow;
//    SimpleRenderWindow renderWindow;
    renderWindow.resize(600, 600);
    renderWindow.show();

    return a.exec();
}

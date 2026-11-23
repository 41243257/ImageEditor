#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ImageEditor"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/images/editor.png"))); // 視窗左上角與工作列的圖示

    MainWindow window;
    window.show();

    return app.exec();
}

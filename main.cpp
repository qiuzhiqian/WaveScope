#include "wavescope.h"
#include <QApplication>

/*
 * Author:qiuzhiqian
 * Email:xia_mengliang@163.com
 * Date:2017.07.15
 */

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WaveScope w;
    w.show();

    return a.exec();
}

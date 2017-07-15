#ifndef WAVESCOPE_H
#define WAVESCOPE_H

#include <QMainWindow>

#include <QChart>

using namespace QtCharts;

#include <QtCharts/QChartGlobal>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QValueAxis>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QTimer>

#include <QTextEdit>

/*
 * Author:qiuzhiqian
 * Email:xia_mengliang@163.com
 * Date:2017.07.15
 */

typedef QList<QPointF> DataList;

namespace Ui {
class WaveScope;
}

class WaveScope : public QMainWindow
{
    Q_OBJECT

public:
    explicit WaveScope(QWidget *parent = 0);
    ~WaveScope();

    void UpadteChart();
    QChart *createChart(quint16 cnt);

    void PointCntChange(quint16 cnt);

    //串口部分
    void Get_Serial();
    void Set_Serial();

    qreal getData();

private slots:
    void slt_open_serial();
    void slt_read_serial();
    void slt_time_timeout();

    void slt_test();

    void slt_change_period();
    void slt_change_xcnt();

private:
    Ui::WaveScope *ui;

    quint16 xcnt;
    quint16 accindex;

    QChart *m_chart;
    QLineSeries *m_series;
    QChartView *chartView;

    DataList m_list;

    qreal ymin=0;
    qreal ymax=0;

    void LeftShift(DataList *t_list, int cnt=0);
    void RightShift(DataList *t_list, int cnt=0);

    QSerialPort *m_serial;


    quint8 serial_data[8]={0};
    quint8 DataFlag=0;

    quint8 pro_type=0;     //协议模式,=0调试模式，=1图显模式

    quint16 wave_period;       //刷新周期
    QTimer *wave_time;

    QTextEdit *te_debug_show;
    QString debug_string;
};

#endif // WAVESCOPE_H

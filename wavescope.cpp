#include "wavescope.h"
#include "ui_wavescope.h"
#include <QDebug>

/*
 * Author:qiuzhiqian
 * Email:xia_mengliang@163.com
 * Date:2017.07.15
 */

//时间轴移动策略：判断是否有可用数据，如果有则添加到末尾，然后数值左移1，然后判断第一位是否为零，为零就移除

WaveScope::WaveScope(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WaveScope)
{
    ui->setupUi(this);
    QVBoxLayout *centerLayout=(QVBoxLayout *)(ui->centralWidget->layout());

    //xcnt=800;
    //wave_period=5;
    QByteArray ba=ui->le_showcnt->text().toLatin1();
    xcnt=atoi(ui->le_showcnt->text().toLatin1().data());        //X轴范围
    ba=ui->le_runFq->text().toLatin1();
    wave_period=atoi(ui->le_runFq->text().toLatin1().data());      //刷图周期

    ymin=0;
    ymax=0;
    m_chart=createChart(xcnt);
    chartView = new QChartView(m_chart);
    centerLayout->addWidget(chartView,1);

    te_debug_show=new QTextEdit();
    te_debug_show->setMinimumHeight(50);
    te_debug_show->setMaximumHeight(80);
    centerLayout->addWidget(te_debug_show,1);

    Get_Serial();

    wave_time=new QTimer();

    connect(ui->btn_start,SIGNAL(clicked(bool)),this,SLOT(slt_open_serial()));
    connect(m_serial,SIGNAL(readyRead()),this,SLOT(slt_read_serial()));
    connect(wave_time,SIGNAL(timeout()),this,SLOT(slt_time_timeout()));
    //connect(ui->btn_test,SIGNAL(clicked(bool)),this,SLOT(slt_test()));

    connect(ui->le_showcnt,SIGNAL(editingFinished()),this,SLOT(slt_change_xcnt()));
    connect(ui->le_runFq,SIGNAL(editingFinished()),this,SLOT(slt_change_period()));
}

WaveScope::~WaveScope()
{
    delete ui;
}

void WaveScope::UpadteChart()       //图表刷新
{
    int len=0;
    if(DataFlag==1)     //有可用数据添加
    {
        DataFlag=0;

        qreal t_data=getData();
        QPointF Datavalue((qreal)xcnt+1,t_data);
        m_list.append(Datavalue);
    }

    len=m_list.count();
    if(len==0)
        return;

    LeftShift(&m_list,1);

    if(m_list.at(0).x()<0)                //有坐标点需要移除
    {
        m_list.removeAt(0);
    }

    qreal ymin_bak=ymin,ymax_bak=ymax;
    len=m_list.count();
    for(int i=0;i<len;i++)                  //查找幅值上下限
    {
        qreal val=m_list.at(i).y();
        if(val>ymax)    ymax=val;
        else if(val<ymin)   ymin=val;
    }

    m_series->replace(m_list);              //将刷新后的数据表替换到series中

    if((ymin_bak!=ymin) || (ymax_bak!=ymax))        //幅值有变化，需要刷新坐标
    {
        QValueAxis *axisY = new QValueAxis;
        axisY->setTitleText("Data");
        axisY->setRange(ymin,ymax);
        m_chart->setAxisY(axisY);
        m_series->attachAxis(axisY);
    }
}


QChart *WaveScope::createChart(quint16 cnt)     //初始化图表
{
    // spine chart
    QChart *chart = new QChart();
    //chart->setTitle("Serial Data");
    //QString name("Series ");

    m_list.clear();
    accindex=0;

    m_series = new QLineSeries(chart);

    m_series->setName("Serial Data");
    chart->addSeries(m_series);

    QValueAxis *axisX = new QValueAxis;         //坐标轴设置
    axisX->setTitleText("Time");
    axisX->setRange(0,cnt);
    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("Data");
    axisY->setRange(0,25);
    chart->setAxisX(axisX,m_series);
    chart->setAxisY(axisY,m_series);

    return chart;
}

void WaveScope::LeftShift(DataList *t_list,int cnt)     //左移操作
{
    int ListLen=t_list->count();
    for(int i=0;i<ListLen;i++)
    {
        QPointF pointf=t_list->at(i);
        pointf.rx()-=cnt;
        t_list->replace(i,pointf);
    }
}

void WaveScope::RightShift(DataList *t_list,int cnt)    //右移操作
{
    int ListLen=t_list->count();
    for(int i=0;i<ListLen;i++)
    {
        QPointF pointf=t_list->at(i);
        pointf.rx()+=cnt;
        t_list->replace(i,pointf);
    }
}


void WaveScope::PointCntChange(quint16 cnt)
{
    if(cnt>xcnt)        //增加
    {
        int len=cnt-xcnt;
        //RightShift(m_series,len);

        QPointF Datavalue(0,0);
        for(int i=0;i<len;i++)
        {
            Datavalue.setX(i);
            m_series->insert(i,Datavalue);
        }

    }
    else if(cnt<xcnt)   //减少
    {
        int len=xcnt-cnt;
        m_series->removePoints(0,len);
        //LeftShift(m_series,len);
    }
    xcnt=cnt;
}

void WaveScope::slt_open_serial()
{
    Set_Serial();
}

void WaveScope::slt_read_serial()
{
    QByteArray s_data=m_serial->readAll();
    int len=s_data.count();
    if(s_data.at(0)=='D' &&
            s_data.at(1)=='T' &&
            s_data.at(len-2)==0x0D &&
            s_data.at(len-1)==0x0A)
        pro_type=1;
    else
        pro_type=0;

    if(pro_type)        //图显数据
    {
        s_data.remove(len-2,2);     //剔除协议字符，保留数据字符
        s_data.remove(0,2);
        len-=4;
        for(int i=0;i<8;i++)
        {
            /*数据保存到一个8位数组中，所以支持
             * 1位(char/uchar)
             * 2位(short/ushort)
             * 4位(int/uint/float)
             * 8位(long/ulong/double)
             * */
            if((len-i-1)>=0)
                serial_data[i]=s_data.at(len-i-1);
            else
                serial_data[i]=0;
        }
        DataFlag=1;
    }
    else                //调试数据
    {
        //直接终端显示
        debug_string.append(s_data);
        te_debug_show->setText(debug_string);
    }
}

void WaveScope::slt_change_xcnt()
{
    QByteArray ba=ui->le_showcnt->text().toLatin1();
    xcnt=atoi(ba.data());        //X轴范围

    if(xcnt<=0)
        xcnt=800;
    qDebug()<<xcnt;


    m_list.clear();
    m_series->clear();
    ymin=0;
    ymax=0;

    QValueAxis *axisX = new QValueAxis;
    axisX->setTitleText("Time");
    axisX->setRange(0,xcnt);
    m_chart->setAxisX(axisX);
    m_series->attachAxis(axisX);
}

void WaveScope::slt_change_period()
{
    QByteArray ba=ui->le_runFq->text().toLatin1();
    wave_period=atoi(ba.data());      //刷图周期

    if(wave_period<=0)
        wave_period=5;
}

//串口部分函数
void WaveScope::Get_Serial()
{
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->cb_name->addItem(info.portName());
    }

    m_serial=new QSerialPort();
}

void WaveScope::Set_Serial()
{
    //设置串口号
    QString comname=ui->cb_name->currentText();
    m_serial->setPortName(comname);

    //设置波特率
    quint32 baudrate_val = atoi(ui->cb_bdrate->currentText().toLatin1());
    m_serial->setBaudRate(baudrate_val,QSerialPort::AllDirections);


    //设置数据位
    qint32 databits_index=ui->cb_databit->currentIndex();
    switch (databits_index) {
    case 0:
        m_serial->setDataBits(QSerialPort::Data5);
        break;
    case 1:
        m_serial->setDataBits(QSerialPort::Data6);
        break;
    case 2:
        m_serial->setDataBits(QSerialPort::Data7);
        break;
    case 3:
        m_serial->setDataBits(QSerialPort::Data8);
        break;
    default:
        m_serial->setDataBits(QSerialPort::UnknownDataBits);
        break;
    }

    //设置校验位
    qint32 parity_index=ui->cb_parity->currentIndex();
    switch (parity_index) {
    case 0:
        m_serial->setParity(QSerialPort::NoParity);
        break;
    case 1:
        m_serial->setParity(QSerialPort::OddParity);
        break;
    case 2:
        m_serial->setParity(QSerialPort::EvenParity);
        break;
    default:
        m_serial->setParity(QSerialPort::UnknownParity);
        break;
    }

    //设置停止位
    qint32 stopbit_index=ui->cb_stopbit->currentIndex();
    switch (stopbit_index) {
    case 0:
        m_serial->setStopBits(QSerialPort::OneStop);
        break;
    case 1:
        m_serial->setStopBits(QSerialPort::TwoStop);
        break;
    default:
        m_serial->setStopBits(QSerialPort::UnknownStopBits);
        break;
    }


    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if(ui->btn_start->text()=="打开串口")
    {
        bool com=m_serial->open(QIODevice::ReadWrite);//打开串口并选择读写模式
        if(com)
        {
            ui->btn_start->setText("关闭串口");
            wave_time->start(wave_period);

            ui->cb_name->setEnabled(false);
            ui->cb_bdrate->setEnabled(false);
            ui->cb_databit->setEnabled(false);
            ui->cb_parity->setEnabled(false);
            ui->cb_stopbit->setEnabled(false);

            ui->le_runFq->setEnabled(false);
            ui->le_showcnt->setEnabled(false);
        }
    }
    else
    {
        m_serial->close();
        ui->btn_start->setText("打开串口");
        wave_time->stop();

        ui->cb_name->setEnabled(true);
        ui->cb_bdrate->setEnabled(true);
        ui->cb_databit->setEnabled(true);
        ui->cb_parity->setEnabled(true);
        ui->cb_stopbit->setEnabled(true);

        ui->le_runFq->setEnabled(true);
        ui->le_showcnt->setEnabled(true);
    }
}

qreal WaveScope::getData()
{
    return (qreal)(*(quint32 *)serial_data);
}

void WaveScope::slt_time_timeout()
{
    UpadteChart();
}

void WaveScope::slt_test()
{
    UpadteChart();
}

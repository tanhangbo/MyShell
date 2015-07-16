#include "dialog.h"
#include "ui_dialog.h"
#include <QTextCodec>





Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    /* UTF-8 support */
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    ui->setupUi(this);

    QString title = "MyShell 0.2 - tanhangbo build " + tr(__DATE__);
    this->setWindowTitle(title);
    this->setWindowFlags(this->windowFlags() | Qt::WindowMinMaxButtonsHint);



    serial = new QSerialPort(this);
    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));



    standardItemModel = new QStandardItemModel(this);
    ui->history_list->setModel(standardItemModel);

    ui->status->setText("");

    /* arrow up and down event */
    ui->sendcontent->installEventFilter(this);
    ui->receive_content->installEventFilter(this);
    history_index = 0;
    current_index = 0;


    log_current_file_name = "";
    log_is_logging = false;
    cur_file = NULL;

    error_closed = 0;

    find_serial();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::append_receive_content(QString str)
{
    ui->receive_content->moveCursor(QTextCursor::End);
    ui->receive_content->insertPlainText(str);
    ui->receive_content->moveCursor(QTextCursor::End);

}

void Dialog::append_item_to_list(QString str)
{

    QStandardItem *item = new QStandardItem(str);
    item->setEditable(false);
    standardItemModel->appendRow(item);

}

void Dialog::find_serial()
{

    foreach (const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {
        ui->setting_port->addItem(serialPortInfo.description() + "(" + serialPortInfo.portName() + ")");
    }
    ui->setting_baud->addItem("9600");
    ui->setting_baud->addItem("19200");
    ui->setting_baud->addItem("38400");
    ui->setting_baud->addItem("115200");
    ui->setting_baud->setCurrentIndex(2);

    ui->setting_databits->addItem("5");
    ui->setting_databits->addItem("6");
    ui->setting_databits->addItem("7");
    ui->setting_databits->addItem("8");
    ui->setting_databits->setCurrentIndex(3);


    ui->setting_stopbit->addItem("1");
    ui->setting_stopbit->addItem("1.5");
    ui->setting_stopbit->addItem("2");

}

bool Dialog::eventFilter(QObject* obj, QEvent *event)
{
    if (obj == ui->sendcontent)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up)
            {
                current_index--;
                if (current_index < 0)
                    current_index = 0;
                ui->sendcontent->setText(history[current_index]);
                //qDebug() << "lineEdit -> Qt::Key_Up";
                return true;
            }
            else if(keyEvent->key() == Qt::Key_Down)
            {
                current_index++;
                if (current_index >= history_index)
                    current_index = history_index;
                if (current_index == 100)
                    current_index = 0;
                ui->sendcontent->setText(history[current_index]);

                // qDebug() << "lineEdit -> Qt::Key_Down";
                return true;
            }
        }
        return false;
    }


    if (obj == ui->receive_content)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_Return)
            {
                writeData("");
                ui->sendcontent->setFocus();
            }
            else
            {
                //do nothing
            }
        }
        return false;
    }


    return Dialog::eventFilter(obj, event);
}





void Dialog::on_open_clicked()
{


    if (ui->open->text() == "关闭") {
        closeSerialPort();
        ui->open ->setText("打开");
        ui->status->setText(" 已关闭");
        append_receive_content("\n\n[MyShell]Connection closed.\n\n\n");
        return;
    }



    openSerialPort(get_serialport_setting());
}


struct serial_port_info Dialog::get_serialport_setting()
{

    /* port_name */
    QString ui_port_name = ui->setting_port->currentText();
    int left_identity = ui_port_name.indexOf("(", 0);
    int right_identity = ui_port_name.indexOf(")", 0);
    QString port_name = ui_port_name.mid(left_identity + 1, right_identity - left_identity - 1);

    QSerialPort::DataBits data_bits;
    QString ui_data_bits = ui->setting_databits->currentText();
    if (ui_data_bits == "5")
        data_bits = QSerialPort::Data5;
    else if (ui_data_bits == "6")
        data_bits = QSerialPort::Data6;
    else if (ui_data_bits == "7")
        data_bits = QSerialPort::Data7;
    else if (ui_data_bits == "8")
        data_bits = QSerialPort::Data8;
    else
        data_bits = QSerialPort::Data8;

    QString ui_baud = ui->setting_baud->currentText();
    QSerialPort::BaudRate baud_rate;
    if (ui_baud == "9600")
        baud_rate = QSerialPort::Baud9600;
    else if (ui_baud == "19200")
        baud_rate = QSerialPort::Baud19200;
    else if (ui_baud == "38400")
        baud_rate = QSerialPort::Baud38400;
    else if (ui_baud == "115200")
        baud_rate = QSerialPort::Baud115200;


    QString ui_stopbit = ui->setting_stopbit->currentText();
    QSerialPort::StopBits stop_bit;
    if (ui_stopbit == "1")
        stop_bit = QSerialPort::OneStop;
    if (ui_stopbit == "1.5")
        stop_bit = QSerialPort::OneAndHalfStop;
    if (ui_stopbit == "2")
        stop_bit = QSerialPort::TwoStop;



    struct serial_port_info info;
    info.PortName = port_name;
    info.DataBits = data_bits;
    info.FlowControl = QSerialPort::NoFlowControl;
    info.Parity = QSerialPort::NoParity;
    info.BaudRate = baud_rate;
    info.StopBits = stop_bit;

    return info;

}

void Dialog::openSerialPort(struct serial_port_info info)
{

    qDebug() << "using " <<info.PortName<<info.BaudRate<<info.DataBits<<info.StopBits;



    append_receive_content("\n[MyShell]Connecting to " + info.PortName + ".\n");


    serial->setPortName(info.PortName);
    serial->setBaudRate(info.BaudRate);
    serial->setDataBits(info.DataBits);
    serial->setParity(info.Parity);
    serial->setStopBits(info.StopBits);
    serial->setFlowControl(info.FlowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        ui->status->setText(info.PortName + " open ok");
        error_closed = false;
        ui->open ->setText("关闭");
        append_receive_content("\n[MyShell]Connected.\n");
    } else {
        ui->status->setText(info.PortName + " open fail");
        append_receive_content("\n[MyShell]Failed to open " + info.PortName + ".\n");
    }

}

void Dialog::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
}

void Dialog::writeData(QByteArray data)
{

    if (error_closed) {
        closeSerialPort();
        openSerialPort(get_serialport_setting());
    }

    if (!serial->isOpen()) {
        ui->status->setText("serial not opened");
        return;
    }



    data.append("\r\n");
    serial->write(data);
}

void Dialog::readData()
{
    int i = 0;
    QByteArray raw_data = serial->readAll();
    QByteArray data;

    /* remove '\r' */
    for (i  = 0; i < raw_data.size(); i++)
        if (raw_data.at(i) != 0x0d)
            data.append(raw_data.at(i));


    if (cur_file) {
        QTextStream stream(cur_file);
        stream << data ;
    }


    //ui->receive_content->moveCursor(QTextCursor::End);
    ui->receive_content->insertPlainText(data);
    ui->receive_content->moveCursor(QTextCursor::End);
}
/*
QSerialPort::NoError	0	No error occurred.
QSerialPort::DeviceNotFoundError	1	An error occurred while attempting to open an non-existing device.
QSerialPort::PermissionError	2	An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open.
QSerialPort::OpenError	3	An error occurred while attempting to open an already opened device in this object.
QSerialPort::ParityError	4	Parity error detected by the hardware while reading data.
QSerialPort::FramingError	5	Framing error detected by the hardware while reading data.
QSerialPort::BreakConditionError	6	Break condition detected by the hardware on the input line.
QSerialPort::WriteError	7	An I/O error occurred while writing the data.
QSerialPort::ReadError	8	An I/O error occurred while reading the data.
QSerialPort::ResourceError	9	An I/O error occurred when a resource becomes unavailable, e.g. when the device is unexpectedly removed from the system.
QSerialPort::UnsupportedOperationError	10	The requested device operation is not supported or prohibited by the running operating system.
QSerialPort::UnknownError	11	An unidentified error occurred.

*/
void Dialog::handleError(QSerialPort::SerialPortError err)
{

    qDebug() << __FUNCTION__ << err;

    if (QSerialPort::ReadError == err) {
        append_receive_content("[MyShell]Serial port got unplugged.\n");
        error_closed = true;
    }

}

/*
void Dialog::on_test_clicked()
{
    return;

    foreach (const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {

        qDebug() << "\nPort:" << serialPortInfo.portName();
        qDebug() << "Location:" << serialPortInfo.systemLocation();
        qDebug() << "Description:" << serialPortInfo.description();
        qDebug() << "Manufacturer:" << serialPortInfo.manufacturer();
        qDebug() << "Vendor Identifier:" << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : QByteArray());
        qDebug() << "Product Identifier:" << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : QByteArray());
        qDebug() << "Busy:" << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No"));

        QSerialPort *port = new QSerialPort(serialPortInfo);
        if (port->open(QIODevice::ReadWrite)) {
            qDebug() << "Baud rate:" << port->baudRate();
            qDebug() << "Data bits:" << port->dataBits();
            qDebug() << "Stop bits:" << port->stopBits();
            qDebug() << "Parity:" << port->parity();
            qDebug() << "Flow control:" << port->flowControl();
            qDebug() << "Read buffer size:" << port->readBufferSize();
            port->close();
        } else {
            qDebug() << "Unable to open port, error code" << port->error();
        }
        delete port;
    }


    //open_serial();


}
*/
void Dialog::on_sendcontent_returnPressed()
{
    QString content = ui->sendcontent->text();
    QByteArray data = content.toLatin1();
    writeData(data);
    ui->sendcontent->clear();



    if (data.length() > 0) {
        history[history_index].append(data);
        history_index++;
        if (history_index == 100)
            history_index = 0;
        current_index = history_index;

        append_item_to_list(data);
    }
}


void Dialog::on_history_list_doubleClicked(const QModelIndex &index)
{
    QString data = standardItemModel->itemFromIndex(index)->text();
    qDebug()<<"double clicked" << data;
    writeData(data.toLatin1());
}

void Dialog::on_clear_clicked()
{
    ui->receive_content->clear();
}

QString get_time_string()
{
    QDateTime cur_time = QDateTime::currentDateTime();
    return cur_time.toString("yyyy_MM_dd_hhmmss");
}

void Dialog::on_startrecord_clicked()
{

    if (log_current_file_name.length() > 0) {
        log_current_file_name = "";
        ui->startrecord->setText("开始记录");
        append_receive_content("\n[MyShell]logging stopped\n\n");
        if (cur_file) {
            cur_file->close();
            delete cur_file;
            cur_file = NULL;
        }
        return;
    }


    qDebug() << QDir::currentPath();
    QDir dir(QDir::currentPath() + "/logs");
    if (!dir.exists()) {
        qDebug() << "create new log dir";
        dir.mkpath(".");
    }

    QString file_name = QDir::currentPath() + "/logs/myshell_" + get_time_string() + ".txt";
    qDebug() << file_name;

    log_current_file_name = file_name;


    if (cur_file != NULL) {
        delete cur_file;
        cur_file = NULL;
    }
    cur_file = new QFile(log_current_file_name);
    if (!cur_file->open(QIODevice::ReadWrite)) {
        cur_file->close();
        qDebug() << "error create";
    }

    ui->startrecord->setText("停止记录");
    append_receive_content("\n[MyShell]logging enabled, file at  " + log_current_file_name + "\n\n");

}

void Dialog::on_open_dir_clicked()
{
    QString log_current_path = QDir::currentPath() + "/" + "logs";

    QDir dir(log_current_path);
    if (!dir.exists()) {
        qDebug() << "create new log dir";
        dir.mkpath(".");
    }

    QDesktopServices::openUrl(QUrl(log_current_path, QUrl::TolerantMode));

}

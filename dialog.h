#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QKeyEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QDesktopServices>

struct serial_port_info {
    QString PortName;
    QSerialPort::BaudRate BaudRate;
    QSerialPort::DataBits DataBits;
    QSerialPort::Parity Parity;
    QSerialPort::StopBits StopBits;
    QSerialPort::FlowControl FlowControl;
};


namespace Ui {
class Dialog;
}



class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private slots:
    void handleError(QSerialPort::SerialPortError);
    void readData();
    void on_sendcontent_returnPressed();
    void on_open_clicked();
    void on_history_list_doubleClicked(const QModelIndex &index);

    void on_clear_clicked();

    void on_startrecord_clicked();

    void on_open_dir_clicked();

private:
    Ui::Dialog *ui;
    QSerialPort *serial;
    void openSerialPort(struct serial_port_info info);
    void closeSerialPort();
    void writeData(QByteArray data);
    bool eventFilter(QObject* obj, QEvent *event);
    void find_serial();
    void append_item_to_list(QString);
    struct serial_port_info get_serialport_setting();



    QStandardItemModel *standardItemModel;

    QByteArray history[100];
    int history_index;
    int current_index;

    QFile *cur_file;
    QString log_current_file_name;
    int log_is_logging;

    void append_receive_content(QString);

    bool error_closed;

};

#endif // DIALOG_H

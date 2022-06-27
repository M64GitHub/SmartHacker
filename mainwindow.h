#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "rv_hex_edit.h"

extern "C" {
    #include "gcm.h"
    #include "detect_platform.h"
    #include <math.h>
}


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void SetEncSelection(int s, int e);

    rv_hex_edit *he1;
    rv_hex_edit *he2;

    QSerialPort com;
    QByteArray buffer;

public slots:
    void read_serial();
    void decrypt();
    void decode();
    void serial_changed(QString s);
    void debug_log(QString s);

    void spinbox_enc_changed();
    void he1_selection_changed(unsigned int from, unsigned int to);

    // void raw_selection_changed();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H

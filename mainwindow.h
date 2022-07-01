#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "rv_hex_edit.h"
#include "bytebuffer.h"

#include "dlms-apdu.h"
#include "autohacker.h"

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

    DlmsApdu    apdu;
    AutoHacker  autohacker; 

public slots:
    void read_serial();
    void set_raw_from_text();

    void parse_raw();

    void decrypt_apdu();
    void decode_apdu();

    void serial_changed(QString s);
    void debug_log(QString s);

    void spinbox_enc_changed();
    void he1_selection_changed(unsigned int from, unsigned int to);

    void autohack();
    void print_result_data(int i);
    void add_color_ranges_decoded();

    QString get_num_dec_hex(int i);

    void result_list_clicked(int i);
    void add_color_ranges_result(int i); // SYS_TITLE, etc...

    void set_offs_st();
    void set_offs_fc();

    void update_cursorpos_label(unsigned int);

    void key_list_clicked(QString s);
    void add_key_to_list();
    // void raw_selection_changed();
    

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H

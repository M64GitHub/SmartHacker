#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    he1 = new rv_hex_edit(this);
    he2 = new rv_hex_edit(this);

    he1->setMinimumWidth(930);

    ui->horizontal_Layout_hex1->addWidget(he1);
    ui->horizontal_Layout_hex2->addWidget(he2);

// blue
//  color_bg1_vals  = QColor(0xf0, 0xe8, 0xf8);
//  color_bg2_vals  = QColor(0xe0, 0xd8, 0xe8);

// green
//    he2->color_bg1_vals = QColor(0xf0, 0xf8, 0xe8);
//    he2->color_bg2_vals = QColor(0xe0, 0xe8, 0xd8);

// pink
//    he2->color_bg1_vals  = QColor(0xf0, 0xe8, 0xf8);
//    he2->color_bg2_vals  = QColor(0xe0, 0xd8, 0xe8);

    connect(ui->pushButton_read, SIGNAL(clicked()), this, SLOT(read_serial()));
    connect(ui->pushButton_decrypt, SIGNAL(clicked()), this, SLOT(decrypt()));
    connect(ui->m_serialPortComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(serial_changed(QString)));

    connect(ui->spinBox_enc_from, SIGNAL(valueChanged(int)), this , SLOT(spinbox_enc_changed()));
    connect(ui->spinBox_enc_to, SIGNAL(valueChanged(int)), this , SLOT(spinbox_enc_changed()));

    connect(ui->pushButton_decode, SIGNAL(clicked()), this, SLOT(decode()));

    connect(ui->pushButton_set_raw, SIGNAL(clicked()), this, SLOT(set_raw_from_text()));

    // --

    connect(he1, SIGNAL(selection_changed(unsigned int, unsigned int)),
            this, SLOT(he1_selection_changed(unsigned int, unsigned int)));

    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
        ui->m_serialPortComboBox->addItem(info.portName());

    com.setPortName(ui->m_serialPortComboBox->itemText(0));
    com.setBaudRate(QSerialPort::Baud2400);
    com.setReadBufferSize(1);
    com.open(QSerialPort::ReadOnly);

    gcm_initialize();

    debug_log("SmartHacker initialized\n");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::debug_log(QString s)
{
    ui->textEdit_txt->append(s);
}

void MainWindow::serial_changed(QString s)
{
    debug_log("changing port to " + s);

    if(com.isOpen()) {
        debug_log("closing " + com.portName());
        com.close();
    }

    com.setBaudRate(QSerialPort::Baud2400);
    com.setReadBufferSize(1);
    com.setPortName(s);
    debug_log("opening " + com.portName());
    com.open(QSerialPort::ReadOnly);
}

void MainWindow::spinbox_enc_changed()
{
    int pos = ui->spinBox_enc_from->value();
    int len = (ui->spinBox_enc_to->value() - ui->spinBox_enc_from->value());
    if(len >0)
        SetEncSelection(pos, len);
}

void MainWindow::he1_selection_changed(unsigned int from, unsigned int to)
{
    ui->spinBox_enc_from->setValue(from);
    ui->spinBox_enc_to->setValue(to);
    he1->viewport()->repaint();
}

void MainWindow::SetEncSelection(int pos, int len)
{
    rv_hv_selection s;
    s.hv = he2;
    s.start = pos;
    s.end = pos+len;
    s.len = len;

    for(int i=0; i<he1->color_ranges.size(); i++) {
        if(he1->color_ranges.at(i).desc == "RANGE TO DECRYPT") {
            he1->color_ranges.removeAt(i);
            break;
        }
    }
    he1->add_color_range(pos, pos+len,
                         QColor(0x42, 0x85, 0xf4),
                         QColor(0xff, 0xff, 0xff),
                         "RANGE TO DECRYPT"
                         );
    he1->viewport()->repaint();


    // blue
    //  color_bg1_vals  = QColor(0xf0, 0xe8, 0xf8);
    //  color_bg2_vals  = QColor(0xe0, 0xd8, 0xe8);

    // green
    //    he2->color_bg1_vals = QColor(0xf0, 0xf8, 0xe8);
    //    he2->color_bg2_vals = QColor(0xe0, 0xe8, 0xd8);

    // pink
    //    he2->color_bg1_vals  = QColor(0xf0, 0xe8, 0xf8);
    //    he2->color_bg2_vals  = QColor(0xe0, 0xd8, 0xe8);
}

void MainWindow::read_serial()
{
    while(com.bytesAvailable()) {
        com.readAll();
        com.waitForReadyRead(1000);
    }

    QByteArray buffer;

    com.waitForReadyRead(6000);
    while(com.bytesAvailable()) {
        buffer.append(com.readAll());
        com.waitForReadyRead(100);
    }

    buf_raw.init(buffer.length());

    for(int i=0; i<buffer.length(); i++) {
        buf_raw.buf()[i] = buffer.data()[i];
    }

    // now buf_raw and buffer contain the same data

    he1->set_data_buffer(buf_raw.buf(), buf_raw.len());

    ui->spinBox_enc_from->setMaximum(buf_raw.len());
    ui->spinBox_enc_to->setMaximum(buf_raw.len());

    ui->spinBox_enc_from->setValue((int)(ENC_DATA_OFFS));
    ui->spinBox_enc_to->setValue(buffer.length() - 2 -1);

    parse_raw();
}

void MainWindow::set_raw_from_text()
{
    QByteArray buffer;

    buffer = QByteArray::fromHex(QByteArray::fromStdString(ui->textEdit_raw->toPlainText().toStdString()));
    
    ui->textEdit_txt->append("set raw from text: " + ui->textEdit_raw->toPlainText());
    ui->textEdit_txt->append("raw len: " + QString::number(buffer.length()));

    buf_raw.init(buffer.length());

    for(int i=0; i<buffer.length(); i++) {
        buf_raw.buf()[i] = buffer.data()[i];
    }

    // now buf_raw and buffer contain the same data

    he1->set_data_buffer(buf_raw.buf(), buf_raw.len());
    he1->viewport()->repaint();

    ui->spinBox_enc_from->setMaximum(buf_raw.len());
    ui->spinBox_enc_to->setMaximum(buf_raw.len());

    ui->spinBox_enc_from->setValue((int)(ENC_DATA_OFFS));
    ui->spinBox_enc_to->setValue(buffer.length() - 2 -1);

}

void MainWindow::parse_raw()
{
    he1->color_bg1_vals = QColor(0x59, 0x69, 0x8d);
    he1->color_bg2_vals = QColor(0x59, 0x69, 0x8d);
    he1->color_hex_vals = QColor(0xff, 0xff, 0xff);
    he1->viewport()->repaint();

    // -- encr apdu
    for(int i=0; i<he1->color_ranges.size(); i++) {
        if(he1->color_ranges.at(i).desc == "ENCRYPTED APDU") {
            he1->color_ranges.removeAt(i);
            break;
        }
    }
    he1->add_color_range((int)ENC_DATA_OFFS, (int)buf_raw.len()-2 - 1,
                         QColor(0xb0, 0xd0, 0xff),
                         QColor(0x20, 0x20, 0x20),
                         "ENCRYPTED APDU");

    // blue
    //  color_bg1_vals  = QColor(0xf0, 0xe8, 0xf8);
    //  color_bg2_vals  = QColor(0xe0, 0xd8, 0xe8);

    // green
    //    he2->color_bg1_vals = QColor(0xf0, 0xf8, 0xe8);
    //    he2->color_bg2_vals = QColor(0xe0, 0xe8, 0xd8);

    // pink
    //    he2->color_bg1_vals  = QColor(0xf0, 0xe8, 0xf8);
    //    he2->color_bg2_vals  = QColor(0xe0, 0xd8, 0xe8);


    // -- MBUS START
    for(int i=0; i<he1->color_ranges.size(); i++) {
        if(he1->color_ranges.at(i).desc == "MBUS START") {
            he1->color_ranges.removeAt(i);
            break;
        }
    }
    he1->add_color_range(0, 3,
                         QColor(0xab, 0x47, 0xbc),
                         QColor(0xff, 0xff, 0xff),
                         "MBUS START");

    // -- SYSTEM TITLE
    for(int i=0; i<he1->color_ranges.size(); i++) {
        if(he1->color_ranges.at(i).desc == "SYSTEM TITLE") {
            he1->color_ranges.removeAt(i);
            break;
        }
    }
    he1->add_color_range(11, 11+7,
                         QColor(0x43, 0xa0, 0x47),
                         QColor(0xff, 0xff, 0xff),
                         "SYSTEM TITLE");


    // -- FRAME CTR
    for(int i=0; i<he1->color_ranges.size(); i++) {
        if(he1->color_ranges.at(i).desc == "FRAME COUNTER") {
            he1->color_ranges.removeAt(i);
            break;
        }
    }
    he1->add_color_range(22, 22+3,
                         QColor(0xf4, 0x43, 0x36),
                         QColor(0xff, 0xff, 0xff),
                         "FRAME COUNTER");

    // -- MBUS CHCKSUM
    for(int i=0; i<he1->color_ranges.size(); i++) {
        if(he1->color_ranges.at(i).desc == "MBUS CHKSUM") {
            he1->color_ranges.removeAt(i);
            break;
        }
    }
    he1->add_color_range(buf_raw.len()-2, buf_raw.len()-2,
                         // QColor(0xec, 0x40, 0x7a),
                          QColor(0xf0, 0x62, 0x92),
                         QColor(0xff, 0xff, 0xff),
                         "MBUS CHKSUM");

    // -- MBUS CHCKSUM
    for(int i=0; i<he1->color_ranges.size(); i++) {
        if(he1->color_ranges.at(i).desc == "MBUS STOP") {
            he1->color_ranges.removeAt(i);
            break;
        }
    }
    he1->add_color_range(buf_raw.len()-1, buf_raw.len()-1,
                         QColor(0xf5, 0x7c, 0x00),
                         QColor(0xff, 0xff, 0xff),
                         "MBUS STOP");

    spinbox_enc_changed();
}

void MainWindow::decrypt()
{
    unsigned char tag[16];
    unsigned char iv[12];

    unsigned char *buf = buf_raw.buf();

    unsigned char key[] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };

    unsigned char aad = 0x30;

    // create iv
    for(int i=0;i<8;i++)
        iv[i] = buf[SYSTEM_TITLE_OFFS+i];
    for(int i=0;i<4;i++)
        iv[i+8] = buf[FRAME_CTR_OFFS+i];

    gcm_context ctx;

    QByteArray key_input = QByteArray::fromHex(QByteArray::fromStdString(ui->lineEdit_key->text().toStdString()));

    // gcm_setkey(&ctx, key, 16);
    gcm_setkey(&ctx,(unsigned char *) key_input.data(), 16);

    int decrypted_len = ui->spinBox_enc_to->text().toUInt() - ui->spinBox_enc_from->text().toUInt();

    buf_decrypted.init(decrypted_len);

    he2->set_data_buffer(buf_decrypted.buf(), decrypted_len);
    he2->color_bg1_vals = QColor(0xf0, 0xf8, 0xe8);
    he2->color_bg2_vals = QColor(0xe0, 0xe8, 0xd8);

    gcm_crypt_and_tag(
        &ctx,
        DECRYPT,
        iv, 12,
        &aad, 1,
        buf + ui->spinBox_enc_from->text().toUInt(),
        buf_decrypted.buf(),
        decrypted_len,
        tag, 16);

    he2->viewport()->repaint();

    printf("\nTAG:\n");
    for(int i=0; i<16; i++) {
        printf("%02X", tag[i]);
    }
    printf("\n\n");
    // decrypted[228] = 0xff;
    buf_decrypted.write_at(228, 0xff);

    ui->textEdit_txt->append("decrypted len: " + QString::number(buf_decrypted.len()));
}

void MainWindow::decode()
{
    apdu.scan_octetstrings(buf_decrypted.buf(), buf_decrypted.len());

    QString s;
    for(int i=0; i<apdu.num_entries; i++) {
        s = "ID: " + QString::fromLocal8Bit(apdu.entries[i].id) + "\n" +
            "NAME: " + QString::fromLocal8Bit(apdu.entries[i].name) + "\n" +
            "VALUE: " + QString::fromLocal8Bit(apdu.entries[i].strval) + "\n" +
            "UNIT: " + QString::fromLocal8Bit(apdu.entries[i].unit) + "\n";
        debug_log(s);
    }
}

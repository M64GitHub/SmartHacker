#include "mainwindow.h"
#include "ui_mainwindow.h"


#define MAX_FRAMESIZE     512
#define MIN_FRAMESIZE     200

#define MAX_UNITS           5
#define MAX_TRANSLATIONS   64

#define MBUS_START_OFFS     0
#define MBUS_START_LEN      4

#define SYSTEM_TITLE_OFFS  11
#define SYSTEM_TITLE_LEN    8

#define FRAME_CTR_OFFS     22
#define FRAME_CTR_LEN       4

#define ENC_DATA_OFFS      26
#define ENC_DATA_END_OFFS  -2

typedef struct s_entry {
    char name[32];
    int  offset;
    char id[32];
    char unit[32];
    char strval[32];
} entry;

typedef struct s_unit {
    int id;
    char name[32];
} unit;

entry entries[64];
int num_entries = 0;
char datetime[128];

unsigned char RAW[256+128];
int RAW_len = 0;

unsigned char decrypted[256+128];
int decrypted_len = 0;

unit units[MAX_UNITS];

typedef struct s_translation {
    char id[32];
    char translation[32];
} translation;

translation translations[64];

int num_translations = 0;

void init_entries()
{
    for(int i=0; i<num_entries; i++) {
        entries[i].id[0] = 0x0;
        entries[i].name[0] = 0x0;
        entries[i].unit[0] = 0x0;
        entries[i].strval[0] = 0x0;
    }
    num_entries = 0;
}

void init_units()
{
    units[0].id = 0x1e;
    strcpy(units[0].name, "Wh");

    units[1].id = 0x1b;
    strcpy(units[1].name, "W");

    units[2].id = 0x23;
    strcpy(units[2].name, "V");

    units[3].id = 0x21;
    strcpy(units[3].name, "A");

    units[4].id = 0xff;
    strcpy(units[4].name, " ");
}

char *get_unit(unsigned char id)
{
    for(int i=0; i<MAX_UNITS; i++) {
        if(id == units[i].id) return units[i].name;
    }

    return 0; // unit not found
}

char *translate(char *id)
{
    for(int i=0; i<num_translations; i++) {
        if(!strcmp(translations[i].id, id)) {
            return translations[i].translation;
        }
    }

    return 0;
}

int read_entry(unsigned char *decr, entry *e)
{
    int offs = e->offset;
    unsigned char *p = decr;

    // -- read octet string: .id
    if(p[offs+0] != 0x09) {
        printf("ERROR @%04x: data type != octetstring (0x09): %02x\n",
               offs, p[offs+0]);
        return 1;
    }

    char buf[64];
    e->id[0] = 0x0;
    int octetlen = p[offs+1];
    for(int i=0; i<octetlen; i++) {
        if(i!=octetlen-1)
            sprintf(buf, "%d.", p[offs+2+i]);
        else
            sprintf(buf, "%d", p[offs+2+i]);
        strcat(e->id, buf);
    }
    offs += 2 + octetlen;

    // -- read data
    // 0x06 : UInt32, or 0x12: UInt16

    double val = 0;

    if( (p[offs+0] != 0x06) && (p[offs+0] != 0x12) ) {
       printf("ERROR @%04x: data type != UInt32/UInt16 (06/12): %02x\n",
              offs, p[offs+0]);
        return 1;
    }

    if(p[offs+0] == 0x06) {
        // Uint32
        val += p[offs+1] << 24;
        val += p[offs+2] << 16;
        val += p[offs+3] << 8;
        val += p[offs+4];
        offs +=4+1;
    } else if(p[offs+0] == 0x12) {
        // Uint16
        val += p[offs+1] << 8;
        val += p[offs+2];
        offs += 2+1;
    }

    // scale / unit
    // first skip struct: 0x02, 0x02
    if( (p[offs+0] != 0x02) && (p[offs+1] != 0x02) ) {
        printf("ERROR @%04x: no struct found (expected 0x0202)\n", offs);
        return 1;
    }
    offs += 2;

    // read Int8
    if(p[offs+0] != 0x0F) {
        printf("ERROR @%04x: no Int8 found (expected 0x0F)\n", offs);
        return 1;
    }
    offs++;
    // read scaler
    if(p[offs]) { // if scaler != 0
        int scaler = p[offs];
        if(scaler > 127) scaler = scaler - 256;

        // printf("    scaler: %d", scaler);
        double pot = pow(10.0 , scaler);
        // printf(", val = val * %f\n", pot);
        val = val * pot;
    } /* else printf("skipping scaler ...\n"); */

    snprintf(e->strval, 16, "%.2f", val);

    // unit
    offs += 2;
    char *unit;

    if(!(unit=get_unit(p[offs]))) {
        printf("ERROR @%04x: unit not found for %02x\n",
               offs, p[offs]);
        return 1;
    }
    strcpy(e->unit, unit);

    char *t = translate(e->id);
    if(t) strcpy(e->name, t);

    return 0;
};

void read_timestamp(unsigned char *p, char *d)
{
    int year, month, day, hour, minute, second = 0;

    year    = p[0]*256;
    year    += p[1];

    month   = p[2];
    day     = p[3];

    hour    = p[5];
    minute  = p[6];
    second  = p[7];

    sprintf(d, "%02d.%02d.%4d %02d:%02d:%02d",
            day, month, year, hour, minute, second);
}

void print_entry(entry *e)
{
//    printf("ID        : %s\n", e->id);
//    printf("NAME      : %s\n", e->name);
//    printf("VAL       : %s %s\n", e->strval, e->unit);
//    printf("\n");
    char pbuf[128];
    sprintf(pbuf, "ID        : %s\n", e->id);

    sprintf(pbuf, "NAME      : %s\n", e->name);
    sprintf(pbuf, "VAL       : %s %s\n", e->strval, e->unit);
}

void get_time(unsigned char *p)
{
    // read datetime
    read_timestamp(p+6, datetime);
    printf("Timestamp : %s\n", datetime);
}

// -- scan functions

void scan_octetstrings(unsigned char *p, int maxlen)
{
    // 09 XX (<8) YY (<3)

    int offs = 0;
    char buf1[4];
    char buf[64];

    printf("scanning for object IDs ...\n\n");

    while(offs < (maxlen-0x08)) {
        offs++;
        if ((p[offs+0] == 0x09) && (p[offs+1] <0x0C) && (p[offs+2] <0x03)  ) {
            int found_pos = offs;
            buf[0] = 0x0;
            int octetlen = p[offs+1];
            for(int i=0; i<octetlen; i++) {
                if(i!=octetlen-1)
                    sprintf(buf1, "%d.", p[offs+2+i]);
                else
                    sprintf(buf1, "%d", p[offs+2+i]);
                strcat(buf, buf1);
            }
            offs += 2 + octetlen;

            printf("found: '%s' @ 0x%04x\n", buf, found_pos);

            entry *e = &entries[num_entries];
            e->offset = found_pos;
            int rv = read_entry(decrypted, e);
            if(!rv) num_entries++;
            else offs = found_pos + 1;
        }
    }

    printf("\n");
}

void print_entries()
{
    for(int i=0; i<num_entries; i++) {
        print_entry(&entries[i]);
    }
}

void init_translations()
{
    translation *t;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.1.8.0.255");
    strcpy(t->translation, "Wirkenergie A+");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.2.8.0.255");
    strcpy(t->translation, "Wirkenergie A-");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.1.7.0.255");
    strcpy(t->translation, "Momentanleistung P+");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.2.7.0.255");
    strcpy(t->translation, "Momentanleistung P-");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.32.7.0.255");
    strcpy(t->translation, "Spannung L1");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.52.7.0.255");
    strcpy(t->translation, "Spannung L2");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.72.7.0.255");
    strcpy(t->translation, "Spannung L3");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.31.7.0.255");
    strcpy(t->translation, "Strom L1");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.51.7.0.255");
    strcpy(t->translation, "Strom L2");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.71.7.0.255");
    strcpy(t->translation, "Strom L3");
    num_translations++;

    t = &translations[num_translations];
    strcpy(t->id, "1.0.13.7.0.255");
    strcpy(t->translation, "Leistungsfaktor");
    num_translations++;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    he1 = new rv_hex_edit(this);
    he2 = new rv_hex_edit(this);

    he1->setMinimumWidth(930);

//    he1->setMaximumWidth(he1->get_total_width()+40);

//    he1->setMinimumHeight(he1->get_total_height());

//    he2->setMaximumWidth(he1->get_total_width()+40);
//    he2->setMinimumHeight(he2->get_total_height());

    ui->horizontal_Layout_hex1->addWidget(he1);
    ui->horizontal_Layout_hex2->addWidget(he2);

    he1->set_data_buffer(RAW, 256);

    he2->set_data_buffer(decrypted, 16);
    he2->viewport()->repaint();

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
    init_entries();
    init_units();
    init_translations();

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
    buffer.clear();

    while(com.bytesAvailable()) {
        com.readAll();
        com.waitForReadyRead(1000);
    }

    com.waitForReadyRead(6000);
    while(com.bytesAvailable()) {
        buffer.append(com.readAll());
        com.waitForReadyRead(100);
    }

    for(int i=0; i<buffer.length(); i++) {
        RAW[i] = buffer.data()[i];
    }
    he1->set_data_buffer(RAW, buffer.length());

    ui->spinBox_enc_from->setMaximum(buffer.length());
    ui->spinBox_enc_to->setMaximum(buffer.length());

    ui->spinBox_enc_from->setValue((int)(ENC_DATA_OFFS));
    ui->spinBox_enc_to->setValue(buffer.length() - 2 -1);

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
    he1->add_color_range((int)ENC_DATA_OFFS, (int)buffer.length()-2 - 1,
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
    he1->add_color_range(buffer.length()-2, buffer.length()-2,
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
    he1->add_color_range(buffer.length()-1, buffer.length()-1,
                         QColor(0xf5, 0x7c, 0x00),
                         QColor(0xff, 0xff, 0xff),
                         "MBUS STOP");

    spinbox_enc_changed();
}

void MainWindow::decrypt()
{
    unsigned char tag[16];
    unsigned char iv[12];

    unsigned char *buf = RAW;

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

    gcm_setkey(&ctx, key, 16);

    decrypted_len = ui->spinBox_enc_to->text().toUInt() - ui->spinBox_enc_from->text().toUInt();

    he2->set_data_buffer(decrypted, decrypted_len);
    he2->color_bg1_vals = QColor(0xf0, 0xf8, 0xe8);
    he2->color_bg2_vals = QColor(0xe0, 0xe8, 0xd8);

    gcm_crypt_and_tag(
        &ctx,
        DECRYPT,
        iv, 12,
        &aad, 1,
        buf + ui->spinBox_enc_from->text().toUInt(),
        decrypted,
        decrypted_len,
        tag, 16);

    he2->viewport()->repaint();

    printf("\nTAG:\n");
    for(int i=0; i<16; i++) {
        printf("%02X", tag[i]);
    }
    printf("\n\n");
    decrypted[228] = 0xff;
}

void MainWindow::decode()
{
    scan_octetstrings(decrypted, decrypted_len);

    QString s;
    for(int i=0; i<num_entries; i++) {
        s = "ID: " + QString::fromLocal8Bit(entries[i].id) + "\n" +
            "NAME: " + QString::fromLocal8Bit(entries[i].name) + "\n" +
            "VALUE: " + QString::fromLocal8Bit(entries[i].strval) + "\n" +
            "UNIT: " + QString::fromLocal8Bit(entries[i].unit) + "\n";
        debug_log(s);
    }
}

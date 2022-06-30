#include "dlms-apdu.h"


DlmsApdu::DlmsApdu()
{
    init_entries();
    init_units();
    init_translations();
}

void DlmsApdu::init_entries()
{
    for(int i=0; i<num_entries; i++) {
        entries[i].id[0] = 0x0;
        entries[i].name[0] = 0x0;
        entries[i].unit[0] = 0x0;
        entries[i].strval[0] = 0x0;
    }
    num_entries = 0;
}

void DlmsApdu::init_units()
{
    num_entries = 0;

    units[num_entries].id = 0x1e;
    strcpy(units[num_entries++].name, "Wh");

    units[num_entries].id = 0x1b;
    strcpy(units[num_entries++].name, "W");

    units[num_entries].id = 0x23;
    strcpy(units[num_entries++].name, "V");

    units[num_entries].id = 0x21;
    strcpy(units[num_entries++].name, "A");

    units[num_entries].id = 0xff;
    strcpy(units[num_entries++].name, " ");
}

char *DlmsApdu::get_unit(unsigned char id)
{
    for(int i=0; i<MAX_UNITS; i++) {
        if(id == units[i].id) return units[i].name;
    }

    return 0; // unit not found
}

char *DlmsApdu::translate(char *id)
{
    for(int i=0; i<num_translations; i++) {
        if(!strcmp(translations[i].id, id)) {
            return translations[i].translation;
        }
    }

    return 0;
}

int DlmsApdu::read_apdu_entry(unsigned char *decr, apdu_entry *e)
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

void DlmsApdu::read_timestamp(unsigned char *p, char *d)
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

void DlmsApdu::print_apdu_entry(apdu_entry *e)
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

void DlmsApdu::get_time(unsigned char *p)
{
    // read datetime
    read_timestamp(p+6, datetime);
    printf("Timestamp : %s\n", datetime);
}

// -- scan functions

int DlmsApdu::scan_octetstrings(unsigned char *p, int maxlen)
{
    // 09 XX (<8) YY (<3)

    int offs = 0;
    char buf1[4];
    char buf[64];

    printf("scanning for object IDs ...\n\n");

    num_entries = 0;
    while(offs < (maxlen-0x08)) {
        offs++;
        if (
            // search criteria
            ((p[offs+0] == 0x09) && (p[offs+1] <0x0C) && (p[offs+2] <0x03)) &&
            // safety / segfault chk
            (offs + p[offs+1] < maxlen)
        ) {
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

            apdu_entry *e = &entries[num_entries];
            e->offset = found_pos;
            int rv = read_apdu_entry(p, e);
            if(!rv) num_entries++;
            else offs = found_pos + 1;
        }
    }
    printf("\n");

    return num_entries;
}

void DlmsApdu::print_entries()
{
    for(int i=0; i<num_entries; i++) {
        print_apdu_entry(&entries[i]);
    }
}

void DlmsApdu::init_translations()
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

void DlmsApdu::decrypt(offset sys_title,
                       offset frame_ctr,
                       offset encrypted_block_start,
                       offset encrypted_block_end,
                       unsigned char *key)
{
    unsigned char tag[16];
    unsigned char iv[12];

    unsigned char *buf = buf_raw.buf();

    // create iv
    for(int i=0;i<8;i++)
        iv[i] = buf[sys_title+i];
    for(int i=0;i<4;i++)
        iv[i+8] = buf[frame_ctr+i];

    gcm_context ctx;

    gcm_setkey(&ctx, key, 16);

    int decrypted_len = encrypted_block_end - encrypted_block_start;

    buf_decrypted.init(decrypted_len);

    gcm_crypt_and_tag(
        &ctx,
        DECRYPT,
        iv, 12,
        0, 0, 
        buf + encrypted_block_start,
        buf_decrypted.buf(),
        decrypted_len,
        tag, 16);

    printf("\nTAG:\n");
    for(int i=0; i<16; i++) {
        printf("%02X", tag[i]);
    }
    printf("\n\n");
}

void DlmsApdu::decode()
{
    num_entries = 0;
    scan_octetstrings(buf_decrypted.buf(), buf_decrypted.len());
}

#ifndef DLMS_APDU_H
#define DLMS_APDU_H
#include <math.h>
#include <string.h>
#include <stdio.h>
extern "C" {
#include "gcm.h"
#include "detect_platform.h"
}

#include "bytebuffer.h"

#define MAX_ENTRIES        64 
#define MAX_UNITS           5
#define MAX_TRANSLATIONS   64

// -- apdu data

typedef struct s_apdu_entry {
    char name[32];
    int  offset;
    char id[32];
    char unit[32];
    char strval[32];
    double val;
} apdu_entry;

typedef struct s_unit {
    int id;
    char name[32];
} unit;

typedef struct s_translation {
    char id[32];
    char translation[32];
} translation;

// -- dlms data

typedef struct s_dlms_parameters {
    offset  offs_SYSTEM_TITLE;
    offset  offs_FRAME_COUNTER;
    offset  offs_ENC_DATA;
    offset  len_ENC_DATA;
} dlms_parameters;

class DlmsApdu {

public:
    DlmsApdu();

    void init_entries();
    void init_units();
    void init_translations();

    int read_apdu_entry(unsigned char *decr, 
                        apdu_entry *e);
    char *get_unit(unsigned char id);
    char *translate(char *id);

    void get_time(unsigned char *p);
    void read_timestamp(unsigned char *p, char *d);
    void print_apdu_entry(apdu_entry *e);

    void scan_octetstrings(unsigned char *p, 
                           int maxlen);
    void print_entries();

    void set_enc_params(offset offs_sys_title,
                        offset offs_frame_ctr,
                        offset encrypted_block_end,
                        offset offs_enc_data);

    void decrypt(offset sys_title,
                 offset frame_ctr,
                 offset encrypted_block_start,
                 offset encrypted_block_end,
                 unsigned char *key);

    void decode();

    apdu_entry entries[MAX_ENTRIES];
    int  num_entries = 0;

    ByteBuffer buf_raw;
    ByteBuffer buf_decrypted;

private:
    dlms_parameters decrypt_params;

    unit    units[MAX_UNITS];
    int     num_units = 0;

    translation translations[64];
    int num_translations = 0;

    char    datetime[128];
};

#endif

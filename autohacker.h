#ifndef AUTOHACKER_H
#define AUTOHACKER_H

#include "dlms-apdu.h"

#define AUTO_HACKER_MAX_RESULTS 1024

class AutoHacker {

public:
    AutoHacker();

    void hack(DlmsApdu *a,
              int maxresults,
              int systitle_min,
              int systitle_max,
              int spce_systitle_framectr_max,
              int spce_framectr_enc_max,
              int threshold_entries,
              unsigned char *key,
              bool decode);

    // inputs
    char            known_plaintext = 0x0f;

    int             offs_systitle_min = 2;
    int             offs_systitle_max = 20;
    int             spce_framectr_enc_max = 8;

    int             threshold_min_good_entries = 2;

    bool            decode_entries = false;

    // results
    dlms_parameters results[AUTO_HACKER_MAX_RESULTS]; 
    int             num_results;

    DlmsApdu        *apdu;

    unsigned int iteration = 0;
};

#endif

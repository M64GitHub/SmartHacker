#include "autohacker.h"

AutoHacker::AutoHacker()
{
}

void AutoHacker::hack(DlmsApdu *a,
                 int maxresults, 
                 int systitle_min, 
                 int systitle_max, 
                 int spce_systitle_framectr_max, 
                 int spce_framectr_enc_max,
                 int threshold_entries, 
                 unsigned char * key,
                 bool decode)
{
    apdu = a;
    num_results = 0;
    iteration = 0;
    
    // apdu->decrypt(
    //               offset sys_title, 
    //               offset frame_ctr, 
    //               offset encrypted_block_start, 
    //               offset encrypted_block_end, 
    //               unsigned char *key
    //               )

    for(int i = 0; i<(systitle_max - systitle_min); i++)
    for(int j = 0; j<spce_systitle_framectr_max; j++)
    for(int k = 0; k<spce_framectr_enc_max; k++)
    {
                                // -8 : min 8 bytes to decrypt
        if((apdu->buf_raw.len()-2-8 -(i + systitle_min)) > 
                        (i + systitle_min + 8 + j + 4 + k)) {
            apdu->decrypt(
                    i + systitle_min,
                    i + systitle_min + 8 + j,
                    i + systitle_min + 8 + j + 4 + k,
                    apdu->buf_raw.len() - 2,
                    key
                );

            // printf("iteration: %d: %02x\n", 
            //        iteration, 
            //        apdu->buf_decrypted.byte_at(0));

            if(apdu->scan_octetstrings(apdu->buf_decrypted.buf(), 
                                       apdu->buf_decrypted.len())
                    >= threshold_entries
            ) {
            // if(apdu->buf_decrypted.byte_at(0) == 0x0f) {
                // save result
                results[num_results].offs_SYSTEM_TITLE  = 
                            i + systitle_min;
                results[num_results].offs_FRAME_COUNTER = 
                            i + systitle_min + 8 + j ;
                results[num_results].offs_ENC_DATA = 
                            i + systitle_min + 8 + j + 4 + k;
                results[num_results].len_ENC_DATA =
                    apdu->buf_raw.len() - (i + systitle_min + 8 + j + 4 +  k  + 2);

                num_results++;

                if(num_results == AUTO_HACKER_MAX_RESULTS) goto found;
            }
        }

        iteration++;
    };

found:
    return;
}

#ifndef KLS_COMP_H
#define KLS_COMP_H

/* TODO: remove file
#include <stdint.h>


#define KLS_COMPRESS_PREV_OCCURENCE_TOO_LARGE 1
#define KLS_COMPRESS_LINE_NUMBER_TOO_LARGE 2
#define KLS_COMPRESS_PO_AND_LN_TOO_LARGE 3

struct KlsOcc 
{
    uint32_t jumpback;
    uint64_t line_number;    
    uint64_t prev_occurence;
};

struct KlsFileStart
{
    uint32_t file_index;
};

enum KlsItemType
{
    KLS_ITEM_UNKNOWN=0,
    KLS_ITEM_FILE_START,
    KLS_ITEM_OCC_16_22_22,
    KLS_ITEM_OCC_16_16_28,
    KLS_ITEM_OCC_8_26_26,
    KLS_ITEM_OCC_8_16_36,
    KLS_ITEM_OCC_8_36_16,
    KLS_ITEM_OCC_0_30_30,
    KLS_ITEM_OCC_0_16_44,
    KLS_ITEM_OCC_0_44_16
};
    
struct KlsItem 
{
    enum KlsItemType type;
    union 
    {
        struct KlsFileStart kfs;
        struct KlsOcc occ;
    };
};


typedef uint64_t KlsItemCompressed;

int kls_compress(uint32_t max_jumpback, uint64_t line_number,
                 uint64_t prev_occurence, KlsItemCompressed* res);
int kls_compress_file_start(uint32_t file_index,
                            KlsItemCompressed* res);

int kls_uncompress_item(KlsItemCompressed item, struct KlsItem* res);

*/
#endif

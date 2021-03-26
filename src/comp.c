#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "comp.h"


#define KLS_COMPRESSED_WIDTH 64
#define KLS_COMPRESSED_TYPE_WIDTH 4
#define KLS_COMPRESSED_PAYLOAD_WIDTH KLS_COMPRESSED_WIDTH - KLS_COMPRESSED_TYPE_WIDTH


void kls_compress_bitwise(int b0, int b1, int b2, struct KlsItem item,
                          KlsItemCompressed* res)
{
    assert(b0 + b1 + b2 == KLS_COMPRESSED_PAYLOAD_WIDTH);

    uint64_t mask_b0 = (1 << b0) - 1;
    uint64_t mask_b1 = (1 << b1) - 1;
    uint64_t mask_b2 = (1 << b2) - 1;

    uint64_t tmp;

    tmp = item.occ.jumpback & mask_b0;
    tmp = tmp << (KLS_COMPRESSED_PAYLOAD_WIDTH - b0);
    *res = *res | tmp;

    tmp = item.occ.line_number & mask_b1;
    tmp = tmp << (KLS_COMPRESSED_PAYLOAD_WIDTH - b0 - b1);
    *res = *res | tmp;

    tmp = item.occ.prev_occurence & mask_b2;
    *res = *res | tmp;
}

int kls_uncompress_bitwise(int b0, int b1, int b2, 
                           KlsItemCompressed item,
                           struct KlsItem* res)
{
    assert(b0 + b1 + b2 == KLS_COMPRESSED_PAYLOAD_WIDTH);
    uint64_t mask_b0 = (uint64_t)((1 << b0) - 1) << (b2 + b1);
    uint64_t mask_b1 = (uint64_t)((1 << b1) - 1) << b2;
    uint64_t mask_b2 = (1 << b2) - 1;

    res->occ.jumpback = (item & mask_b0) >> (b2 + b1);
    res->occ.line_number = (item & mask_b1) >> b2;
    res->occ.prev_occurence = item & mask_b2;
}


int kls_compress_item(struct KlsItem item, 
                      KlsItemCompressed* res)
{
    *res = (uint8_t)item.type;
    *res = *res << KLS_COMPRESSED_PAYLOAD_WIDTH;

    switch (item.type)
    {
    case KLS_ITEM_UNKNOWN:
        break;
    case KLS_ITEM_FILE_START:
        *res = *res | item.kfs.file_index;
        break;
    case KLS_ITEM_OCC_16_22_22:
        kls_compress_bitwise(16, 22, 22, item, res);
        break;
    case KLS_ITEM_OCC_16_16_28:
        kls_compress_bitwise(16, 16, 28, item, res);
        break;
    case KLS_ITEM_OCC_8_26_26:
        kls_compress_bitwise(8, 26, 26, item, res);
        break;
    case KLS_ITEM_OCC_8_16_36:
        kls_compress_bitwise(8, 16, 36, item, res);
        break;
    case KLS_ITEM_OCC_8_36_16:
        kls_compress_bitwise(8, 36, 16, item, res);
        break;
    case KLS_ITEM_OCC_0_30_30:
        kls_compress_bitwise(0, 30, 30, item, res);
        break;
    case KLS_ITEM_OCC_0_16_44:
        kls_compress_bitwise(0, 16, 44, item, res);
        break;
    case KLS_ITEM_OCC_0_44_16:
        kls_compress_bitwise(0, 44, 16, item, res);
        break;
    }
}

int kls_uncompress_item(KlsItemCompressed item, struct KlsItem* res)
{
    uint8_t t = (uint8_t)(item >> KLS_COMPRESSED_PAYLOAD_WIDTH);
    res->type = (enum KlsItemType)t;
    // TODO: return !=0 if type too large
    switch (res->type)
    {
    case KLS_ITEM_UNKNOWN:
        break;
    case KLS_ITEM_FILE_START:
        res->kfs.file_index = (uint32_t)item;
        break;

    case KLS_ITEM_OCC_16_22_22:
        kls_uncompress_bitwise(16, 22, 22, item, res);
        break;
    case KLS_ITEM_OCC_16_16_28:
        kls_uncompress_bitwise(16, 16, 28, item, res);
        break;
    case KLS_ITEM_OCC_8_26_26:
        kls_uncompress_bitwise(8, 26, 26, item, res);
        break;
    case KLS_ITEM_OCC_8_16_36:
        kls_uncompress_bitwise(8, 16, 36, item, res);
        break;
    case KLS_ITEM_OCC_8_36_16:
        kls_uncompress_bitwise(8, 36, 16, item, res);
        break;
    case KLS_ITEM_OCC_0_30_30:
        kls_uncompress_bitwise(0, 30, 30, item, res);
        break;
    case KLS_ITEM_OCC_0_16_44:
        kls_uncompress_bitwise(0, 16, 44, item, res);
        break;
    case KLS_ITEM_OCC_0_44_16:
        kls_uncompress_bitwise(0, 44, 16, item, res);
        break;
    }
    return 0;
}


int kls_compress(uint32_t max_jumpback, uint64_t line_number,
                 uint64_t prev_occurence, KlsItemCompressed* res)
{
    // TODO: optimize for better performance
    struct KlsItem item;
    item.occ.line_number = line_number;
    item.occ.prev_occurence = prev_occurence;

    int b1 = 0, b2 = 0;
    while (line_number > 0) 
    {
        b1++;
        line_number = line_number >> 1;
    }
    while (prev_occurence > 0) 
    {
        b2++;
        prev_occurence = prev_occurence >> 1;
    }

    if (b2 > 44)
        return KLS_COMPRESS_PREV_OCCURENCE_TOO_LARGE;
    if (b1 > 44)
        return KLS_COMPRESS_LINE_NUMBER_TOO_LARGE;

    uint32_t jmask = 0xff;

    if (b1 <= 22 && b2 <= 22) 
    {
        item.type = KLS_ITEM_OCC_16_22_22;
        jmask = 0xffff;
    }
    else if (b1 <= 16 && b2 <= 28) 
    {
        item.type = KLS_ITEM_OCC_16_16_28;
        jmask = 0xffff;
    }
    else if (b1 <= 26 && b2 <= 26)
        item.type = KLS_ITEM_OCC_8_26_26;
    else if (b1 <= 16 && b2 <= 36)
        item.type = KLS_ITEM_OCC_8_16_36;
    else if (b1 <= 36 && b2 <= 16)
        item.type = KLS_ITEM_OCC_8_36_16;
    else if (b1 <= 30 && b2 <= 30)
        item.type = KLS_ITEM_OCC_0_30_30;
    else if (b1 <= 16 && b2 <= 44)
        item.type = KLS_ITEM_OCC_0_16_44;
    else if (b1 <= 44 && b2 <= 16)
        item.type = KLS_ITEM_OCC_0_44_16;
    else 
        return KLS_COMPRESS_PO_AND_LN_TOO_LARGE;


    item.occ.jumpback = max_jumpback > jmask ? jmask : max_jumpback;
    
    kls_compress_item(item, res);
    return 0;
}


int kls_compress_file_start(uint32_t file_index,
                            KlsItemCompressed* res) 
{
    struct KlsItem item;
    memset(&item, 0, sizeof(struct KlsItem));
    item.type = KLS_ITEM_FILE_START;
    item.kfs.file_index = file_index;
    return kls_compress_item(item, res);
}

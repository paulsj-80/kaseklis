#include <stdio.h>
#include "utils.h"
#include "htable.h"


int test_htable()
{
    {
        struct t_kls_ht_context ht;
        kls_ht_create(&ht, 10);

        t_occ_id tmp;

        KLS_ASSERT(kls_ht_get(&ht, "kid0") == 0, 
                   "should not be present");

        kls_ht_put(&ht, "kid0", 11, &tmp, 1000);

        KLS_ASSERT(kls_ht_get(&ht, "kid0") != 0, "should be present");

        kls_ht_put(&ht, "uid1", 12, &tmp, 1000);
        kls_ht_put(&ht, "zid7", 12, &tmp, 1000);
        kls_ht_put(&ht, "id7", 12, &tmp, 1000);
        KLS_ASSERT(tmp == 0, "prev occ id should be 0");
        kls_ht_put(&ht, "id7", 17, &tmp, 1000);
        KLS_ASSERT(tmp == 12, "prev occ id should be 12");
        
        kls_ht_dump(&ht, 0);
        kls_ht_dump_stats(&ht);

        kls_ht_destroy(&ht);

    }
    {
        struct t_kls_ht_context ht;
        kls_ht_create(&ht, 1024 * 1024);

        t_occ_id tmp;

        KLS_ASSERT(kls_ht_get(&ht, "kid0") == 0, 
                   "should not be present");

        kls_ht_put(&ht, "kid0", 11, &tmp, 1000);

        KLS_ASSERT(kls_ht_get(&ht, "kid0") != 0, "should be present");

        kls_ht_put(&ht, "uid1", 12, &tmp, 1000);
        kls_ht_put(&ht, "zid7", 12, &tmp, 1000);
        kls_ht_put(&ht, "id7", 12, &tmp, 1000);
        KLS_ASSERT(tmp == 0, "prev occ id should be 0");
        kls_ht_put(&ht, "id7", 17, &tmp, 1000);
        KLS_ASSERT(tmp == 12, "prev occ id should be 12");
        
        kls_ht_dump_stats(&ht);

        kls_ht_destroy(&ht);

    }
}

int main(int argc, char** arg)
{
    return test_htable();
}

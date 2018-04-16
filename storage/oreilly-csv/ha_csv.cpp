//
// Created by c6s on 18-4-16.
//

#include "ha_csv.h"

ha_csv::ha_csv(handlerton *ht_arg, TABLE_SHARE *share_arg) : handler(ht_arg, share_arg) {}

int ha_csv::close(void) {
    return 0;
}

int ha_csv::rnd_next(uchar *buf) {
    return 0;
}

int ha_csv::rnd_pos(uchar *buf, uchar *pos) {
    return 0;
}

void ha_csv::position(const uchar *record) {

}

int ha_csv::info(uint uint1) {
    return 0;
}

const char *ha_csv::table_type() const {
    return nullptr;
}

const char **ha_csv::bas_ext() const {
    return nullptr;
}

ulong ha_csv::index_flags(uint idx, uint part, bool all_parts) const {
    return 0;
}

THR_LOCK_DATA **ha_csv::store_lock(THD *thd, THR_LOCK_DATA **to, enum thr_lock_type lock_type) {
    return nullptr;
}

int ha_csv::open(const char *name, int mode, uint test_if_locked) {
    return 0;
}

handler::Table_flags ha_csv::table_flags(void) const {
    return 0;
}

int ha_csv::create(const char *name, TABLE *form, HA_CREATE_INFO *info) {
    return 0;
}

int ha_csv::fetch_line(uchar *buf) {
    return 0;
}

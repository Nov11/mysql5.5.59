//
// Created by c6s on 18-4-16.
//
// This file originates from Oreilly's  <Understanding MySQL Internals> chapter 7
// Modification is made since code from that book is of version MySQL 5.1 differs from current code base which is 5.5

#ifndef MYSQL_HA_CSV_H
#define MYSQL_HA_CSV_H

#include <mysql_version.h>
#include "handler.h"
#include <sql_string.h>

const int CSV_READ_BLOCK_SIZE = 512;

struct CSV_INFO {
  char fname[FN_REFLEN + 1]{0};
  int fd = -1;
};

class ha_csv : public handler {

  /* Low-level storage engine data. */
  CSV_INFO *file = nullptr;

  /* Lock structures for the table lock manager. */
  THR_LOCK_DATA lock;
  THR_LOCK thr_lock;

  /* Table scan cursor.*/
  my_off_t pos;

  /* Buffer for reading CSV line blocks. */
  char read_buf[CSV_READ_BLOCK_SIZE];

  /* Buffer for parsing the field values. */
  String field_buf;

  /* read line into the caller-provided record buffer */
  int fetch_line(uchar *buffer);
 public:
  ha_csv(handlerton *ht_arg, TABLE_SHARE *share_arg);

  int close(void) override;

  int rnd_next(uchar *buf) override;

  int rnd_pos(uchar *buf, uchar *pos) override;

  void position(const uchar *record) override;

  int info(uint flags) override;

  const char *table_type() const override;

  const char **bas_ext() const override;

  ulong index_flags(uint idx, uint part, bool all_parts) const override;

  THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to, enum thr_lock_type lock_type) override;

 private:
  int open(const char *name, int mode, uint test_if_locked) override;

  /* Initializes the storage engine object for a sequential scan. */
  int rnd_init(bool scan) override {
    pos = 0;
    stats.records = 0;
    return 0;
  }

  Table_flags table_flags(void) const override;

  int create(const char *name, TABLE *form, HA_CREATE_INFO *info) override;

};

#endif //MYSQL_HA_CSV_H

//
// Created by c6s on 18-4-16.
//

#include <sql_class.h>
#include "ha_csv.h"
#include "table.h"
#include "field.h"

ha_csv::ha_csv(handlerton *ht_arg, TABLE_SHARE *share_arg) : handler(ht_arg, share_arg) {}

int ha_csv::close(void) {
  /*
     Clean up the lock structures, close the file handle, and
     deallocate the data file descriptor memory.
   */
  thr_lock_delete(&thr_lock);
  if (file) {
    if (file->fd >= 0)
      my_close(file->fd, MYF(0));
    my_free(file);
    file = 0;
  }
  return 0;
}

/* Called once for each record during a sequential table scan. */
int ha_csv::rnd_next(uchar *buf) {
  /*
   Increment the global statistics counter displayed by SHOW STATUS
   under Handler_read_rnd_next.
 */
  ha_statistic_increment(&SSV::ha_read_rnd_next_count);

  /* fetch_line() does the actual work. */
  int error = fetch_line(buf);

  /*
    On success, update our estimate for the total number of records in the
    table.
  */
  if (!error)
    stats.records++;

  /* Return whatever code we got from fetch_line() to the caller. */
  return error;
}

/*
  Positions the scan cursor at the position specified by set_pos and
  read the record at that position. Used in GROUP BY and ORDER BY
  optimization when the "filesort" technique is applied.
*/
int ha_csv::rnd_pos(uchar *buf, uchar *set_pos) {
  ha_statistic_increment(&SSV::ha_read_rnd_count);
  pos = my_get_ptr(set_pos, ref_length);
  return fetch_line(buf);
}

/*
  Stores the "position" reference to the current record in the ref
  variable. At this point, this method is called in situation that are
  impossible for this storage engine, but this could change in the future.
 */
void ha_csv::position(const uchar *record) {
  my_store_ptr(ref, ref_length, pos);
}

/* Updates the statistical variables in the handler object */
int ha_csv::info(uint flags) {
  /*
    The optimizer must never think that the table has fewer than
    two records unless this is indeed the case. Reporting a smaller number
    makes the optimizer assume it needs to read no more than one record from
    the table. Our storage engine does always know the number of records,
    and in many cases cannot even make a good guess. To be safe and to keep
    things simple, we always report that we have at least 2 records.
  */
  if (stats.records < 2)
    stats.records = 2;

  /*
    The rest of the variables merely appear in SHOW TABLE STATUS output and
    do not affect the optimizer. For the purpose of this example they can
    be set to 0.
   */

  stats.deleted = 0;
  errkey = 0;
  stats.mean_rec_length = 0;
  stats.data_file_length = 0;
  stats.index_file_length = 0;
  stats.max_data_file_length = 0;
  stats.delete_length = 0;
  if (flags & HA_STATUS_AUTO)
    stats.auto_increment_value = 1;
  return 0;
}

/**
 * for display purpose
 * @return
 */
const char *ha_csv::table_type() const {
  return "oreilly's csv storage engine";
}

static const char *csv_ext[] = {".mycsv", 0};
const char **ha_csv::bas_ext() const {
  return csv_ext;
}

ulong ha_csv::index_flags(uint idx, uint part, bool all_parts) const {
  return 0;
}

THR_LOCK_DATA **ha_csv::store_lock(THD *thd, THR_LOCK_DATA **to, enum thr_lock_type lock_type) {
  if (lock_type != TL_IGNORE && lock.type == TL_UNLOCK)
    lock.type = lock_type;

  *to++ = &lock;
  return to;
}

int ha_csv::open(const char *name, int mode, uint test_if_locked) {
  /* Initialize the lock structures used by the lock manager. */
  thr_lock_init(&thr_lock);
  thr_lock_data_init(&thr_lock, &lock, NULL);

  /* Allocate memory for the data file descriptor. */
  file = (CSV_INFO *) my_malloc(sizeof(CSV_INFO), MYF(MY_WME));
  if (!file)
    return 1;

  /* Translate the name of the parameter 'name' into the data file name. */
  fn_format(file->fname, name, "", ".mycsv",
            MY_REPLACE_EXT | MY_UNPACK_FILENAME);

  /*
     Open the file, and save the file handle id in the data
     file descriptor structure.
   */
  if ((file->fd = my_open(file->fname, mode, MYF(0))) < 0) {
    int error = my_errno;
    close();
    return error;
  }

  /* Read operations start from the beginning of the file. */
  pos = 0;
  return 0;
}

/*
   We need this function to report that the records member cannot be used
   to optimize SELECT COUNT(*) without a WHERE clause. Note that the value
   records actually shows the correct count after a full scan, and can
   indeed be used to optimize SELECT COUNT(*). This is left as an exercise
   for the reader.
 */
handler::Table_flags ha_csv::table_flags(void) const {
//  return HA_NOT_EXACT_COUNT; <- this macro is not valid any more. I cannot find an equivalent
  return HA_NO_TRANSACTIONS;
}

/* Nothing special to do on the storage engine level when the table
   is created. The .CSV file is placed externally into the data directory.
*/
int ha_csv::create(const char *name, TABLE *form, HA_CREATE_INFO *info) {
  return 0;
}

int ha_csv::fetch_line(uchar *buffer) {
  /*
     Keep track of the current offset in the file as we read
     portions of the line into a buffer.
     Start at the current read cursor position.
   */
  my_off_t cur_pos = pos;

  /*
     We will use this to iterate through the array of
     table field pointers to store the parsed data in the right
     place and the right format.
   */
  Field **field = table->field;

  /*
    Used in parsing to remember the previous character. The impossible
    value of 256 indicates that the last character either did not exist
    (we are on the first one), or its value is irrelevant.
  */
  int last_c = 256;

  /* Set to 1 if we are inside a quoted string. */
  int in_quote = 0;

  /* How many bytes we have seen so far in this line. */
  uint bytes_parsed = 0;

  /* Loop breaker flag. */
  int line_read_done = 0;

  /* Truncate the field value buffer. */
  field_buf.length(0);

  /* Attempt to read a whole line. */
  for (; !line_read_done;) {
    /* Read a block into a local buffer and deal with errors. */
    uchar buf[CSV_READ_BLOCK_SIZE];
    size_t bytes_read = my_pread(file->fd, buf, sizeof(buf), cur_pos, MYF(MY_WME));
    if (bytes_read == MY_FILE_ERROR)
      return HA_ERR_END_OF_FILE;
    if (!bytes_read)
      return HA_ERR_END_OF_FILE;

    /*
      If we reach this point, the read was successful. Start parsing the
      data we have read.
    */
    uchar *p = buf;
    uchar *buf_end = buf + bytes_read;

    /* For each byte in the buffer. */
    for (; p < buf_end;) {
      char c = *p;
      int end_of_line = 0;
      int end_of_field = 0;
      int char_escaped = 0;

      switch (c) {
        /*
          A double-quote marks the start or the end of a quoted string
          unless it has been escaped.
        */
        case '"':
          if (last_c == '"' || last_c == '\\') {
            field_buf.append(c);
            char_escaped = 1;

            /*
              When we see the first quote, in_quote will get flipped.
              A subsequent quote, however, tells us we are still inside the
              quoted string.
            */
            if (last_c == '"')
              in_quote = 1;
          } else
            in_quote = !in_quote;
          break;
          /*
            Treat the backward slash as an escape character.
          */
        case '\\':
          if (last_c == '\\') {
            field_buf.append(c);
            char_escaped = 1;
          }
          break;

          /*
            Set the termination flags on end-of-line unless it is quoted.
          */
        case '\r':
        case '\n':
          if (in_quote) {
            field_buf.append(c);
          } else {
            end_of_line = 1;
            end_of_field = 1;
          }
          break;

          /* Comma signifies end-of-field unless quoted. */
        case ',':
          if (in_quote) {
            field_buf.append(c);
          } else
            end_of_field = 1;
          break;

          /*
            Regular charcters just get appended to the field
            value buffer.
          */
        default:field_buf.append(c);
          break;
      }

      /*
        If at the end a field, and a matching field exists in the table
        (it may not if the CSV file has extra fields), transfer the field
        value buffer contents into the corresponding Field object. This
        actually takes care of initializing the correct parts of the buffer
        argument passed to us by the caller. The internal convention of the
        optimizer dictates that the buffer pointers of the Field objects
        must already be set up to point at the correct areas of the buffer
        argument prior to calls to the data-retrieval methods of the handler
        class.
      */
      if (end_of_field && *field) {
        auto f = *field;
        bitmap_set_bit(f->table->write_set, f->field_index);
        (*field)->store(field_buf.ptr(), field_buf.length(),
                        system_charset_info);
        field++;
        field_buf.length(0);
      }

      /*
        Special case - a character that was escaped itself should not be
        regared as an escape character.
      */
      if (char_escaped)
        last_c = 256;
      else
        last_c = c;
      p++;

      /* Prepare for loop exit on end-of-line. */
      if (end_of_line) {
        if (c == '\r')
          p++;
        line_read_done = 1;
        in_quote = 0;
        break;
      }
    }

    /* Block read/parse cycle is complete - update the counters. */
    bytes_parsed += (p - buf);
    cur_pos += bytes_read;
  }

  /*
    Now we are done with the line read/parsing. We still have a number
    of small tasks left to complete the job.
  */

  /* Initialize the NULL indicator flags in the record. */
  memset(buffer, 0, table->s->null_bytes);

  /*
    The parsed line may not have had the values of all of the fields.
    Set the remaining fields to their default values.
   */
  for (; *field; field++) {
    (*field)->set_default();
  }

  /* Move the cursor to the next record. */
  pos += bytes_parsed;

  /* Report success. */
  return 0;
}

handlerton *mycsv_hton;

static handler *mycsv_create_handler(handlerton *hton,
                                     TABLE_SHARE *table,
                                     MEM_ROOT *mem_root) {
  return new(mem_root) ha_csv(hton, table);
}

static int mycsv_init(void *p) {
  mycsv_hton = (handlerton *) p;

  mycsv_hton->state = SHOW_OPTION_YES;
  mycsv_hton->create = mycsv_create_handler;
  mycsv_hton->flags = HTON_CAN_RECREATE;

  return 0;
}

static int mycsv_done(void *p) {
  return 0;
}

/* Defines the global structure for the plugin. */
/*struct st_mysql_plugin
{
  int type;             *//* the plugin type (a MYSQL_XXX_PLUGIN value)   *//*
  void *info;           *//* pointer to type-specific plugin descriptor   *//*
  const char *name;     *//* plugin name                                  *//*
  const char *author;   *//* plugin author (for I_S.PLUGINS)              *//*
  const char *descr;    *//* general descriptive text (for I_S.PLUGINS)   *//*
  int license;          *//* the plugin license (PLUGIN_LICENSE_XXX)      *//*
  int (*init)(void *);  *//* the function to invoke when plugin is loaded *//*
  int (*deinit)(void *);*//* the function to invoke when plugin is unloaded *//*
  unsigned int version; *//* plugin version (for I_S.PLUGINS)             *//*
  struct st_mysql_show_var *status_vars;
  struct st_mysql_sys_var **system_vars;
  void * __reserved1;   *//* reserved for dependency checking             *//*
  unsigned long flags;  *//* flags for plugin *//*
};*/
int invaliddata;
mysql_declare_plugin(my_csv)
        {
            MYSQL_STORAGE_ENGINE_PLUGIN,
            &invaliddata,
            "mycsv",
            "author:based on oreilly's mysql interals/ source code is modified to adopt them into version 5.5 from 5.1",
            "description: useless storage engine",
            PLUGIN_LICENSE_GPL,
            mycsv_init,
            mycsv_done,
            0x0001,
            nullptr,
            nullptr,
            nullptr,
            0
        }mysql_declare_plugin_end;

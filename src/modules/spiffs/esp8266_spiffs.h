/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __ESPBOT_SPIFFS_H__
#define __ESPBOT_SPIFFS_H__

// ESP8266 and SDK references
#include "spiffs_flash_functions.h"
#include "spiffs.h"

// file system possible status
typedef enum
{
  FFS_AVAILABLE = 222,
  FFS_UNMOUNTED,   // gracefully unmounted
  FFS_UNAVAILABLE, // some FATAL error occurred while mounting of formatting
  FFS_ERRORS,      // a file system check found errors
  FFS_NOT_INIT
} flashfs_status;

// file possible status
typedef enum
{
  FFS_F_UNAVAILABLE = 444, // not open yet or fatal error while opening
  FFS_F_OPEN,
  FFS_F_MODIFIED_UNSAVED,
  FFS_F_REMOVED
} flashfs_file_status;


void init(void);    // will mount the file system (eventually formatting it)
void format(void);  // will clean everything, a new init is required after this
void unmount(void); // thinking about a controlled reset of the target
                    // when you want to gracefully shutdown ...
s32_t check();      // same as SPIFFS_check

struct spiffs_dirent *list(int); // (0) => return first file information
                                  // (1) => return next file information

flashfs_status get_status();
bool is_available();

s32_t last_error();
u32_t get_total_size();
u32_t get_used_size();
u32_t get_free_space();

char *get_name();
spiffs_file open(char *); // not sure if really useful but just in case ...
                    // if the file was originally opened with a different filename
                    // it will be closed
                    // then the new filename will be open

void close(spiffs_file fd);

flashfs_file_status get_file_status(); // who cares if you have is_available() ...

int n_read(spiffs_file fd, char *t_buffer, int t_len);   // same as SPIFFS_read
                                          // will read t_len bytes from file to t_buffer
                                          // returns number of bytes read, or -1 if error
int n_append(spiffs_file fd, char *t_buffer, int t_len); // same as SPIFFS_write
                                          // will write t_len bytes from t_buffer to file
                                          // returns number of bytes written, or -1 if error

void clear(spiffs_file fd); // clear the whole file content
void file_remove(spiffs_file fd);
void flush_cache(spiffs_file fd);         // not sure if really useful but just in case ...
bool file_exists(char *); // making it static does not require creating an object before
int file_size(char *);    // making it static does not require creating an object before 

/*  EXAMPLE EXAMPLE EXAMPLE 
    ...
    Flashfs fs;
    fs.init();
    if (fs.is_available)
    {
      Ffile cfg(&fs, "config.cfg"); // constructor will open or create the file
      if (cfg.is_available())
      {
        cfg.n_append("this configuration",19);
      }
    } // destructor will close the file
    ...
*/

#endif
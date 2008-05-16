/** Tag-route follower.
 *
 * Replays a recorded tag route.
 */

#include "base/types.h"
#include "base/core.h"
#include "base/display.h"
#include "base/fs.h"
#include "base/util.h"
#include "base/drivers/avr.h"
#include "base/drivers/systick.h"
#include "base/drivers/motors.h"
#include "base/lib/gui/gui.h"

#define ROUTE_FILE "tag_route.data"

#define TEST_DATA "Bonjour!"
#define DATA_SIZE 300 

void record(char *filename);
void replay(char *filename);

void record(char *filename) {
  fs_fd_t fd;
  fs_err_t err;
  size_t i;

  nx_display_clear();
  nx_display_string("Creating file...\n");

  err = nx_fs_open(filename, FS_FILE_MODE_CREATE, &fd);
  if (err == FS_ERR_FILE_ALREADY_EXISTS) {
    if (nx_gui_text_menu_yesno("Overwrite?")) {
      nx_display_clear();

      nx_fs_open(filename, FS_FILE_MODE_OPEN, &fd);
      if (nx_fs_unlink(fd) != FS_ERR_NO_ERROR) {
        nx_display_string("Erase error.\n");
        return;
      }

      if (nx_fs_open(filename, FS_FILE_MODE_CREATE, &fd) != FS_ERR_NO_ERROR) {
        nx_display_string("Create error.\n");
        return;
      }
    } else {
      nx_display_string("Aborting.\n");
      return;
    }
  }

  nx_display_string("File opened.\n\n");
  nx_display_string("Writing...\n");

  for (i=0; i<DATA_SIZE; i++) {
    err = nx_fs_write(fd, (U8)'e');

    if (err != FS_ERR_NO_ERROR) {
      nx_display_string("Err: ");
      nx_display_uint(err);
      nx_display_end_line();
    }
  }

  nx_display_uint(nx_fs_get_filesize(fd));
  nx_display_string("B written.\n");
  nx_fs_close(fd);
}

void replay(char *filename) {
  fs_fd_t fd;
  fs_err_t err;
  U8 c[DATA_SIZE+1] = {0};
  size_t i=0;

  nx_display_clear();
  nx_display_string("Opening file.\n");

  err = nx_fs_open(filename, FS_FILE_MODE_OPEN, &fd);
  if (err == FS_ERR_FILE_NOT_FOUND) {
    nx_display_string("File not found.\n");
    return;
  } else if (err != FS_ERR_NO_ERROR) {
    nx_display_string("Can't open file!\n");
    return;
  }
  
  nx_display_string("File opened.\n\n");
  nx_display_uint(nx_fs_get_filesize(fd));
  nx_display_string(" bytes.\n");

  while (i <= DATA_SIZE &&
    nx_fs_read(fd, &(c[i++])) == FS_ERR_NO_ERROR);

  nx_display_string("...");
  nx_display_string((char *)(c+DATA_SIZE-5));
  nx_display_string("\nDone.");

  nx_fs_close(fd);
}

void main(void) {
  char *entries[] = {"Record", "Replay", "--", "Halt", NULL};
  gui_text_menu_t menu;
  U8 res;

  /*
  U32 nulldata[EFC_PAGE_WORDS] = {0};
  for (res=128; res<140; res++)
    nx__efc_write_page(nulldata, res);
  */

  menu.entries = entries;
  menu.title = "Tag route";
  menu.active_mark = GUI_DEFAULT_TEXT_MARK;

  while ((res = nx_gui_text_menu(menu)) != 3) {
    switch (res) {
      case 0:
        record(ROUTE_FILE);
        break;
      case 1:
        replay(ROUTE_FILE);
        break;
      default:
        continue;
        break;
    }
    
    nx_display_string("\nOk to go back");
    while (nx_avr_get_button() != BUTTON_OK);
    nx_systick_wait_ms(500);
  }
  
  nx_display_string(">> Halting...");
  nx_systick_wait_ms(1000);
}

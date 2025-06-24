/* Copyright (c) 2023-2025 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include "player.h"             /* player UI APIs */

static gchar dir_path[PATH_MAX];
static gchar file_path[PATH_MAX];
static guint32 current_file_no = 0;
static guint32 last_file_count = 0;
static gboolean wait_for_file_no = FALSE;

/* Private MACRO */
#define MESSAGE_REQUEST_INPUT_FILE_NO()	(LOGI (">> enter file No. to play: "))
#define MESSAGE_REQUEST_INPUT_COMMAND()	(LOGI ("> enter your command ('h' for help): "))

/* Private functions */
void search_in_dir (const gchar * path, const gchar * suffix);
gboolean get_file_path (const gchar * path, const gchar * suffix,
    const guint32 file_no);

/* Returns the current selected/playing file absolute path */
gchar *
get_current_file_path (void)
{
  return file_path;
}

int
is_regular_file (const gchar * path)
{
  struct stat path_stat;
  stat (path, &path_stat);
  return S_ISREG (path_stat.st_mode);
}

int
is_regular_dir (const gchar * path)
{
  struct stat path_stat;
  stat (path, &path_stat);
  return S_ISDIR (path_stat.st_mode);
}

/* Sends the location or file path information to the player for futher referecens */
gboolean
inject_dir_path_to_player (const gchar * path)
{
  if (realpath (path, dir_path) == NULL) {
    printf ("realpath() failed.\n");
    return FALSE;
  }
  if (is_regular_file (dir_path)) {
    sprintf (file_path, "%s", dir_path);
  } else if (!is_regular_dir (dir_path)) {
    LOGE ("%s not a regular file or directory.\n", path);
    return FALSE;
  }
  return TRUE;
}

/* This function will call the search_in_dir() to scan the current dir_path and
filter the file name with the FILE_SUFFIX defined in player.h */
void
update_file_list (void)
{
  /* Show the media file list */
  search_in_dir (dir_path, FILE_SUFFIX);
}

/* Gives a try to get the file path without scan the dir_path */
gboolean
try_to_update_file_path (void)
{
  /* Play if only have one file */
  if (1 == last_file_count) {
    current_file_no = 1;
    if (!get_file_path (dir_path, FILE_SUFFIX, current_file_no)) {
      /* Should not be here */
      g_printerr ("Can not get file path from %s\n", dir_path);
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

gboolean
request_update_file_path (gint32 offset_file)
{
  if (1 == last_file_count) {
    g_print ("No other file to play.\n");
    return FALSE;
  }

  /* Update current file No. */
  if (offset_file < 0) {
    while (offset_file < 0) {
      if ((current_file_no <= 1) || (current_file_no > last_file_count)) {
        current_file_no = last_file_count;
      } else {
        LOGD ("-- previous file.\n");
        --current_file_no;
      }
      ++offset_file;
    }
  } else if (offset_file > 0) {
    while (offset_file > 0) {
      if (current_file_no >= last_file_count) {
        current_file_no = 1;
      } else {
        LOGD ("-- next file.\n");
        ++current_file_no;
      }
      --offset_file;
    }
  } else {
    /* Do nothing with offset_file == 0 */
  }

  /* Get new file path */
  if (!get_file_path (dir_path, FILE_SUFFIX, current_file_no)) {
    g_printerr ("Can not get file path from %s\n", dir_path);
    wait_for_file_no = TRUE;
    return FALSE;
  }
  return TRUE;
}

void
search_in_dir (const gchar * path, const gchar * suffix)
{
  LOGI (">> Your file list - select number to play:\n");

  /* If is a single file */
  if (is_regular_file (path)) {
    /* If this file have suitable suffix */
    if (g_str_has_suffix (path, suffix)) {
      last_file_count = 1;
      LOGI ("- [%d] %s\n", last_file_count, path);
      return;
    }
    return;
  }

  /* If is a path */
  LOGD ("Searching in dir %s\n", path);
  guint32 file_count = 0;
  if (is_regular_dir (path)) {
    DIR *dp;
    struct dirent *ep;
    dp = opendir (path);
    if (dp != NULL) {
      while ((ep = readdir (dp))) {
        if (g_str_has_suffix (ep->d_name, suffix)) {
          ++file_count;
          LOGI ("- [%d] %s\n", file_count, ep->d_name);
        }
      }
      last_file_count = file_count;
      closedir (dp);
    } else {
      /* Should not be here */
      LOGE ("Couldn't open the directory: %s\n", path);
      return;
    }
  } else {
    LOGE ("%s is not a regular path.\n", path);
    return;
  }
  wait_for_file_no = TRUE;
  LOGI ("current selected file [%d]: %s\n", current_file_no,
      basename (file_path));
}

gboolean
get_file_path (const char *path, const char *suffix, const guint32 file_no)
{
  /* Check the file_no is in the acceptable range */
  if ((file_no > last_file_count) || (0 == file_no)) {
    LOGE ("Incorrect file No.%d\n", file_no);
    return FALSE;
  }

  /* If is a single file */
  if (is_regular_file (path)) {
    /* If this file have suitable suffix */
    if (g_str_has_suffix (path, suffix)) {
      sprintf (file_path, "%s", dir_path);
      current_file_no = file_no;
      LOGD ("-- selected [%d] %s\n", current_file_no, file_path);
      return TRUE;
    }
    LOGE ("%s can not be play.\n", path);
    return FALSE;
  }

  /* Get the file_path corresponding to the file_no */
  guint32 file_count = 0;
  gchar relative_path[PATH_MAX + 1];
  if (is_regular_dir (path)) {
    DIR *dp;
    struct dirent *ep;
    dp = opendir (path);
    if (dp != NULL) {
      while ((ep = readdir (dp))) {
        if ((g_str_has_suffix (ep->d_name, suffix))) {
          ++file_count;
          if (file_no == file_count) {
            sprintf (relative_path, "%s//%s", path, ep->d_name);
            char *res = realpath (relative_path, file_path);
            if (res) {
              current_file_no = file_no;
              LOGD ("-- selected [%d] %s\n", current_file_no, file_path);
            } else {
              LOGE ("Your selected file is deleted.\n");
              closedir (dp);
              return FALSE;
            }
          }
        }
      }
      closedir (dp);
    } else {
      /* Should not be here */
      LOGE ("Couldn't open the directory: %s\n", path);
      return FALSE;
    }
  } else {
    /* Should not be here */
    LOGE ("This is not a directory!\n");
    return FALSE;
  }
  return TRUE;
}

/* Function will base on the wait_for_file_no flag to request user input command or file number to play.
This function will block until the user input command or type enter after input the file number*/
UserCommand
get_user_command (void)
{
  UserCommand ret = INVALID;
  if (!wait_for_file_no) {
    /* Get char */
    ssize_t read = 0;
    MESSAGE_REQUEST_INPUT_COMMAND ();

    if (system ("/bin/stty raw") != 0) {
      printf ("system() failed.\n");
      return INVALID;
    }
    read = getchar ();
    if (system ("/bin/stty cooked") != 0) {
      printf ("system() failed.\n");
      return INVALID;
    }
    LOGI ("\n");

    switch (read) {
      case 'q':
        ret = QUIT;
        break;
      case 'r':
        ret = REPLAY;
        break;
      case ' ':                /* SPACE */
        ret = PAUSE_PLAY;
        break;
      case 's':
        ret = STOP;
        break;
      case '.':
        ret = FORWARD;
        break;
      case ',':
        ret = REWIND;
        break;
      case 'h':
        ret = HELP;
        break;
      case 'l':
        ret = LIST;
        break;
      case '[':
        ret = PREVIOUS;
        break;
      case ']':
        ret = NEXT;
        break;
      default:
        ret = INVALID;
        break;
    }
  } else {
    /* Get line */
    char *line = NULL;
    size_t len = 0;
    ssize_t _read = 0;
    int file_no = 0;
    MESSAGE_REQUEST_INPUT_FILE_NO ();
    _read = getline (&line, &len, stdin);
    if (_read) {
      file_no = atoi (line);
      wait_for_file_no = FALSE;
      if ((0 == file_no) || (file_no > last_file_count)) {
        LOGI ("Invalid number\n");
        ret = INVALID;
      } else {
        current_file_no = file_no;
        ret = PLAYFILE;
      }
    } else {
      ret = INVALID;
    }
  }
  return ret;
}

void
print_supported_command (void)
{
  LOGI ("Supported commands:\n");
  LOGI ("+    r    : Replay\n");
  LOGI ("+ <SPACE> : Pause/Play\n");
  LOGI ("+    s    : Stop\n");
  LOGI ("+    .    : Forward\n");
  LOGI ("+    ,    : Rewind\n");
  LOGI ("+    h    : Help\n");
  LOGI ("+    q    : Quit\n");
  LOGI ("+    l    : List\n");
  LOGI ("+    [    : Previous\n");
  LOGI ("+    ]    : Next\n");
}

void
print_current_selected_file (gboolean refresh_console_message)
{
  if (refresh_console_message) {
    if (!wait_for_file_no) {
      if (system ("/bin/stty cooked") != 0) {
        printf ("system() failed.\n");
      }
    }
    LOGI ("AUTO PLAY NEXT FILE\n");
  }

  LOGI ("Now playing file [%d]: %s\n", current_file_no, basename (file_path));

  if (refresh_console_message) {
    if (wait_for_file_no) {
      update_file_list ();
      MESSAGE_REQUEST_INPUT_FILE_NO ();
    } else {
      MESSAGE_REQUEST_INPUT_COMMAND ();
      if (system ("/bin/stty raw") != 0) {
        printf ("system() failed.\n");
      }
    }
  }
}

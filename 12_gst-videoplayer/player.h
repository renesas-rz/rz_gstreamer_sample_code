/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include <gst/gst.h>
#include <stdbool.h>            /* bool type */
#include <stdio.h>              /* getline() API */
#include <stdlib.h>             /* free(), atoi() API */
#include <sys/stat.h>           /* stat(), struct stat */
#include <dirent.h>             /* struct dirent */
#include <libgen.h>             /* basename */
#include <limits.h>             /* PATH_MAX */

//#define DEBUG_LOG
#ifdef DEBUG_LOG
#define LOGD(...)	g_print(__VA_ARGS__)
#else
#define LOGD(...)
#endif

#define LOGI(...)	g_print(__VA_ARGS__)
#define LOGE(...)	g_printerr(__VA_ARGS__)

#define FILE_SUFFIX		".mp4"

typedef enum tag_user_command
{
  INVALID = 0,
  REPLAY,
  PAUSE_PLAY,
  STOP,
  FORWARD,
  REWIND,
  HELP,
  QUIT,
  LIST,
  PREVIOUS,
  NEXT,
  PLAYFILE,
} UserCommand;

gchar *get_current_file_path (void);

gboolean inject_dir_path_to_player (const gchar * path);
void update_file_list (void);
gboolean try_to_update_file_path (void);
gboolean request_update_file_path (gint32 offset_file);

UserCommand get_user_command (void);
void print_supported_command (void);
void print_current_selected_file (gboolean refresh_console_message);

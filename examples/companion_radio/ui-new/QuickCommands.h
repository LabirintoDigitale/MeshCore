#pragma once

#include <stdint.h>

// Quick commands (UI_QUICK_COMMANDS): a "Comandi" page in the device menu.
// Each command slot has its own text and its own list of companion recipients,
// both edited on the device (Config). Selecting a command sends its text as a
// direct message to its recipients, as if it had been typed in the app.
// Slots are kept on the internal flash, so they survive a reboot.

#ifndef UI_QUICK_MAX_CMDS
  #define UI_QUICK_MAX_CMDS        10
#endif

#ifndef UI_QUICK_MAX_RECIPIENTS
  #define UI_QUICK_MAX_RECIPIENTS   8    // per command
#endif

#ifndef UI_QUICK_TEXT_LEN
  #define UI_QUICK_TEXT_LEN        48    // including the terminating null
#endif

// texts pre-loaded in the first slots the first time (no recipients), all editable on the device
#ifndef UI_QUICK_COMMAND_LIST
  #define UI_QUICK_COMMAND_LIST  "status", "ping"
#endif

#define UI_QUICK_KEY_LEN          6      // recipients are stored as public key prefixes
#define UI_QUICK_CMDS_FILE        "/ui_qcmds"
#define UI_QUICK_CMDS_FILE_VER    1

struct QuickCmdSlot {
  char text[UI_QUICK_TEXT_LEN];
  uint8_t n_dest;
  uint8_t dest[UI_QUICK_MAX_RECIPIENTS][UI_QUICK_KEY_LEN];
};

#pragma once

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/SensorManager.h>
#include <helpers/MultiSerialInterface.h>
#include <Arduino.h>
#include <helpers/sensors/LPPDataHelpers.h>

#ifndef LED_STATE_ON
  #define LED_STATE_ON 1
#endif

#ifdef PIN_BUZZER
  #include <helpers/ui/buzzer.h>
#endif
#ifdef PIN_VIBRATION
  #include <helpers/ui/GenericVibration.h>
#endif

#include "../AbstractUITask.h"
#include "../NodePrefs.h"

#ifdef UI_QUICK_COMMANDS
  #include "QuickCommands.h"
#endif

#ifdef UI_DEVICE_CONFIG
  #define UI_AUDIO_FILE       "/ui_audio"
  #define UI_AUDIO_FILE_VER   1
  #define UI_CHANNEL_KEY_LEN  6     // channels are identified by a prefix of their secret

  struct UIAudioPrefs {
    uint8_t beep_channels;   // beep on channel messages
    uint8_t beep_private;    // beep on direct messages
    uint8_t n_muted;         // channels excluded from the channel beep
    uint8_t muted[MAX_GROUP_CHANNELS][UI_CHANNEL_KEY_LEN];
  };
#endif

class UITask : public AbstractUITask {
  DisplayDriver* _display;
  SensorManager* _sensors;
#ifdef PIN_BUZZER
  genericBuzzer buzzer;
#endif
#ifdef PIN_VIBRATION
  GenericVibration vibration;
#endif
  unsigned long _next_refresh, _auto_off;
  NodePrefs* _node_prefs;
  char _alert[80];
  unsigned long _alert_expiry;
  int _msgcount;
  unsigned long ui_started_at, next_batt_chck;
  int next_backlight_btn_check = 0;
#ifdef PIN_STATUS_LED
  int led_state = 0;
  int next_led_change = 0;
  int last_led_increment = 0;
#endif

#ifdef PIN_USER_BTN_ANA
  unsigned long _analogue_pin_read_millis = millis();
#endif

  UIScreen* splash;
  UIScreen* home;
  UIScreen* msg_preview;
  UIScreen* curr;
#ifdef UI_QUICK_COMMANDS
  UIScreen* quick_cmd;      // send list
  UIScreen* quick_config;   // list of slots
  UIScreen* quick_slot;     // edit one slot
  UIScreen* quick_dest;     // pick recipients of a slot
  UIScreen* quick_text;     // on-screen keyboard for the text of a slot
  QuickCmdSlot _qc[UI_QUICK_MAX_CMDS];
  bool _qc_loaded;

  void beginQuickCommands();
  void loadQuickCmds();
#endif
#ifdef UI_DEVICE_CONFIG
  UIScreen* device_cfg;     // sections
  UIScreen* audio_cfg;      // audio settings
  UIScreen* channel_beep;   // per-channel beep selection
  UIAudioPrefs _audio;
  bool _audio_loaded;

  void beginDeviceConfig();
  void loadAudioPrefs();
  bool isChannelMuted(const uint8_t* secret);
#endif

  void userLedHandler();

  // Button action handlers
  char checkDisplayOn(char c);
  char handleLongPress(char c);
  char handleDoubleClick(char c);
  char handleTripleClick(char c);

  void setCurrScreen(UIScreen* c);

public:

  UITask(mesh::MainBoard* board, MultiSerialInterface* serial) : AbstractUITask(board, serial), _display(NULL), _sensors(NULL) {
    next_batt_chck = _next_refresh = 0;
    ui_started_at = 0;
    curr = NULL;
#ifdef UI_QUICK_COMMANDS
    quick_cmd = quick_config = quick_slot = quick_dest = quick_text = NULL;
    _qc_loaded = false;
#endif
#ifdef UI_DEVICE_CONFIG
    device_cfg = audio_cfg = channel_beep = NULL;
    _audio_loaded = false;
#endif
  }
  void begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs);

  void gotoHomeScreen() { setCurrScreen(home); }
  void showAlert(const char* text, int duration_millis);
  int  getMsgCount() const { return _msgcount; }
  bool hasDisplay() const { return _display != NULL; }
  bool isButtonPressed() const;

  bool isBuzzerQuiet() { 
#ifdef PIN_BUZZER
    return buzzer.isQuiet();
#else
    return true;
#endif
  }

  void toggleBuzzer();
  bool getGPSState();
  void toggleGPS();

#ifdef UI_QUICK_COMMANDS
  void gotoQuickCmdScreen(bool reset = true);
  void gotoQuickConfigScreen(bool reset = true);
  void gotoQuickSlotScreen(int slot, bool reset = true);
  void gotoQuickDestScreen(int slot);
  void gotoQuickTextScreen(int slot);

  QuickCmdSlot* getQuickCmd(int slot);
  int  getQuickCmdCount();                                      // slots with a text
  void saveQuickCmds();
  bool isQuickRecipient(int slot, const uint8_t* pub_key);
  int  toggleQuickRecipient(int slot, const uint8_t* pub_key);  // 1 = added, 0 = removed, -1 = list full
  int  sendQuickCommand(int slot);                              // returns number of messages queued
  void onUIMsgAck(const char* contact_name) override;
#endif

#ifdef UI_DEVICE_CONFIG
  void gotoDeviceConfigScreen(bool reset = true);
  void gotoAudioConfigScreen(bool reset = true);
  void gotoChannelBeepScreen();

  UIAudioPrefs* getAudioPrefs();
  void saveAudioPrefs();
  bool isChannelBeepOn(int channel_idx);
  void toggleChannelBeep(int channel_idx);
  void notifyChannelMsg(uint8_t channel_idx) override;
#endif


  // from AbstractUITask
  void msgRead(int msgcount) override;
  void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) override;
  void notify(UIEventType t = UIEventType::none) override;
  void loop() override;

  void shutdown(bool restart = false);
};

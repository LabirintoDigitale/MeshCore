#ifdef UI_DEVICE_CONFIG

#include "UITask.h"
#include "../MyMesh.h"
#include "target.h"
#include "UIList.h"

/* ---------------------------------------------------------------------------------
   "L1 Config": sections
   --------------------------------------------------------------------------------- */

class DeviceCfgScreen : public UIScreen {
  UITask* _task;
  int _sel, _top;
  enum { ITEM_AUDIO, ITEM_COUNT };

public:
  DeviceCfgScreen(UITask* task) : _task(task), _sel(0), _top(0) { }
  void reset() { _sel = _top = 0; }

  int render(DisplayDriver& display) override {
    drawListHeader(display, "L1 Config", NULL);
    int y = LIST_ROW_Y0;
    for (int i = _top; i < ITEM_COUNT && i < _top + LIST_VISIBLE; i++, y += LIST_ROW_HEIGHT) {
      switch (i) {
        case ITEM_AUDIO: drawListRow(display, y, i == _sel, 2, "Audio"); break;
      }
    }
    return 5000;
  }

  bool handleInput(char c) override {
    switch (listInput(c, _sel, _top, ITEM_COUNT)) {
      case LIST_MOVED: return true;
      case LIST_BACK:
        _task->gotoHomeScreen();
        return true;
      case LIST_SELECT:
        if (_sel == ITEM_AUDIO) _task->gotoAudioConfigScreen();
        return true;
      default:
        return false;
    }
  }
};

/* ---------------------------------------------------------------------------------
   Audio
   --------------------------------------------------------------------------------- */

class AudioCfgScreen : public UIScreen {
  UITask* _task;
  int _sel, _top;
  enum { ITEM_SOUND, ITEM_BEEP_CHANNELS, ITEM_BEEP_PRIVATE, ITEM_PICK_CHANNELS, ITEM_COUNT };

public:
  AudioCfgScreen(UITask* task) : _task(task), _sel(0), _top(0) { }
  void reset() { _sel = _top = 0; }

  int render(DisplayDriver& display) override {
    UIAudioPrefs* p = _task->getAudioPrefs();
    drawListHeader(display, "Audio", NULL);
    char tmp[32];
    int y = LIST_ROW_Y0;
    for (int i = _top; i < ITEM_COUNT && i < _top + LIST_VISIBLE; i++, y += LIST_ROW_HEIGHT) {
      switch (i) {
        case ITEM_SOUND:         sprintf(tmp, "Suono: %s", _task->isBuzzerQuiet() ? "OFF" : "ON"); break;
        case ITEM_BEEP_CHANNELS: sprintf(tmp, "Beep canali: %s", p->beep_channels ? "ON" : "OFF"); break;
        case ITEM_BEEP_PRIVATE:  sprintf(tmp, "Beep privati: %s", p->beep_private ? "ON" : "OFF"); break;
        case ITEM_PICK_CHANNELS: {
          int total = 0, on = 0;
          ChannelDetails ch;
          for (int k = 0; k < MAX_GROUP_CHANNELS; k++) {
            if (!the_mesh.getChannel(k, ch) || ch.name[0] == 0) continue;
            total++;
            if (_task->isChannelBeepOn(k)) on++;
          }
          sprintf(tmp, "Scegli canali (%d/%d)", on, total);
          break;
        }
      }
      drawListRow(display, y, i == _sel, 2, tmp);
    }
    return 5000;
  }

  bool handleInput(char c) override {
    switch (listInput(c, _sel, _top, ITEM_COUNT)) {
      case LIST_MOVED: return true;
      case LIST_BACK:
        _task->gotoDeviceConfigScreen(false);
        return true;
      case LIST_SELECT: {
        UIAudioPrefs* p = _task->getAudioPrefs();
        if (_sel == ITEM_SOUND) {
          _task->toggleBuzzer();
        } else if (_sel == ITEM_BEEP_CHANNELS) {
          p->beep_channels = !p->beep_channels;
          _task->saveAudioPrefs();
        } else if (_sel == ITEM_BEEP_PRIVATE) {
          p->beep_private = !p->beep_private;
          _task->saveAudioPrefs();
        } else if (_sel == ITEM_PICK_CHANNELS) {
          _task->gotoChannelBeepScreen();
        }
        return true;
      }
      default:
        return false;
    }
  }
};

/* ---------------------------------------------------------------------------------
   Per-channel beep: checked = beeps
   --------------------------------------------------------------------------------- */

class ChannelBeepScreen : public UIScreen {
  UITask* _task;
  uint8_t _idx[MAX_GROUP_CHANNELS];
  int _count, _sel, _top;

public:
  ChannelBeepScreen(UITask* task) : _task(task), _count(0), _sel(0), _top(0) { }

  void open() {
    _count = _sel = _top = 0;
    ChannelDetails ch;
    for (int i = 0; i < MAX_GROUP_CHANNELS; i++) {
      if (!the_mesh.getChannel(i, ch) || ch.name[0] == 0) continue;
      _idx[_count++] = i;
    }
  }

  int render(DisplayDriver& display) override {
    int on = 0;
    for (int i = 0; i < _count; i++) if (_task->isChannelBeepOn(_idx[i])) on++;
    char right[12];
    sprintf(right, "%d/%d", on, _count);
    drawListHeader(display, "Beep canali", right);

    if (_count == 0) {
      display.setColor(UIColor::secondary_txt);
      display.drawTextCentered(display.width() / 2, 30, "Nessun canale");
      return 5000;
    }

    int y = LIST_ROW_Y0;
    ChannelDetails ch;
    for (int i = _top; i < _count && i < _top + LIST_VISIBLE; i++, y += LIST_ROW_HEIGHT) {
      if (!the_mesh.getChannel(_idx[i], ch)) continue;
      drawListRow(display, y, i == _sel, 12, ch.name);
      display.drawRect(2, y, 7, 7);   // checkbox, in the row's current colour
      if (_task->isChannelBeepOn(_idx[i])) {
        display.fillRect(4, y + 2, 3, 3);
      }
    }
    return 5000;
  }

  bool handleInput(char c) override {
    switch (listInput(c, _sel, _top, _count)) {
      case LIST_MOVED: return true;
      case LIST_BACK:
        _task->gotoAudioConfigScreen(false);
        return true;
      case LIST_SELECT:
        if (_count > 0) _task->toggleChannelBeep(_idx[_sel]);
        return true;
      default:
        return false;
    }
  }
};

/* ---------------------------------------------------------------------------------
   UITask glue + persistence
   --------------------------------------------------------------------------------- */

void UITask::beginDeviceConfig() {
  device_cfg = new DeviceCfgScreen(this);
  audio_cfg = new AudioCfgScreen(this);
  channel_beep = new ChannelBeepScreen(this);
}

void UITask::gotoDeviceConfigScreen(bool reset) {
  if (reset) ((DeviceCfgScreen *) device_cfg)->reset();
  setCurrScreen(device_cfg);
}

void UITask::gotoAudioConfigScreen(bool reset) {
  if (reset) ((AudioCfgScreen *) audio_cfg)->reset();
  setCurrScreen(audio_cfg);
}

void UITask::gotoChannelBeepScreen() {
  ((ChannelBeepScreen *) channel_beep)->open();
  setCurrScreen(channel_beep);
}

// file format: 1 byte version, then UIAudioPrefs
void UITask::loadAudioPrefs() {
  if (_audio_loaded) return;
  _audio_loaded = true;

  memset(&_audio, 0, sizeof(_audio));
  _audio.beep_channels = 1;
  _audio.beep_private = 1;

  uint8_t buf[1 + sizeof(UIAudioPrefs)];
  int len = the_mesh.getDataStore()->readSmallFile(UI_AUDIO_FILE, buf, sizeof(buf));
  if (len == (int)sizeof(buf) && buf[0] == UI_AUDIO_FILE_VER) {
    memcpy(&_audio, &buf[1], sizeof(_audio));
    if (_audio.n_muted > MAX_GROUP_CHANNELS) _audio.n_muted = MAX_GROUP_CHANNELS;
  }
}

void UITask::saveAudioPrefs() {
  uint8_t buf[1 + sizeof(UIAudioPrefs)];
  buf[0] = UI_AUDIO_FILE_VER;
  memcpy(&buf[1], &_audio, sizeof(_audio));
  the_mesh.getDataStore()->writeSmallFile(UI_AUDIO_FILE, buf, sizeof(buf));
}

UIAudioPrefs* UITask::getAudioPrefs() {
  loadAudioPrefs();
  return &_audio;
}

bool UITask::isChannelMuted(const uint8_t* secret) {
  loadAudioPrefs();
  for (int i = 0; i < _audio.n_muted; i++) {
    if (memcmp(_audio.muted[i], secret, UI_CHANNEL_KEY_LEN) == 0) return true;
  }
  return false;
}

bool UITask::isChannelBeepOn(int channel_idx) {
  ChannelDetails ch;
  if (!the_mesh.getChannel(channel_idx, ch)) return true;
  return !isChannelMuted(ch.channel.secret);
}

void UITask::toggleChannelBeep(int channel_idx) {
  ChannelDetails ch;
  if (!the_mesh.getChannel(channel_idx, ch)) return;
  loadAudioPrefs();
  for (int i = 0; i < _audio.n_muted; i++) {
    if (memcmp(_audio.muted[i], ch.channel.secret, UI_CHANNEL_KEY_LEN) == 0) {   // un-mute
      _audio.n_muted--;
      if (i < _audio.n_muted) memmove(_audio.muted[i], _audio.muted[i + 1], (_audio.n_muted - i) * UI_CHANNEL_KEY_LEN);
      saveAudioPrefs();
      return;
    }
  }
  if (_audio.n_muted < MAX_GROUP_CHANNELS) {   // mute
    memcpy(_audio.muted[_audio.n_muted++], ch.channel.secret, UI_CHANNEL_KEY_LEN);
    saveAudioPrefs();
  }
}

void UITask::notifyChannelMsg(uint8_t channel_idx) {
  if (!isChannelBeepOn(channel_idx)) return;
  notify(UIEventType::channelMessage);
}

#endif  // UI_DEVICE_CONFIG

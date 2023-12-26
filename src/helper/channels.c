#include "channels.h"
#include "../driver/eeprom.h"
#include "../external/printf/printf.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include <string.h>

static uint16_t presetsSizeBytes() {
  return gSettings.presetsCount * PRESET_SIZE + BANDS_OFFSET;
}

uint16_t CHANNELS_GetCountMax() {
  return (EEPROM_SIZE - presetsSizeBytes()) / VFO_SIZE - 2; // 2 VFO
}

void CHANNELS_Load(uint16_t num, VFO *p) {
  EEPROM_ReadBuffer(CHANNELS_OFFSET - (num + 1) * VFO_SIZE, p, VFO_SIZE);
}

void CHANNELS_Save(uint16_t num, VFO *p) {
  EEPROM_WriteBuffer(CHANNELS_OFFSET - (num + 1) * VFO_SIZE, p, VFO_SIZE);
}

void CHANNELS_LoadUser(uint16_t num, VFO *p) { CHANNELS_Load(num + 2, p); }

void CHANNELS_SaveUser(uint16_t num, VFO *p) { CHANNELS_Save(num + 2, p); }

void CHANNELS_SaveCurrentVFO(uint16_t i) {
  if (!IsReadable(gCurrentVFO->name)) {
    sprintf(gCurrentVFO->name, "%u.%05u", gCurrentVFO->fRX / 100000,
            gCurrentVFO->fRX % 100000);
  }
  CHANNELS_SaveUser(i, gCurrentVFO);
}

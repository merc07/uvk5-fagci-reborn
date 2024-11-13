#include "chcfg.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

CH gChEd;

static MenuItem menu[] = {
    {"Name", M_NAME, 0},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, 4},
    {"Gain", M_GAIN, ARRAY_SIZE(gainTable)},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(sqTypeNames)},
    {"SQ level", M_SQ, 10},
    {"RX freq", M_F_RX, 0},
    {"TX freq", M_F_TX, 0},
    {"RX code type", M_RX_CODE_TYPE, ARRAY_SIZE(TX_CODE_TYPES)},
    {"RX code", M_RX_CODE, 0},
    {"TX code type", M_TX_CODE_TYPE, ARRAY_SIZE(TX_CODE_TYPES)},
    {"TX code", M_TX_CODE, 0},
    {"TX offset", M_TX_OFFSET, 0},
    {"TX offset dir", M_TX_OFFSET_DIR, ARRAY_SIZE(TX_OFFSET_NAMES)},
    {"TX power", M_F_TXP, ARRAY_SIZE(TX_POWER_NAMES)},
    {"Radio", M_RADIO, ARRAY_SIZE(radioNames)},
    {"Scrambler", M_SCRAMBLER, 16},
    {"Enable TX", M_TX, 2},
    {"Readonly", M_READONLY, 2},
    {"Type", M_TYPE, ARRAY_SIZE(CH_TYPE_NAMES)},

    // TODO: conditional naming
    {"F start", M_START, 0},
    {"F end", M_END, 0},

    {"Save", M_SAVE, 0},
};
static const uint8_t MENU_SIZE = ARRAY_SIZE(menu);

static void setInitialSubmenuIndex(void) {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_RADIO:
    subMenuIndex = gChEd.radio;
    break;
  case M_BW:
    subMenuIndex = gChEd.bw;
    break;
  case M_RX_CODE_TYPE:
    subMenuIndex = gChEd.code.rx.type;
    break;
  case M_RX_CODE:
    subMenuIndex = gChEd.code.rx.value;
    break;
  case M_TX_CODE_TYPE:
    subMenuIndex = gChEd.code.tx.type;
    break;
  case M_TX_CODE:
    subMenuIndex = gChEd.code.tx.value;
    break;
  case M_F_TXP:
    subMenuIndex = gChEd.power;
    break;
  case M_TX_OFFSET_DIR:
    subMenuIndex = gChEd.offsetDir;
    break;
  case M_MODULATION:
    subMenuIndex = gChEd.modulation;
    break;
  case M_STEP:
    subMenuIndex = gChEd.step;
    break;
  case M_SQ_TYPE:
    subMenuIndex = gChEd.squelch.type;
    break;
  case M_SQ:
    subMenuIndex = gChEd.squelch.value;
    break;
  case M_GAIN:
    subMenuIndex = gChEd.gainIndex;
    break;
  case M_SCRAMBLER:
    subMenuIndex = gChEd.scrambler;
    break;
  case M_TX:
    subMenuIndex = gChEd.allowTx;
    break;
  case M_READONLY:
    subMenuIndex = gChEd.readonly;
    break;
  case M_TYPE:
    subMenuIndex = gChEd.type;
    break;
  default:
    subMenuIndex = 0;
    break;
  }
}

static void getMenuItemText(uint16_t index, char *name) {
  MenuItem *m = &menu[index];
  if (gChEd.type == TYPE_PRESET && m->type == M_F_RX) {
    strncpy(name, "Start f", 31);
    return;
  }
  if (gChEd.type == TYPE_PRESET && m->type == M_F_TX) {
    strncpy(name, "End f", 31);
    return;
  }
  strncpy(name, menu[index].name, 31);
}

static void updateTxCodeListSize() {
  for (uint8_t i = 0; i < ARRAY_SIZE(menu); ++i) {
    MenuItem *item = &menu[i];
    uint8_t type = CODE_TYPE_OFF;
    if (item->type == M_TX_CODE) {
      type = gChEd.code.tx.type;
    } else if (item->type == M_RX_CODE) {
      type = gChEd.code.rx.type;
    }
    if (type == CODE_TYPE_CONTINUOUS_TONE) {
      item->size = ARRAY_SIZE(CTCSS_Options);
    } else if (type != CODE_TYPE_OFF) {
      item->size = ARRAY_SIZE(DCS_Options);
    } else {
      item->size = 0;
    }
  }
}

static void getSubmenuItemText(uint16_t index, char *name) {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_RADIO:
    strncpy(name, radioNames[index], 31);
    return;
  case M_MODULATION:
    strncpy(name, modulationTypeOptions[index], 31);
    return;
  case M_BW:
    strncpy(name, RADIO_GetBWName(index), 15);
    return;
  case M_RX_CODE_TYPE:
    strncpy(name, TX_CODE_TYPES[index], 15);
    return;
  case M_RX_CODE:
    PrintRTXCode(name, radio->code.rx.type, index);
    return;
  case M_TX_CODE_TYPE:
    strncpy(name, TX_CODE_TYPES[index], 15);
    return;
  case M_TX_CODE:
    PrintRTXCode(name, radio->code.tx.type, index);
    return;
  case M_F_TXP:
    strncpy(name, TX_POWER_NAMES[index], 15);
    return;
  case M_TX_OFFSET_DIR:
    strncpy(name, TX_OFFSET_NAMES[index], 15);
    return;
  case M_STEP:
    sprintf(name, "%u.%02uKHz", StepFrequencyTable[index] / 100,
            StepFrequencyTable[index] % 100);
    return;
  case M_SQ_TYPE:
    strncpy(name, sqTypeNames[index], 31);
    return;
  case M_SQ:
  case M_SCRAMBLER:
    sprintf(name, "%u", index);
    return;
  case M_GAIN:
    sprintf(name, index == AUTO_GAIN_INDEX ? "auto" : "%+ddB",
            -gainTable[index].gainDb + 33);
    return;
  case M_TX:
  case M_READONLY:
    strncpy(name, yesNo[index], 31);
    return;
  case M_TYPE:
    strncpy(name, CH_TYPE_NAMES[index], 31);
    return;
  default:
    break;
  }
}

static void setTXF(uint32_t f) {
  radio->txF = f;
  RADIO_SaveCurrentVFO();
}

static void setTXOffset(uint32_t f) {
  gCurrentPreset.txF = f;
  PRESETS_SaveCurrent();
}

void CHCFG_init(void) { updateTxCodeListSize(); }

void CHCFG_update(void) {}

static bool accept(void) {
  const MenuItem *item = &menu[menuIndex];
  // RUN APPS HERE
  switch (item->type) {
  case M_F_RX:
    gFInputCallback = RADIO_TuneTo;
    gFInputTempFreq = radio->rxF;
    APPS_run(APP_FINPUT);
    return true;
  case M_F_TX:
    gFInputCallback = setTXF;
    gFInputTempFreq = radio->txF;
    APPS_run(APP_FINPUT);
    return true;
  case M_TX_OFFSET:
    gFInputCallback = setTXOffset;
    gFInputTempFreq = gCurrentPreset.txF;
    APPS_run(APP_FINPUT);
    return true;
  case M_SAVE:
    APPS_run(APP_SAVECH);
    return true;
  default:
    break;
  }
  updateTxCodeListSize();
  if (isSubMenu) {
    AcceptRadioConfig(item, subMenuIndex);
    isSubMenu = false;
  } else {
    isSubMenu = true;
    setInitialSubmenuIndex();
  }
  return true;
}

static void setMenuIndexAndRun(uint16_t v) {
  if (isSubMenu) {
    subMenuIndex = v - 1;
  } else {
    menuIndex = v - 1;
  }
  accept();
}

static void upDown(uint8_t inc) {
  if (isSubMenu) {
    IncDec8(&subMenuIndex, 0, menu[menuIndex].size, inc);
    OnRadioSubmenuChange(&menu[menuIndex], subMenuIndex);
  } else {
    IncDec8(&menuIndex, 0, MENU_SIZE, inc);
  }
}

bool CHCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(menuIndex + 1, 1, MENU_SIZE);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      uint8_t v = NUMNAV_Input(key) - 1;
      if (isSubMenu) {
        subMenuIndex = v;
      } else {
        menuIndex = v;
      }
      return true;
    }
  }
  switch (key) {
  case KEY_UP:
    upDown(-1);
    return true;
  case KEY_DOWN:
    upDown(1);
    return true;
  case KEY_MENU:
    return accept();
  case KEY_EXIT:
    if (isSubMenu) {
      isSubMenu = false;
    } else {
      APPS_exit();
    }
    return true;
  default:
    break;
  }
  return false;
}

void CHCFG_render(void) {
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else {
    STATUSLINE_SetText(apps[APP_VFO_CFG].name);
  }
  MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    UI_ShowMenu(getSubmenuItemText, item->size, subMenuIndex);
    STATUSLINE_SetText(item->name);
  } else {
    UI_ShowMenu(getMenuItemText, ARRAY_SIZE(menu), menuIndex);
    char Output[32] = "";
    GetMenuItemValue(item->type, Output);
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT - 4, POS_C, C_FILL, Output);
  }
}
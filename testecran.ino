// ===== FORCER LE USB SPOOFING (ne pas modifier sauf besoin spécifique) =====
#undef USB_VID
#undef USB_PID
#undef USB_MANUFACTURER
#undef USB_PRODUCT
#define USB_VID 0x046D
#define USB_PID 0xC07D
#define USB_MANUFACTURER u"Logitech"
#define USB_PRODUCT u"G502 HERO Gaming Mouse"
// ==========================================================================

#include <hidboot.h>
#include <usbhub.h>
#include <Mouse.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <U8g2lib.h>

// --------- CONFIG UTILISATEUR (fichier à côté du sketch) ------------------
#include "user_config.h"

// ===================== ÉCRAN OLED SH1107 128x128 I2C ======================
U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2(U8G2_R1);

// ====================== CONSTANTES ARMES ==================================
enum { IDX_R99=0, IDX_R301, IDX_FLAT, IDX_VOLT, IDX_SPITFIRE, WEAPON_COUNT };

// Valeurs runtime depuis la config
uint8_t ACTIVATION_MODE              = CFG_ACTIVATION_MODE;
uint8_t ENABLE_R99                   = CFG_ENABLE_R99;
uint8_t ENABLE_R301                  = CFG_ENABLE_R301;
uint8_t ENABLE_FLATLINE              = CFG_ENABLE_FLATLINE;
uint8_t ENABLE_VOLT                 = CFG_ENABLE_VOLT;
uint8_t ENABLE_SPITFIRE             = CFG_ENABLE_SPITFIRE;
float   REFERENCE_GAME_SENSITIVITY   = CFG_REFERENCE_GAME_SENSITIVITY;
float   USER_GAME_SENSITIVITY        = CFG_USER_GAME_SENSITIVITY;
float   USER_ADDITIONAL_COMPENSATION = CFG_USER_ADDITIONAL_COMPENSATION;
float   MULTIPLIER_X                 = CFG_MULTIPLIER_X;
float   MULTIPLIER_Y                 = CFG_MULTIPLIER_Y;
uint8_t ENABLE_WHEEL_CYCLE           = CFG_ENABLE_WHEEL_CYCLE;
uint8_t ENABLE_XB1_CYCLE             = CFG_ENABLE_XB1_CYCLE;
uint8_t ENABLE_MMB_MODE_TOGGLE       = CFG_ENABLE_MMB_MODE_TOGGLE;
uint8_t ENABLE_XB1_MODE_TOGGLE       = CFG_ENABLE_XB1_MODE_TOGGLE;
uint16_t MODE_TOGGLE_THRESHOLD_MS    = CFG_MODE_TOGGLE_THRESHOLD_MS;

// Ordre des armes (rempli par parseWeaponOrder)
uint8_t WEAPON_ORDER[WEAPON_COUNT];
uint8_t WEAPON_ORDER_SIZE = 0;

// ======================= PATTERNS (PROGMEM) ===============================
const char NAME_R99[] PROGMEM = "R99";
const uint8_t SIZE_R99 = 30;
const uint16_t TP_R99[SIZE_R99] PROGMEM = {
  0,55,111,166,222,277,333,388,444,500,555,611,666,722,777,833,888,944,1000,1055,1111,1166,1222,1277,1333,1388,1444,1500,1555,1611
};
const float X_R99[SIZE_R99] PROGMEM = {
  0,6.9,9.1,2.1,0.5,12.5,26.2,39.8,48.3,51.9,38.9,24.9,22,17.6,23.4,34.5,47.8,64.5,47.4,30.3,10.3,0.8,-11.5,0.4,15.4,35.6,44.4,32.8,16.8,26.6
};
const float Y_R99[SIZE_R99] PROGMEM = {
  0,-31.1,-57,-82.8,-124.2,-178.8,-223.1,-255.6,-297.3,-329.1,-365.6,-400.9,-428.9,-467.5,-477.7,-483.6,-496.4,-495.2,-517.8,-518.9,-526.8,-532.9,-537.6,-546.1,-540.5,-539.6,-545.7,-554.1,-561.8,-577.7
};

const char NAME_R301[] PROGMEM = "R301";
const uint8_t SIZE_R301 = 31;
const uint16_t TP_R301[SIZE_R301] PROGMEM = {
  0,74,148,222,296,370,444,518,592,666,740,814,888,962,1037,1111,1185,1259,1333,1407,1481,1555,1629,1703,1777,1851,1925,2000,2074,2148,2222
};
const float X_R301[SIZE_R301] PROGMEM = {
  0,12.5,11.4,25.7,24.2,26.7,26.2,30,36.9,46.7,59.5,65.3,55.8,43.5,30.7,17,5,-4.6,-7.6,-1.2,9.2,21.2,31.1,43,51.6,59.6,65.9,67.6,67.8,71.4,75.6
};
const float Y_R301[SIZE_R301] PROGMEM = {
  0,-26.3,-61.7,-86,-113.6,-136.2,-150.7,-161.9,-167.5,-175.6,-172.3,-182.4,-199.3,-213.7,-224.6,-227.4,-226.5,-226.7,-237.6,-246.4,-256.2,-259.1,-266.2,-262.7,-261.4,-260.3,-266.5,-272.6,-280.8,-288.8,-294.5
};

const char NAME_FLAT[] PROGMEM = "FLATLINE";
const uint8_t SIZE_FLAT = 29;
const uint16_t TP_FLAT[SIZE_FLAT] PROGMEM = {
  0,100,200,300,400,500,600,700,800,900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800
};
const float X_FLAT[SIZE_FLAT] PROGMEM = {
  0,-14.1,-18.1,-42.7,-63,-65.7,-63.4,-50.8,-28.5,-20.4,-13.6,-11.8,-23.8,-46.9,-68.1,-89.5,-93.5,-101.7,-123,-144.7,-155.5,-160.2,-155.2,-140,-122.7,-107.5,-92.4,-77.9,-75.1
};
const float Y_FLAT[SIZE_FLAT] PROGMEM = {
  0,-47.8,-68,-101.6,-122.5,-150.6,-178.3,-185.3,-175.5,-171,-182.2,-202.1,-208.4,-226.3,-226.5,-227.6,-244.7,-266.2,-280,-294.9,-296.7,-303.4,-317.9,-323.3,-333.1,-337,-341.3,-338.5,-355.9
};

const char NAME_VOLT[] PROGMEM = "VOLT";
const uint8_t SIZE_VOLT = 26;
const uint16_t TP_VOLT[SIZE_VOLT] PROGMEM = {
  0,83,167,250,333,417,500,583,667,750,833,917,1000,1083,1167,1250,1333,1417,1500,1583,1667,1750,1833,1917,2000,2083
};
const float X_VOLT[SIZE_VOLT] PROGMEM = {
  0,11.2,13.8,10.4,21.2,20.5,39.3,44.7,35,46.2,44.6,28.5,8,-5.1,-17.4,-12.8,0.5,10,5.9,-3.1,-12.9,-21.1,-23.1,-23.1,-24.9,-26.3
};
const float Y_VOLT[SIZE_VOLT] PROGMEM = {
  0,-28.6,-64,-106.9,-128.8,-177.8,-205.6,-248.3,-279.3,-312.2,-333.8,-339.7,-336.4,-350,-365.8,-376.8,-377.4,-379,-388.2,-395.3,-399.4,-398.7,-401,-407.5,-414,-427
};

const char NAME_SPITFIRE[] PROGMEM = "SPITFIRE";
const uint8_t SIZE_SPITFIRE = 55;
const uint16_t TP_SPITFIRE[SIZE_SPITFIRE] PROGMEM = {
  0,111,222,333,444,555,666,777,888,1000,1111,1222,1333,1444,1555,1666,1777,1888,2000,2111,2222,2333,2444,2555,2666,2777,2888,3000,3111,3222,3333,3444,3555,3666,3777,3888,4000,4111,4222,4333,4444,4555,4666,4777,4888,5000,5111,5222,5333,5444,5555,5666,5777,5888,6000
};
const float X_SPITFIRE[SIZE_SPITFIRE] PROGMEM = {
  0,-2.1,-4.8,-17.1,-24.8,-30.4,-30.4,-24.5,-13.4,-10.5,-5.8,-3.1,-10.2,-24.1,-33.6,-46.6,-53,-58.1,-63,-73.1,-85,-90.8,-88.2,-77.3,-70.1,-59.7,-48.7,-36.8,-32.9,-27,-26.4,-13.4,-8.7,-3.4,0.6,-6.1,-19.4,-25,-38.4,-45.7,-47.4,-51.3,-61,-73.1,-78.5,-75.8,-67.2,-61.4,-50.3,-39.7,-27.7,-23.6,-17.1,-17.6,-3.3
};
const float Y_SPITFIRE[SIZE_SPITFIRE] PROGMEM = {
  0,-35.4,-47.7,-69.2,-84.4,-105.3,-128.2,-135.1,-131.3,-128,-134.7,-150.5,-156.2,-166.2,-170.3,-166.2,-169.8,-188.1,-204.6,-213.7,-217,-218.2,-230.6,-236.7,-241.6,-247.7,-252.3,-245.7,-256,-269.6,-278,-274.4,-269.6,-276.2,-290.2,-293.2,-303.7,-305.8,-300.6,-302.7,-321.3,-340,-351.8,-355.8,-357.6,-371.9,-381.2,-388.3,-397.7,-403.3,-398.6,-410.3,-424.8,-437.8,-435.1
};

// ======================= CATALOGUE & ETAT ================================
struct Weapon {
  const char* name_P;
  uint8_t size;
  const uint16_t* tp_P;
  const float*    x_P;
  const float*    y_P;
  uint8_t enabled;
};

const Weapon weapons[WEAPON_COUNT] = {
  { NAME_R99,      SIZE_R99,      TP_R99,      X_R99,      Y_R99,      0 },
  { NAME_R301,     SIZE_R301,     TP_R301,     X_R301,     Y_R301,     0 },
  { NAME_FLAT,     SIZE_FLAT,     TP_FLAT,     X_FLAT,     Y_FLAT,     0 },
  { NAME_VOLT,     SIZE_VOLT,     TP_VOLT,     X_VOLT,     Y_VOLT,     0 },
  { NAME_SPITFIRE, SIZE_SPITFIRE, TP_SPITFIRE, X_SPITFIRE, Y_SPITFIRE, 0 }
};

int8_t  currentWeaponIndex   = -1;
int8_t  currentOrderPosition = -1;
bool    scriptEnabled        = false;
bool    leftPressed          = false;
bool    rightPressed         = false;

unsigned long patternStartTime    = 0;
uint8_t       currentPatternIndex = 0;
float         lastPatternX        = 0;
float         lastPatternY        = 0;
bool          patternActive       = false;

unsigned long mmbDownAt = 0; bool mmbArmed = false;
unsigned long xb1DownAt = 0; bool xb1Armed = false;

bool displayNeedsUpdate = true;

// ======================= UTILS ===========================================
#ifndef MOUSE_XB1
#define MOUSE_XB1 0x04
#endif
#ifndef MOUSE_XB2
#define MOUSE_XB2 0x05
#endif

static float readFloat_P(const float* addr){
  float v;
  memcpy_P(&v, addr, sizeof(v));
  return v;
}

static int8_t floatToInt8(float v){
  if(v > 127.0f)       v = 127.0f;
  else if(v < -128.0f) v = -128.0f;
  v = (v >= 0.0f) ? (v + 0.5f) : (v - 0.5f);
  return (int8_t)v;
}

// =============== Parse ordre "R99, R301, FLATLINE" -> indices =============
static bool weaponNameToIndex(const char* name, uint8_t &idx){
  if     (strcasecmp(name,"R99")      == 0){ idx=IDX_R99;       return true; }
  else if(strcasecmp(name,"R301")     == 0){ idx=IDX_R301;      return true; }
  else if(strcasecmp(name,"FLATLINE") == 0){ idx=IDX_FLAT;      return true; }
  else if(strcasecmp(name,"VOLT")     == 0){ idx=IDX_VOLT;      return true; }
  else if(strcasecmp(name,"SPITFIRE") == 0){ idx=IDX_SPITFIRE;  return true; }
  return false;
}

static void parseWeaponOrder(const char* list){
  char buf[96];
  strncpy(buf, list, sizeof(buf)-1);
  buf[sizeof(buf)-1]=0;

  char* p = buf;
  WEAPON_ORDER_SIZE = 0;

  while(p && WEAPON_ORDER_SIZE < WEAPON_COUNT){
    while(*p==' ' || *p=='\t') ++p;
    char* comma = strchr(p, ',');
    if(comma) *comma = 0;
    int n = strlen(p);
    while(n>0 && (p[n-1]==' ' || p[n-1]=='\t')) p[--n]=0;
    uint8_t idx;
    if(n>0 && weaponNameToIndex(p, idx)){
      WEAPON_ORDER[WEAPON_ORDER_SIZE++] = idx;
    }
    if(!comma) break;
    p = comma + 1;
  }
  if(WEAPON_ORDER_SIZE == 0){
    WEAPON_ORDER[0] = IDX_R99;
    WEAPON_ORDER_SIZE = 1;
  }
}

// ====================== PARSEUR SOURIS ===================================
class MouseRptParser : public MouseReportParser {
protected:
  void OnMouseMove(MOUSEINFO *mi){
    Mouse.move(mi->dX, mi->dY, 0);
  }

  void OnWheel(int8_t wheel){
    if(wheel) Mouse.move(0, 0, wheel);
  }

  void OnMouseScroll(MOUSEINFO *mi){
    int8_t z = (int8_t)mi->dZ;
    if(z) Mouse.move(0,0,z);
  }

  void OnLeftButtonDown(MOUSEINFO *mi){
    leftPressed = true;
    Mouse.press(MOUSE_LEFT);

    bool ok = (ACTIVATION_MODE==0)
              ? (scriptEnabled && currentWeaponIndex>=0)
              : (scriptEnabled && rightPressed && currentWeaponIndex>=0);

    if(ok){
      patternActive       = true;
      patternStartTime    = millis();
      currentPatternIndex = 0;
      lastPatternX        = 0;
      lastPatternY        = 0;
    }
  }

  void OnLeftButtonUp(MOUSEINFO *mi){
    leftPressed = false;
    Mouse.release(MOUSE_LEFT);
    if(patternActive){
      patternActive = false;
    }
  }

  void OnRightButtonDown(MOUSEINFO *mi){
    rightPressed = true;
    Mouse.press(MOUSE_RIGHT);

    if(ACTIVATION_MODE==1 && scriptEnabled && leftPressed && currentWeaponIndex>=0){
      patternActive       = true;
      patternStartTime    = millis();
      currentPatternIndex = 0;
      lastPatternX        = 0;
      lastPatternY        = 0;
    }
  }

  void OnRightButtonUp(MOUSEINFO *mi){
    rightPressed = false;
    Mouse.release(MOUSE_RIGHT);
    if(patternActive && ACTIVATION_MODE==1){
      patternActive = false;
    }
  }

  void OnMiddleButtonDown(MOUSEINFO *mi){
    mmbDownAt = millis();
    mmbArmed  = true;

    if(!ENABLE_WHEEL_CYCLE && !ENABLE_MMB_MODE_TOGGLE){
      Mouse.press(MOUSE_MIDDLE);
    }
  }

  void OnMiddleButtonUp(MOUSEINFO *mi){
    unsigned long held = millis() - mmbDownAt;

    if(!mmbArmed){
      if(!ENABLE_WHEEL_CYCLE && !ENABLE_MMB_MODE_TOGGLE) Mouse.release(MOUSE_MIDDLE);
      return;
    }

    if(ENABLE_MMB_MODE_TOGGLE && held >= MODE_TOGGLE_THRESHOLD_MS){
      ACTIVATION_MODE = (ACTIVATION_MODE == 0) ? 1 : 0;
      displayNeedsUpdate = true;
    } else {
      if(ENABLE_WHEEL_CYCLE){
        cycleToNextWeapon();
      } else {
        Mouse.press(MOUSE_MIDDLE);
        Mouse.release(MOUSE_MIDDLE);
      }
    }
    mmbArmed = false;
  }

  void OnXB1ButtonDown(MOUSEINFO *mi){
    xb1DownAt = millis();
    xb1Armed  = true;

    if(!ENABLE_XB1_CYCLE && !ENABLE_XB1_MODE_TOGGLE){
      Mouse.press(MOUSE_XB1);
    }
  }

  void OnXB1ButtonUp(MOUSEINFO *mi){
    unsigned long held = millis() - xb1DownAt;
    const uint16_t shortPress = 600;

    if(!xb1Armed){
      if(!ENABLE_XB1_CYCLE && !ENABLE_XB1_MODE_TOGGLE) Mouse.release(MOUSE_XB1);
      return;
    }

    if(held >= MODE_TOGGLE_THRESHOLD_MS){
      if(ENABLE_XB1_MODE_TOGGLE){
        ACTIVATION_MODE = (ACTIVATION_MODE == 0) ? 1 : 0;
        displayNeedsUpdate = true;
      } else {
        Mouse.press(MOUSE_XB1);
        Mouse.release(MOUSE_XB1);
      }
    }
    else if(held < shortPress){
      if(ENABLE_XB1_CYCLE){
        cycleToNextWeapon();
      } else {
        Mouse.press(MOUSE_XB1);
        Mouse.release(MOUSE_XB1);
      }
    }
    else {
      Mouse.press(MOUSE_XB1);
      Mouse.release(MOUSE_XB1);
    }

    xb1Armed = false;
  }

  void OnXB2ButtonDown(MOUSEINFO *mi){
    Mouse.press(MOUSE_XB2);
  }

  void OnXB2ButtonUp(MOUSEINFO *mi){
    Mouse.release(MOUSE_XB2);
  }
};

// ====================== USB Host init =====================================
USB Usb;
USBHub Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE> HidMouse(&Usb);
MouseRptParser MousePrs;

// ====================== AFFICHAGE OLED ===================================
// Police : u8g2_font_t0_22_tr
static void updateDisplay(){
  u8g2.firstPage();
  do {
    // Très important : garder le host USB en vie pendant le dessin
    Usb.Task();

    u8g2.setFont(u8g2_font_t0_22_tr);

    // -------- Ligne 1 : ARME ACTUELLE OU OFF (centré) --------
    char line1[16];
    if (scriptEnabled && currentWeaponIndex >= 0) {
      strcpy_P(line1, weapons[currentWeaponIndex].name_P);
    } else {
      strcpy(line1, "OFF");
    }
    uint8_t w1 = u8g2.getStrWidth(line1);
    uint8_t x1 = (128 - w1) / 2;
    u8g2.drawStr(x1, 48, line1);

    // -------- Ligne 2 : PROCHAINE ARME (centrée, toujours) --------
    char line2[24];
    char nextName[12];
    bool hasNext = false;

    if (WEAPON_ORDER_SIZE > 0) {
      if (currentOrderPosition < 0) {
        // OFF -> prochaine = première arme active
        for (uint8_t i = 0; i < WEAPON_ORDER_SIZE; i++) {
          uint8_t idx = WEAPON_ORDER[i];
          if (((Weapon&)weapons[idx]).enabled) {
            strcpy_P(nextName, weapons[idx].name_P);
            hasNext = true;
            break;
          }
        }
      } else {
        // Sinon -> prochaine arme active après la position actuelle
        for (uint8_t pos = currentOrderPosition + 1; pos < WEAPON_ORDER_SIZE; pos++) {
          uint8_t idx = WEAPON_ORDER[pos];
          if (((Weapon&)weapons[idx]).enabled) {
            strcpy_P(nextName, weapons[idx].name_P);
            hasNext = true;
            break;
          }
        }
      }
    }

    if (hasNext) {
      strcpy(line2, "Next: ");
      strcat(line2, nextName);
    } else {
      strcpy(line2, "Next: OFF");
    }

    uint8_t w2 = u8g2.getStrWidth(line2);
    uint8_t x2 = (128 - w2) / 2;
    u8g2.drawStr(x2, 80, line2);

  } while (u8g2.nextPage());
}

static void cycleToNextWeapon(){
  patternActive = false;

  for(uint8_t attempts = 0; attempts <= WEAPON_ORDER_SIZE; attempts++){
    currentOrderPosition++;
    if(currentOrderPosition >= WEAPON_ORDER_SIZE){
      // OFF
      currentOrderPosition = -1;
      currentWeaponIndex   = -1;
      scriptEnabled        = false;
      displayNeedsUpdate   = true;
      return;
    }

    uint8_t weaponIdx = WEAPON_ORDER[currentOrderPosition];
    if(((Weapon&)weapons[weaponIdx]).enabled == 1){
      currentWeaponIndex = weaponIdx;
      scriptEnabled      = true;
      displayNeedsUpdate = true;
      return;
    }
  }

  currentOrderPosition = -1;
  currentWeaponIndex   = -1;
  scriptEnabled        = false;
  displayNeedsUpdate   = true;
}

// ====================== SETUP / LOOP ======================================
void setup(){
  // Init I2C + écran OLED
  u8g2.begin();
  Wire.setClock(400000L);       // I2C à 400 kHz pour accélérer l'écran
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_t0_22_tr);
  const char* startMsg = "START";
  uint8_t w = u8g2.getStrWidth(startMsg);
  uint8_t x = (128 - w)/2;
  u8g2.drawStr(x, 64, startMsg);
  u8g2.sendBuffer();

  parseWeaponOrder(CFG_WEAPON_ORDER_STRING);

  ((Weapon&)weapons[IDX_R99]).enabled       = ENABLE_R99;
  ((Weapon&)weapons[IDX_R301]).enabled      = ENABLE_R301;
  ((Weapon&)weapons[IDX_FLAT]).enabled      = ENABLE_FLATLINE;
  ((Weapon&)weapons[IDX_VOLT]).enabled       = ENABLE_VOLT;
  ((Weapon&)weapons[IDX_SPITFIRE]).enabled   = ENABLE_SPITFIRE;

  Mouse.begin();
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  if (Usb.Init() == -1) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_t0_22_tr);
    const char* err = "USB ERR";
    uint8_t we = u8g2.getStrWidth(err);
    uint8_t xe = (128 - we)/2;
    u8g2.drawStr(xe, 64, err);
    u8g2.sendBuffer();
    while (1);
  }

  // Tweaks USB Host
  Usb.regWr(15,0x01); delay(120);
  Usb.regWr(15,0x00); delay(80);
  Usb.regWr(10,0x01); delay(40);
  Usb.regWr(17,0x18); delay(120);
  Usb.busprobe();      delay(200);

  HidMouse.SetReportParser(0, &MousePrs);
  HidMouse.SetReportParser(1, &MousePrs);
  HidMouse.SetProtocol(0,1);
  HidMouse.SetProtocol(1,1);

  displayNeedsUpdate = true;
  updateDisplay();
}

void loop(){
  Usb.Task();

  if(displayNeedsUpdate){
    updateDisplay();
    displayNeedsUpdate = false;
  }

  if(scriptEnabled && currentWeaponIndex>=0 && currentWeaponIndex<WEAPON_COUNT
     && patternActive && leftPressed){
    bool run = (ACTIVATION_MODE==0) ? true : rightPressed;
    if(run){
      unsigned long elapsed = millis() - patternStartTime;
      const Weapon& w = weapons[currentWeaponIndex];
      if(currentPatternIndex < w.size){
        uint16_t targetTime = pgm_read_word(&w.tp_P[currentPatternIndex]);
        if(elapsed >= targetTime){
          float x = readFloat_P(&w.x_P[currentPatternIndex]);
          float y = readFloat_P(&w.y_P[currentPatternIndex]);
          float dX = x - lastPatternX;
          float dY = y - lastPatternY;

          float sensiFactor = (USER_GAME_SENSITIVITY>0.0001f)
                              ? (REFERENCE_GAME_SENSITIVITY/USER_GAME_SENSITIVITY)
                              : 1.0f;
          sensiFactor *= USER_ADDITIONAL_COMPENSATION;

          float effX = MULTIPLIER_X * sensiFactor;
          float effY = MULTIPLIER_Y * sensiFactor;

          int8_t moveX = floatToInt8(-dX * effX);
          int8_t moveY = floatToInt8(-dY * effY);

          if(moveX || moveY){
            Mouse.move(moveX, moveY, 0);
          }
          lastPatternX = x;
          lastPatternY = y;
          currentPatternIndex++;
          if(currentPatternIndex>=w.size){
            patternActive = false;
          }
        }
      }
    }
  }
}

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
// Inclusions conditionnelles des patterns d'armes
#include "weapons_r99.h"
#include "weapons_r301.h"
#include "weapons_flatline.h"
#include "weapons_volt.h"
#include "weapons_spitfire.h"


// ======================= CATALOGUE & ETAT ================================
struct Weapon {
  const char* name_P;
  uint8_t size;
  const uint16_t* tp_P;
  const float*    x_P;
  const float*    y_P;
  uint8_t enabled;
};

// Définitions conditionnelles pour les armes désactivées
#ifndef CFG_ENABLE_R99
#define NAME_R99      "R99"
#define SIZE_R99      0
#define TP_R99        nullptr
#define X_R99         nullptr
#define Y_R99         nullptr
#endif

#ifndef CFG_ENABLE_R301
#define NAME_R301     "R301"
#define SIZE_R301     0
#define TP_R301       nullptr
#define X_R301        nullptr
#define Y_R301        nullptr
#endif

#ifndef CFG_ENABLE_FLATLINE
#define NAME_FLAT     "FLATLINE"
#define SIZE_FLAT     0
#define TP_FLAT       nullptr
#define X_FLAT        nullptr
#define Y_FLAT        nullptr
#endif

#ifndef CFG_ENABLE_VOLT
#define NAME_VOLT     "VOLT"
#define SIZE_VOLT     0
#define TP_VOLT       nullptr
#define X_VOLT        nullptr
#define Y_VOLT        nullptr
#endif

#ifndef CFG_ENABLE_SPITFIRE
#define NAME_SPITFIRE "SPITFIRE"
#define SIZE_SPITFIRE 0
#define TP_SPITFIRE   nullptr
#define X_SPITFIRE    nullptr
#define Y_SPITFIRE    nullptr
#endif

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
      strcpy(line2, nextName);
    } else {
      strcpy(line2, "OFF");
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

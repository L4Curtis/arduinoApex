// user_config.h — paramètres utilisateur pour l’anti-recul
// Ce fichier est lu à la compilation (même dossier que le .ino).

// === Mode de déclenchement ===
// 0 = tir seul (clic gauche) / 1 = visée + tir (clic droit + gauche)
#ifndef CFG_ACTIVATION_MODE
#define CFG_ACTIVATION_MODE 1
#endif

// === Armes activées (1=ON, 0=OFF) ===
#ifndef CFG_ENABLE_R99
#define CFG_ENABLE_R99 1
#endif
#ifndef CFG_ENABLE_R301
#define CFG_ENABLE_R301 1
#endif
#ifndef CFG_ENABLE_FLATLINE
#define CFG_ENABLE_FLATLINE 0
#endif
#ifndef CFG_ENABLE_VOLT
#define CFG_ENABLE_VOLT 1
#endif

// === Sensibilité & gains ===
// REFERENCE = calibration des patterns (ne change généralement pas)
// USER      = ta sensi in-game
// Application: (REFERENCE/USER) * EXTRA
#ifndef CFG_REFERENCE_GAME_SENSITIVITY
#define CFG_REFERENCE_GAME_SENSITIVITY 0.89f
#endif
#ifndef CFG_USER_GAME_SENSITIVITY
#define CFG_USER_GAME_SENSITIVITY 0.89f
#endif
#ifndef CFG_USER_ADDITIONAL_COMPENSATION
#define CFG_USER_ADDITIONAL_COMPENSATION 1.00f
#endif

// === Fins réglages (optionnels) ===
#ifndef CFG_MULTIPLIER_X
#define CFG_MULTIPLIER_X 1.00f
#endif
#ifndef CFG_MULTIPLIER_Y
#define CFG_MULTIPLIER_Y 1.00f
#endif

// === Contrôles (cycle d'arme) ===
// Le scroll haut/bas reste toujours en pass-through (ne change pas d’arme).
#ifndef CFG_ENABLE_WHEEL_CYCLE       // clic molette court -> cycle
#define CFG_ENABLE_WHEEL_CYCLE 0
#endif
#ifndef CFG_ENABLE_XB1_CYCLE         // XB1 court -> cycle
#define CFG_ENABLE_XB1_CYCLE 1
#endif

// === Changer le mode de tir par appui long ===
#ifndef CFG_ENABLE_MMB_MODE_TOGGLE   // clic molette long -> 0<->1
#define CFG_ENABLE_MMB_MODE_TOGGLE 0
#endif
#ifndef CFG_ENABLE_XB1_MODE_TOGGLE   // XB1 long -> 0<->1
#define CFG_ENABLE_XB1_MODE_TOGGLE 1
#endif
#ifndef CFG_MODE_TOGGLE_THRESHOLD_MS // seuil long (ms)
#define CFG_MODE_TOGGLE_THRESHOLD_MS 1000
#endif

// === Ordre des armes pour le cycle ===
// ⚠ Utiliser les noms EXACTS: R99, R301, FLATLINE, VOLT
#ifndef CFG_WEAPON_ORDER_STRING
#define CFG_WEAPON_ORDER_STRING "VOLT, FLATLINE, R301, R99"
#endif

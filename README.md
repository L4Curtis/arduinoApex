# Arduino Apex Anti-Recoil

Projet Arduino pour l'anti-recul sur Apex Legends avec écran OLED SH1107 128x128.

## Installation

1. Cloner le projet :
```bash
git clone https://github.com/L4Curtis/arduinoApex.git
```

2. Ouvrir le sketch `testecran.ino` dans l'IDE Arduino

3. Compiler et uploader sur la carte Arduino

## Configuration

Le fichier `user_config.h` contient vos préférences personnelles. 

**Important :** Ce fichier est configuré avec `skip-worktree` pour que vos modifications locales ne soient jamais pushées sur GitHub.

### Personnalisation

Vous pouvez modifier `user_config.h` localement pour :
- Activer/désactiver les armes
- Ajuster la sensibilité
- Changer l'ordre des armes
- Configurer les contrôles

### Réinitialiser la configuration

Si vous voulez revenir à la configuration par défaut :
```bash
git update-index --no-skip-worktree user_config.h
git checkout user_config.h
git update-index --skip-worktree user_config.h
```

## Armes supportées

- R99
- R301  
- FLATLINE
- VOLT
- SPITFIRE

## Contrôles

- **Clic molette court** : Cycle d'armes (si activé)
- **Clic molette long** : Change le mode de tir (si activé)
- **XB1 court** : Cycle d'armes (si activé)  
- **XB1 long** : Change le mode de tir (si activé)

## Modes de tir

- **Mode 0** : Tir seul (clic gauche)
- **Mode 1** : Visée + tir (clic droit + gauche)

## Matériel requis

- Arduino (compatible avec USB Host Shield)
- USB Host Shield
- Écran OLED SH1107 128x128 (I2C)
- Souris (connectée via USB Host)

## License

Projet personnel - Usage à vos risques et périls.

#define PIPBOY_SETTINGS_VERSION 0x0001

typedef struct pipboySettings {
  uint16_t version;
  char pipboyName[16];
};

typedef void (*appFunction)();

typedef struct itemGeometry {
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
};

typedef struct menuItem {
  char name[127];
  uint8_t nameLength;
  itemGeometry g;
  appFunction app;  // function that process selected page
};


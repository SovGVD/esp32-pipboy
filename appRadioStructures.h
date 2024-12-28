typedef struct radioPeer {
  char name[16];
  uint8_t nameLength;
  signed int rssi;
  unsigned long lastTickOnline; // tick when peer registered as Online (had ping)
  bool isOnline;
  bool isBroadcast;
};

typedef struct radioMsg {
  char body[127];
  uint8_t len;
};
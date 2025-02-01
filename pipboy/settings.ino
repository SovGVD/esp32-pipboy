#define EEPROM_OFFSET 0
uint8_t pipboySettingsEepromSize = 0;

void settingsLoad()
{
	pipboySettingsEepromSize = sizeof(settings);
	Serial.printf("Load settings... size %u bytes\n", pipboySettingsEepromSize);

	EEPROM.begin(pipboySettingsEepromSize);
	EEPROM.get(EEPROM_OFFSET, settings);

	settingsCheck();
}

void settingsCheck()
{
  if (settings.version == PIPBOY_SETTINGS_VERSION) {
	  Serial.println("Settings valid.");
	  return;
  }

  Serial.println("Invalid settings, was not setup or new uncompatible version.");
  settingsInitEEPROM();
}

void settingsInitEEPROM()
{
	Serial.println("Re-init EEPROM");
	for (int i = 0; i < pipboySettingsEepromSize; i++) {
		EEPROM.write(i, 0);  // Zeroes
	}

	EEPROM.get(EEPROM_OFFSET, settings);

	settings.version = PIPBOY_SETTINGS_VERSION;
  
  settings.pipboyName[0] = 'P';
  settings.pipboyName[1] = 'I';
  settings.pipboyName[2] = 'P';
  settings.pipboyName[3] = 'B';
  settings.pipboyName[4] = 'O';
  settings.pipboyName[5] = 'Y';
  settings.pipboyName[6] = random(48,58);
  settings.pipboyName[7] = random(48,58);
  settings.pipboyName[8] = random(48,58);
  settings.pipboyName[9] = '\0';

	settingsCommit();
}

void settingsSanitize()
{
  settingsSanitizeClearSpaces();
}

void settingsSanitizeClearSpaces()
{
  uint16_t stringEnd = sizeof(settings.pipboyName) - 1;

  while (stringEnd > 0 && (settings.pipboyName[stringEnd] == ' ' || settings.pipboyName[stringEnd] == '\0')) {
    settings.pipboyName[stringEnd] = '\0';
    stringEnd--;
  }
}

void settingsCommit()
{
  settingsSanitize();

	EEPROM.put(EEPROM_OFFSET, settings);
	if (!EEPROM.commit()) {
		Serial.println("Error commit EEPROM.");
		return;
	}

	EEPROM.get(EEPROM_OFFSET, settings);
  Serial.println("EEPROM commit successfully.");
}

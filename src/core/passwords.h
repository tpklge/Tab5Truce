
#include <Arduino.h>
#include <FS.h>
#include "core/truce/legacy_fs.h"
#include <SD.h>

String encryptString(String &plaintext, const String &password_str);

String decryptString(String &cypertext, const String &password_str);

String readDecryptedFile(FS &fs, String filepath);

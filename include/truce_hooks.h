#pragma once
class String;
#ifdef TRUCE_LVGL
namespace truce {
bool captureDisplay();
bool captureText(const String &text, int x, int y);
bool capturePrint(const String &text);
void pumpOperation();
}
#endif

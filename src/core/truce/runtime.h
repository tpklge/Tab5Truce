#pragma once
#ifdef TRUCE_LVGL
#include <Arduino.h>
#include <vector>
#include <functional>
struct Option;
namespace truce {
bool active();
void requireStorage();
int menu(std::vector<Option> &items, uint8_t type, const char *title, int index, bool interpreter, const char *icon = nullptr);
String keyboard(const String &text, int limit, const String &title, bool password, int mode = 0);
void chooseTheme();
void brightness();
void bootIntro();
bool operationBorder(bool clear = true);
bool operationTitle(const String &title);
void beginOperation(const String &title, bool stopOnly = false);
void endOperation();
bool drawDocument(const String &title, const std::vector<String> &lines, size_t first, size_t count);
void readDocument(const String &title, const std::vector<String> &lines);
void beginDataPanel(const String &title);
void operationControls(const char *previous, const char *next, const char *action, const char *back = "Voltar");
bool operationSubtitle(const String &text);
bool beginSpectrum(const String &title, bool sdr = false, bool history = true);
void spectrumTrace(const uint8_t *live, const uint8_t *peak, int left, int right, bool alert = false);
void logicFrame(const std::vector<std::vector<uint8_t>> &signals, int selected, const String &state);
void spectrumRow(const uint8_t *values);
void spectrumRuler(const int *columns, const String *labels, int count, int selected);
void graphicStatus(const String &text);
void dialFrame(const String &title, const String &value, const String &description, int progress);
void mediaFrame(const String &file, const String &state, const String &elapsed, const String &total,
                int progress, int volume, bool loop, int selected);
int takeMediaAction();
int selectValue(const String &title, int value, int minimum, int maximum, const std::function<void(int)> &preview);
void gameFrame(int sharkX, int sharkY, const std::vector<std::pair<int,int>> &fish, int score, bool paused);
void panelFrame(const String &title, const String &state, const std::vector<String> &fields);
bool inspect(const String &title, const String &state, const std::vector<String> &fields, const char *action = "Continuar");
int prompt(const String &title, const String &message, const std::vector<String> &actions, uint32_t automaticMs = 0, bool cycleKeys = false);
bool statusNotice(const String &text);
bool takeBackEvent();
void terminalWindow(const std::vector<String> &lines, const String &prompt);
void terminalReset(const String &title, const String &context = "");
void terminalAppend(const String &title, const String &text, bool newline);
void terminalPrompt(const String &text);
void diagnostics();
String collectRow(const std::function<void()> &draw);
bool notice(const String &text, const char *title);
}
#endif

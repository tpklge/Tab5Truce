#pragma once
#include <lvgl.h>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
namespace truce {
struct Palette {
    std::string name = "Claro";
    uint32_t background = 0xEDF1F7, panel = 0xFFFFFF, text = 0x142238;
    uint32_t muted = 0x526176, border = 0xB7C2D3, accent = 0xC02640;
};
struct Item {
    std::string label;
    std::string icon;
    std::string image;
    bool enabled = true;
    bool selected = false;
};
class View {
public:
    enum { None = -1, Back = -2, Home = -3, Themes = -4, Files = -5, Settings = -6, Submit = -7, Previous = -8, Next = -9 };
    void init(lv_display_t *display);
    void menu(const std::string &title, const std::vector<Item> &items, bool home, const Palette &palette);
    void input(const std::string &title, const std::string &text, size_t limit, bool password, int mode,
               const Palette &palette);
    void message(const std::string &title, const std::string &text, const Palette &palette,
                 const char *actionCaption = "Continuar");
    void slider(const std::string &title, int value, int minimum, int maximum, const Palette &palette);
    int sliderValue() const;
    void adjustSlider(int delta);
    void status(const std::string &text);
    void operation(const std::string &title, bool stopOnly, const Palette &palette);
    void operationControls(const std::string &previous, const std::string &next,
                           const std::string &action, const std::string &back = "Voltar");
    void dataPanel(const std::string &title, const Palette &palette);
    void dataText(const std::string &state, const std::vector<std::string> &lines);
    void document(const std::string &title, const std::string &text, const std::string &page, const Palette &palette);
    void spectrum(const std::string &title, bool sdr, const Palette &palette, bool history = true);
    void spectrumTrace(const uint8_t *live, const uint8_t *peak, int left, int right, bool alert);
    void logicSignals(const std::vector<std::vector<uint8_t>> &signals, int selected);
    void spectrumRow(const uint8_t *values);
    void spectrumRuler(const std::vector<int> &columns, const std::vector<std::string> &labels, int selected);
    void dial(const std::string &title, const Palette &palette);
    void dialValue(const std::string &value, const std::string &description, int progress);
    void media(const Palette &palette);
    void mediaValue(const std::string &file, const std::string &state, const std::string &elapsed,
                    const std::string &total, int progress, int volume, bool loop, int selected);
    void game(const Palette &palette);
    void gameFrame(int sharkX, int sharkY, const std::vector<std::pair<int,int>> &fish, int score, bool paused);
    void choice(const std::string &title, const std::string &message,
                const std::vector<std::string> &actions, const Palette &palette, bool automatic = false);
    void terminal(const std::string &title, const Palette &palette);
    void scrollTerminal(int direction);
    void terminalText(const std::string &output, const std::string &prompt);
    void operationText(const std::string &state, const std::string &details);
    void splash(const Palette &palette);
    void splashProgress(int percent);
    void focus(int index);
    void move(int step);
    void activate();
    void type(const std::string &text);
    void backspace();
    std::string text() const;
    int takeEvent();
    void clear();
    lv_obj_t *root() const { return root_; }
    lv_obj_t *body() const { return body_; }
    lv_obj_t *dock() const { return dock_; }
    lv_obj_t *dataGrid() const { return dataGrid_; }
    lv_obj_t *graph() const { return chart_; }
    lv_obj_t *waterfall() const { return waterfall_; }
    lv_obj_t *ruler() const { return ruler_; }
    lv_obj_t *mediaGrid() const { return mediaGrid_; }
    lv_obj_t *gameBoard() const { return board_; }
    lv_obj_t *button(size_t i) const { return i < buttons_.size() ? buttons_[i] : nullptr; }
    size_t count() const { return buttons_.size(); }
    int focused() const { return focused_; }
private:
    lv_obj_t *root_ = nullptr, *body_ = nullptr, *dock_ = nullptr, *status_ = nullptr;
    lv_obj_t *textarea_ = nullptr, *slider_ = nullptr, *value_ = nullptr;
    lv_obj_t *state_ = nullptr, *details_ = nullptr, *progress_ = nullptr;
    std::vector<lv_obj_t *> buttons_;
    Palette palette_;
    lv_obj_t *dataGrid_ = nullptr;
    std::vector<lv_obj_t *> dataCards_;
    std::vector<std::string> dataLines_;
    lv_obj_t *chart_ = nullptr, *waterfall_ = nullptr, *ruler_ = nullptr, *selection_ = nullptr;
    lv_chart_series_t *liveSeries_ = nullptr, *peakSeries_ = nullptr;
    uint16_t *waterfallPixels_ = nullptr;
    bool sdrPalette_ = false, traceValid_ = false, traceAlert_ = false;
    int traceLeft_ = -1, traceRight_ = -1;
    std::array<uint8_t,256> traceLive_{}, tracePeak_{};
    lv_obj_t *dial_ = nullptr, *digital_ = nullptr, *description_ = nullptr;
    lv_obj_t *track_ = nullptr, *elapsed_ = nullptr, *total_ = nullptr, *mediaGrid_ = nullptr;
    std::vector<lv_obj_t *> mediaButtons_;
    lv_obj_t *terminalScroll_=nullptr, *terminalPrompt_=nullptr;
    int mediaSelected_=-1;
    std::string mediaFile_, stateText_, detailsText_, terminalInputText_;
    lv_obj_t *board_ = nullptr, *shark_ = nullptr;
    std::vector<lv_obj_t *> fish_;
    int event_ = None, focused_ = 0;
    void shell(const std::string &title);
    lv_obj_t *action(lv_obj_t *parent, const char *label, int id);
    static void clicked(lv_event_t *event);
    static void keyEvent(lv_event_t *event);
};
const char *iconFor(const std::string &name);
const lv_font_t *localeFont(const lv_font_t *base);
void releaseLocaleFonts();
const lv_font_t *instrumentFont();
}

#if defined(TRUCE_LVGL) || defined(TRUCE_HOST)
#include "ui_view.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include "pt_br.h"
namespace truce {
struct LocaleFont { const lv_font_t *base; const char *path; lv_font_t *extra=nullptr; lv_font_t merged{}; };
static LocaleFont localeFonts[]={
    {&lv_font_montserrat_20,"T:/fonts/latin20.bin"},
    {&lv_font_montserrat_24,"T:/fonts/latin24.bin"},
    {&lv_font_montserrat_32,"T:/fonts/latin32.bin"}};
static lv_font_t *digitsFont=nullptr;
static lv_font_t *readBinaryFont(const char *path) {
    lv_fs_file_t file;
    if(lv_fs_open(&file,path,LV_FS_MODE_RD)!=LV_FS_RES_OK)return nullptr;
    uint32_t size=0,read=0;lv_font_t *font=nullptr;
    lv_fs_seek(&file,0,LV_FS_SEEK_END);lv_fs_tell(&file,&size);lv_fs_seek(&file,0,LV_FS_SEEK_SET);
    void *data=size>0 && size<=128*1024?lv_malloc(size):nullptr;
    if(data && lv_fs_read(&file,data,size,&read)==LV_FS_RES_OK && read==size)font=lv_binfont_create_from_buffer(data,size);
    lv_fs_close(&file);lv_free(data);return font;
}
const lv_font_t *instrumentFont() {
    if(!digitsFont)digitsFont=readBinaryFont("T:/fonts/digits64.bin");
    return digitsFont?digitsFont:&lv_font_montserrat_32;
}
const lv_font_t *localeFont(const lv_font_t *base) {
    for(auto &font:localeFonts)if(font.base==base) {
        if(!font.extra) {
            font.extra=readBinaryFont(font.path);
            if(font.extra) {
                font.merged=*base;font.merged.fallback=font.extra;
                font.merged.line_height=std::max(base->line_height,font.extra->line_height);
                font.merged.base_line=std::max(base->base_line,font.extra->base_line);
            }
        }
        return font.extra?&font.merged:base;
    }
    return base;
}
void releaseLocaleFonts() {
    if(digitsFont){lv_binfont_destroy(digitsFont);digitsFont=nullptr;}
    for(auto &font:localeFonts)if(font.extra){lv_binfont_destroy(font.extra);font.extra=nullptr;}
}

static void panel(lv_obj_t *obj, uint32_t color, int radius = 0) {
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, 0);

}
static lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font) {
    auto *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, localeFont(font), 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
const char *iconFor(const std::string &s) {
    if (s == "WiFi" || s == "Wifi") return LV_SYMBOL_WIFI;
    if (s == "Bluetooth" || s == "BLE") return LV_SYMBOL_BLUETOOTH;
    if (s == "Files" || s == "SD Card") return LV_SYMBOL_DIRECTORY;
    if (s == "Config" || s == "Settings") return LV_SYMBOL_SETTINGS;
    if (s == "Clock") return LV_SYMBOL_BELL;
    if (s == "Yes" || s == "Sim" || s == "Confirmar" || s == "Continuar")return LV_SYMBOL_OK;
    if (s == "No" || s == "Não" || s == "Cancelar")return LV_SYMBOL_CLOSE;
    if (s == "Play/Pause" || s == "Media Cmds")return LV_SYMBOL_PLAY;
    if (s == "Mute" || s.find("Volume")!=std::string::npos)return LV_SYMBOL_VOLUME_MAX;
    if (s == "Presenter mode")return LV_SYMBOL_RIGHT;
    if (s == "GPS" || s == "Latitude" || s == "Longitude" || s == "Distância" || s == "Satélites") return LV_SYMBOL_GPS;
    if (s == "UID" || s == "Dados da tag" || s == "Tipo de dispositivo") return LV_SYMBOL_SD_CARD;
    if (s == "Arquivo") return LV_SYMBOL_FILE;
    if (s == "Hora" || s == "Data") return LV_SYMBOL_BELL;
    if (s == "Protocolo" || s == "Sinais capturados" || s == "Sinal bruto") return LV_SYMBOL_DOWNLOAD;
    if (s == "Scripts") return LV_SYMBOL_FILE;
    if (s == "RFID") return LV_SYMBOL_SD_CARD;
    if (s == "Ethernet") return LV_SYMBOL_SHUFFLE;
    std::string name=s;
    std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
    if(name.find("scan")!=std::string::npos || name.find("search")!=std::string::npos)return LV_SYMBOL_EYE_OPEN;
    if(name.find("sniff")!=std::string::npos || name.find("monitor")!=std::string::npos)return LV_SYMBOL_EYE_OPEN;
    if(name.find("beacon")!=std::string::npos || name.find("wifi")!=std::string::npos)return LV_SYMBOL_WIFI;
    if(name.find("ble")!=std::string::npos || name.find("bluetooth")!=std::string::npos)return LV_SYMBOL_BLUETOOTH;
    if(name.find("read")!=std::string::npos || name.find("capture")!=std::string::npos || name=="rx")return LV_SYMBOL_DOWNLOAD;
    if(name.find("write")!=std::string::npos || name.find("send")!=std::string::npos || name=="tx")return LV_SYMBOL_UPLOAD;
    if(name.find("replay")!=std::string::npos || name.find("restart")!=std::string::npos)return LV_SYMBOL_REFRESH;
    if(name.find("save")!=std::string::npos)return LV_SYMBOL_SAVE;
    if(name.find("deauth")!=std::string::npos || name.find("stop")!=std::string::npos)return LV_SYMBOL_STOP;
    if(name.find("back")!=std::string::npos || name.find("exit")!=std::string::npos)return LV_SYMBOL_LEFT;
    if(name.find("config")!=std::string::npos || name.find("setting")!=std::string::npos)return LV_SYMBOL_SETTINGS;
    if(name.find("device")!=std::string::npos || name.find("client")!=std::string::npos)return LV_SYMBOL_LIST;
    return LV_SYMBOL_CHARGE;
}
void View::init(lv_display_t *display) {
    root_ = lv_obj_create(nullptr);
    lv_display_set_default(display);
    lv_screen_load(root_);
}
void View::clear() {
    buttons_.clear();
    dataCards_.clear(); dataLines_.clear(); dataGrid_ = nullptr;
    textarea_ = slider_ = value_ = state_ = details_ = progress_ = body_ = dock_ = status_ = nullptr;
    if (root_) lv_obj_clean(root_);
    lv_free(waterfallPixels_);waterfallPixels_=nullptr;
    chart_=waterfall_=ruler_=selection_=dial_=digital_=description_=nullptr;
    track_=elapsed_=total_=mediaGrid_=board_=shark_=terminalScroll_=terminalPrompt_=nullptr;
    liveSeries_=peakSeries_=nullptr;mediaButtons_.clear();mediaSelected_=-1;fish_.clear();
    traceLive_.fill(0);tracePeak_.fill(0);traceValid_=false;
    mediaFile_.clear();stateText_.clear();std::string().swap(detailsText_);std::string().swap(terminalInputText_);
    event_ = None;
}
void View::shell(const std::string &title) {
    clear();
    panel(root_, palette_.background);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_style_text_font(root_, localeFont(&lv_font_montserrat_20), 0);
    lv_obj_set_style_text_color(root_, lv_color_hex(palette_.text), 0);
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    const int w = lv_display_get_horizontal_resolution(lv_obj_get_display(root_));
    const int h = lv_display_get_vertical_resolution(lv_obj_get_display(root_));
    auto *header = lv_obj_create(root_);
    panel(header, palette_.panel);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, w, 88);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_color(header, lv_color_hex(palette_.text), 0);
    auto *brand = label(header, "TRUCE", &lv_font_montserrat_32);
    lv_obj_set_pos(brand, 24, 10);
    lv_obj_set_style_text_color(brand, lv_color_hex(palette_.accent), 0);
    auto *heading = label(header, pt(title).c_str(), &lv_font_montserrat_20);
    lv_obj_set_pos(heading, 24, 52);
    lv_obj_set_width(heading, w / 2 - 40);
    lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);
    status_ = label(header, "microSD /truce", &lv_font_montserrat_20);
    lv_obj_set_width(status_, w / 2 - 32);
    lv_obj_set_pos(status_, w / 2 + 8, 32);
    lv_obj_set_style_text_align(status_, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(status_, lv_color_hex(palette_.muted), 0);
    lv_label_set_long_mode(status_, LV_LABEL_LONG_DOT);
    body_ = lv_obj_create(root_);
    panel(body_, palette_.background);
    lv_obj_set_style_text_color(body_, lv_color_hex(palette_.text), 0);
    lv_obj_set_pos(body_, 0, 88);
    lv_obj_set_size(body_, w, h - 176);
    lv_obj_set_style_pad_all(body_, 24, 0);
    lv_obj_set_style_pad_gap(body_, 16, 0);
    lv_obj_set_scroll_dir(body_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(body_, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(body_, lv_color_hex(palette_.accent), LV_PART_SCROLLBAR);
    dock_ = lv_obj_create(root_);
    panel(dock_, palette_.panel);
    lv_obj_set_pos(dock_, 0, h - 88);
    lv_obj_set_size(dock_, w, 88);
    lv_obj_set_style_pad_all(dock_, 12, 0);
    lv_obj_set_style_pad_column(dock_, 12, 0);
    lv_obj_set_flex_flow(dock_, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(dock_, LV_OBJ_FLAG_SCROLLABLE);
}
void View::clicked(lv_event_t *event) {
    auto *self = static_cast<View *>(lv_event_get_user_data(event));
    auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(event));
    int id = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(obj))) - 100;
    self->event_ = id;
}
lv_obj_t *View::action(lv_obj_t *parent, const char *caption, int id) {
    auto *obj = lv_button_create(parent);
    panel(obj, palette_.panel, 14);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(palette_.border), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(palette_.accent), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(obj,lv_color_hex(palette_.background),LV_STATE_PRESSED);
    lv_obj_set_style_color_filter_dsc(obj, nullptr, LV_STATE_DISABLED);
    lv_obj_set_style_opa(obj, LV_OPA_50, LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(obj, lv_color_hex(palette_.accent), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(obj, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(obj, 2, LV_STATE_FOCUSED);
    lv_obj_set_user_data(obj, reinterpret_cast<void *>(static_cast<intptr_t>(id + 100)));
    lv_obj_add_event_cb(obj, clicked, LV_EVENT_CLICKED, this);
    lv_obj_set_style_text_color(obj, lv_color_hex(palette_.text), 0);
    lv_obj_set_height(obj, 64);
    if (caption) {
        auto *text = label(obj, caption, &lv_font_montserrat_20);
        lv_obj_set_width(text, LV_PCT(100));
        lv_label_set_long_mode(text, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(text);
    }
    return obj;
}
void View::menu(const std::string &title, const std::vector<Item> &items, bool home, const Palette &palette) {
    palette_ = palette;
    shell(title);
    lv_obj_set_flex_flow(body_, LV_FLEX_FLOW_ROW_WRAP);
    const int width = lv_display_get_horizontal_resolution(lv_obj_get_display(root_)) - 48;
    const int cols = std::max(1, (width + 16) / 224);
    const int cardWidth = (width - (cols - 1) * 16) / cols;
    const bool wifiList=title=="Redes Wi-Fi";
    const bool bleList=title=="Dispositivos BLE";

    for (size_t i = 0; i < items.size(); ++i) {
        const auto &item = items[i];
        const auto divider=item.label.rfind('\n');
        const bool hasDetail=divider!=std::string::npos;
        const std::string name=hasDetail?item.label.substr(0,divider):item.label;
        const std::string detail=hasDetail?item.label.substr(divider+1):"";
        auto *card = action(body_, nullptr, static_cast<int>(i));
        buttons_.push_back(card);
        lv_obj_set_size(card, cardWidth, home ? 184 : hasDetail ? 232 : 172);
        lv_obj_set_style_pad_all(card, 16, 0);
        if (!item.enabled) lv_obj_add_state(card, LV_STATE_DISABLED);
        if (item.selected) {
            lv_obj_set_style_border_color(card, lv_color_hex(palette_.accent), 0);
            lv_obj_set_style_border_width(card, 2, 0);
        }
        bool hasImage = false;
        lv_image_header_t info;
        if (home && !item.image.empty() && lv_image_decoder_get_info(item.image.c_str(), &info) == LV_RESULT_OK) {
            auto *image = lv_image_create(card);
            lv_image_set_src(image, item.image.c_str());
            lv_obj_set_size(image, 80, 80);
            lv_image_set_inner_align(image, LV_IMAGE_ALIGN_CONTAIN);
            lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 8);
            hasImage = true;
        }
        if (!hasImage) {
            const char *icon = item.icon.empty() ? (hasDetail && wifiList ? LV_SYMBOL_WIFI : hasDetail && bleList ? LV_SYMBOL_BLUETOOTH : iconFor(name)) : item.icon.c_str();
            auto *glyph = label(card, icon, &lv_font_montserrat_32);
            lv_obj_set_style_text_color(glyph, lv_color_hex(palette_.accent), 0);
            lv_obj_align(glyph, LV_ALIGN_TOP_MID, 0, hasDetail?8:24);
        }
        const std::string captionText=hasDetail && (wifiList || bleList)?name:pt(name);
        auto *caption = label(card, captionText.c_str(), &lv_font_montserrat_24);
        lv_obj_set_width(caption, LV_PCT(100));
        lv_obj_set_height(caption, home ? 32 : 56);
        lv_label_set_long_mode(caption, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
        if(hasDetail) {
            lv_obj_set_pos(caption,0,56);
            auto *metadata=label(card,detail.c_str(),&lv_font_montserrat_20);
            lv_obj_set_size(metadata,LV_PCT(100),64);lv_label_set_long_mode(metadata,LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_align(metadata,LV_TEXT_ALIGN_CENTER,0);
            lv_obj_set_style_text_color(metadata,lv_color_hex(palette_.muted),0);
            lv_obj_set_pos(metadata,0,128);
        } else lv_obj_align(caption, LV_ALIGN_BOTTOM_MID, 0, -4);
    }
    struct Dock { const char *label; int id; };
    const Dock homeDock[] = {{LV_SYMBOL_HOME " Inicio", Home}, {LV_SYMBOL_SD_CARD " Arquivos", Files},
                             {LV_SYMBOL_IMAGE " Temas", Themes}, {LV_SYMBOL_SETTINGS " Ajustes", Settings}};
    const Dock subDock[] = {{LV_SYMBOL_LEFT " Voltar", Back}, {LV_SYMBOL_HOME " Inicio", Home},
                            {LV_SYMBOL_IMAGE " Temas", Themes}};
    for (int i = 0; i < (home ? 4 : 3); ++i) {
        const auto &entry = home ? homeDock[i] : subDock[i];
        auto *obj = action(dock_, entry.label, entry.id);
        lv_obj_set_width(obj, 0);
        lv_obj_set_flex_grow(obj, 1);
    }
    focus(0);
    lv_obj_update_layout(root_);
}
void View::focus(int index) {
    if (buttons_.empty()) return;
    focused_ = std::max(0, std::min(index, static_cast<int>(buttons_.size()) - 1));
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (static_cast<int>(i) == focused_) lv_obj_add_state(buttons_[i], LV_STATE_FOCUSED);
        else lv_obj_remove_state(buttons_[i], LV_STATE_FOCUSED);
    }
    lv_obj_scroll_to_view(buttons_[focused_], LV_ANIM_OFF);
}
void View::move(int step) {
    if (buttons_.empty()) return;
    int next = focused_;
    for (size_t i = 0; i < buttons_.size(); ++i) {
        next = (next + step + static_cast<int>(buttons_.size())) % static_cast<int>(buttons_.size());
        if (!lv_obj_has_state(buttons_[next], LV_STATE_DISABLED)) { focus(next); return; }
    }
}
void View::activate() {
    if (!buttons_.empty() && !lv_obj_has_state(buttons_[focused_], LV_STATE_DISABLED))
        lv_obj_send_event(buttons_[focused_], LV_EVENT_CLICKED, nullptr);
}
void View::keyEvent(lv_event_t *event) {
    auto *self = static_cast<View *>(lv_event_get_user_data(event));
    if (lv_event_get_code(event) == LV_EVENT_READY) self->event_ = Submit;
    if (lv_event_get_code(event) == LV_EVENT_CANCEL) self->event_ = Back;
}
void View::input(const std::string &title, const std::string &text, size_t limit, bool password,
                 int mode, const Palette &palette) {
    palette_ = palette;
    shell(title);
    lv_obj_remove_flag(body_, LV_OBJ_FLAG_SCROLLABLE);
    textarea_ = lv_textarea_create(body_);
    lv_obj_set_size(textarea_, LV_PCT(100), 100);
    lv_textarea_set_one_line(textarea_, true);
    lv_textarea_set_max_length(textarea_, limit);
    lv_textarea_set_password_mode(textarea_, password);
    if (mode == 1) lv_textarea_set_accepted_chars(textarea_, "0123456789abcdefABCDEF");
    if (mode == 2) lv_textarea_set_accepted_chars(textarea_, "0123456789.-");
    lv_textarea_set_text(textarea_, text.c_str());
    panel(textarea_, palette_.panel, 12);
    lv_obj_set_style_text_color(textarea_, lv_color_hex(palette_.text), 0);
    lv_obj_add_state(textarea_, LV_STATE_FOCUSED);
    lv_obj_update_layout(root_);
    auto *keyboard = lv_keyboard_create(body_);
    lv_obj_set_size(keyboard, LV_PCT(100), lv_obj_get_height(body_) - 164);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(keyboard, mode == 2 ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(keyboard, textarea_);
    lv_obj_set_style_pad_all(keyboard,8,0);
    lv_obj_set_style_pad_row(keyboard,8,LV_PART_ITEMS);
    lv_obj_set_style_pad_column(keyboard,8,LV_PART_ITEMS);
    lv_obj_set_style_radius(keyboard,10,LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(keyboard,LV_OPA_COVER,0);
    lv_obj_set_style_bg_opa(keyboard,LV_OPA_COVER,LV_PART_ITEMS);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(palette_.panel), 0);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(palette_.background), LV_PART_ITEMS);
    lv_obj_set_style_text_color(keyboard, lv_color_hex(palette_.text), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(palette_.accent), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_event_cb(keyboard, keyEvent, LV_EVENT_READY, this);
    lv_obj_add_event_cb(keyboard, keyEvent, LV_EVENT_CANCEL, this);
    for (const auto &entry : std::vector<std::pair<const char *, int>>{{"Cancelar", Back}, {"Confirmar", Submit}}) {
        auto *obj = action(dock_, entry.first, entry.second);
        lv_obj_set_width(obj, 0);
        lv_obj_set_flex_grow(obj, 1);
    }
    lv_obj_update_layout(root_);
}
void View::message(const std::string &title, const std::string &text, const Palette &palette, const char *actionCaption) {
    palette_ = palette;
    shell(title);
    auto *caption = label(body_, text.c_str(), &lv_font_montserrat_24);
    lv_obj_set_width(caption, LV_PCT(100));
    auto *retry = action(dock_, actionCaption, Submit);
    lv_obj_set_width(retry, LV_PCT(100));
    lv_obj_update_layout(root_);
}
void View::slider(const std::string &title, int value, int minimum, int maximum, const Palette &palette) {
    palette_ = palette; shell(title);
    value_ = label(body_, std::to_string(value).c_str(), &lv_font_montserrat_32);
    lv_obj_align(value_, LV_ALIGN_TOP_MID, 0, 64);
    slider_ = lv_slider_create(body_);
    lv_obj_set_size(slider_, LV_PCT(80), 48);
    lv_obj_center(slider_);
    lv_slider_set_range(slider_, minimum, maximum);
    lv_slider_set_value(slider_, value, LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(slider_,LV_OPA_COVER,LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider_,LV_OPA_COVER,LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider_,LV_OPA_COVER,LV_PART_KNOB);
    lv_obj_set_style_radius(slider_,12,LV_PART_MAIN);
    lv_obj_set_style_radius(slider_,12,LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider_,16,LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider_, lv_color_hex(palette_.border), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_, lv_color_hex(palette_.accent), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_, lv_color_hex(palette_.text), LV_PART_KNOB);
    lv_obj_add_event_cb(slider_, [](lv_event_t *event) {
        auto *self = static_cast<View *>(lv_event_get_user_data(event));
        lv_label_set_text(self->value_, std::to_string(self->sliderValue()).c_str());
    }, LV_EVENT_VALUE_CHANGED, this);
    for (const auto &entry : std::vector<std::pair<const char *, int>>{{"Cancelar", Back}, {"Aplicar", Submit}}) {
        auto *obj = action(dock_, entry.first, entry.second);
        lv_obj_set_width(obj, 0); lv_obj_set_flex_grow(obj, 1);
    }
    lv_obj_update_layout(root_);
}
int View::sliderValue() const { return slider_ ? lv_slider_get_value(slider_) : 0; }
void View::adjustSlider(int delta) {
    if (!slider_) return;
    lv_slider_set_value(slider_, sliderValue() + delta, LV_ANIM_OFF);
    lv_obj_send_event(slider_, LV_EVENT_VALUE_CHANGED, nullptr);
}
void View::operation(const std::string &title, bool stopOnly, const Palette &palette) {
    palette_ = palette; shell(title);
    const int w = lv_display_get_horizontal_resolution(lv_obj_get_display(root_));
    const int h = lv_display_get_vertical_resolution(lv_obj_get_display(root_));
    const int rail = w >= 1000 ? 176 : 144;
    lv_obj_set_size(body_, w - rail, h - 88);
    lv_obj_set_pos(dock_, w - rail, 88);
    lv_obj_set_size(dock_, rail, h - 88);
    lv_obj_set_flex_flow(dock_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(dock_, 16, 0);
    state_ = label(body_, "Em execução", &lv_font_montserrat_32);
    lv_obj_set_width(state_, LV_PCT(100));
    lv_obj_set_height(state_, 88);
    lv_label_set_long_mode(state_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(state_, lv_color_hex(palette_.text), 0);
    details_ = label(body_, "", &lv_font_montserrat_24);
    lv_obj_set_pos(details_, 0, 112);
    lv_obj_set_width(details_, LV_PCT(100));
    lv_obj_set_height(details_, h - 88 - 180);
    lv_label_set_long_mode(details_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(details_, lv_color_hex(palette_.muted), 0);
    struct Control { const char *caption; int id; };
    const Control controls[]={{LV_SYMBOL_UP "\nAnterior",Previous}, {LV_SYMBOL_DOWN "\nPróximo",Next},
                              {LV_SYMBOL_OK "\nAção",Submit}, {LV_SYMBOL_STOP "\nParar",Back}};
    for(const auto &c:controls) {
        auto *obj=action(dock_,c.caption,c.id);
        lv_obj_set_width(obj,LV_PCT(100));
        lv_obj_set_height(obj,0);lv_obj_set_flex_grow(obj,1);
        if(stopOnly && c.id!=Back) lv_obj_add_state(obj,LV_STATE_DISABLED);
    }
    lv_obj_update_layout(root_);
}
void View::operationControls(const std::string &previous, const std::string &next,
                             const std::string &actionText, const std::string &back) {
    const std::string captions[]={std::string(LV_SYMBOL_UP)+"\n"+previous,
        std::string(LV_SYMBOL_DOWN)+"\n"+next, std::string(LV_SYMBOL_OK)+"\n"+actionText,
        std::string(LV_SYMBOL_LEFT)+"\n"+back};
    if(!dock_ || lv_obj_get_child_count(dock_)!=4)return;
    for(unsigned i=0;i<4;++i) {
        auto *button=lv_obj_get_child(dock_,i);
        auto *text=lv_obj_get_child(button,0);
        if(captions[i]!=lv_label_get_text(text))lv_label_set_text(text,captions[i].c_str());
        const std::string *values[]={&previous,&next,&actionText,&back};
        if(values[i]->empty())lv_obj_add_state(button,LV_STATE_DISABLED);
        else lv_obj_remove_state(button,LV_STATE_DISABLED);
    }
}
void View::dataPanel(const std::string &title, const Palette &palette) {
    operation(title,false,palette);
    lv_obj_delete(details_);details_=nullptr;
    dataGrid_=lv_obj_create(body_);panel(dataGrid_,palette_.background);
    lv_obj_set_pos(dataGrid_,0,112);lv_obj_set_size(dataGrid_,LV_PCT(100),
        lv_display_get_vertical_resolution(lv_obj_get_display(root_))-88-160);
    lv_obj_set_style_pad_all(dataGrid_,0,0);
    lv_obj_set_style_pad_row(dataGrid_,16,0);lv_obj_set_style_pad_column(dataGrid_,16,0);
    lv_obj_set_flex_flow(dataGrid_,LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_scroll_dir(dataGrid_,LV_DIR_VER);
    operationControls("","","Opções");
}
void View::dataText(const std::string &state, const std::vector<std::string> &lines) {
    operationText(state,"");
    if(!dataGrid_ || dataLines_==lines)return;
    dataLines_=lines;
    const size_t count=std::min<size_t>(lines.size(),48);
    while(dataCards_.size()>count){lv_obj_delete(dataCards_.back());dataCards_.pop_back();}
    lv_obj_update_layout(root_);
    const int available=lv_obj_get_content_width(dataGrid_);
    const int columns=std::max(1,(available+16)/280);
    const int cardWidth=(available-(columns-1)*16)/columns;
    for(size_t i=0;i<count;++i) {
        if(i==dataCards_.size()) {
            auto *card=lv_obj_create(dataGrid_);panel(card,palette_.panel,12);
            lv_obj_set_style_pad_all(card,16,0);lv_obj_remove_flag(card,LV_OBJ_FLAG_SCROLLABLE);
            auto *heading=label(card,"",&lv_font_montserrat_20);
            lv_obj_set_style_text_color(heading,lv_color_hex(palette_.muted),0);
            auto *value=label(card,"",&lv_font_montserrat_24);
            lv_obj_set_style_text_color(value,lv_color_hex(palette_.text),0);
            lv_obj_set_pos(value,0,36);
            dataCards_.push_back(card);
        }
        auto *card=dataCards_[i];lv_obj_set_size(card,cardWidth,LV_SIZE_CONTENT);
        auto *heading=lv_obj_get_child(card,0),*value=lv_obj_get_child(card,1);
        const auto colon=lines[i].find(':');
        const std::string caption=colon==std::string::npos?"Detalhes":pt(lines[i].substr(0,colon));
        const std::string content=colon==std::string::npos?pt(lines[i]):lines[i].substr(colon+1);
        const std::string title=std::string(iconFor(caption))+"  "+caption;
        lv_obj_set_width(heading,cardWidth-32);lv_obj_set_height(heading,28);
        lv_label_set_long_mode(heading,LV_LABEL_LONG_DOT);
        lv_obj_set_width(value,cardWidth-32);
        lv_label_set_long_mode(value,LV_LABEL_LONG_WRAP);
        if(title!=lv_label_get_text(heading))lv_label_set_text(heading,title.c_str());
        if(content!=lv_label_get_text(value))lv_label_set_text(value,content.c_str());
    }
    lv_obj_update_layout(root_);
}
void View::document(const std::string &title,const std::string &text,const std::string &page,const Palette &palette) {
    operation(title,false,palette);
    operationControls("Página anterior","Próxima página","Continuar");
    operationText(page,"");
    auto *scroller=lv_obj_create(body_);panel(scroller,palette_.panel,12);
    lv_obj_set_pos(scroller,0,112);lv_obj_set_size(scroller,LV_PCT(100),
        lv_display_get_vertical_resolution(lv_obj_get_display(root_))-88-160);
    lv_obj_set_style_pad_all(scroller,16,0);
    lv_obj_set_scroll_dir(scroller,LV_DIR_VER);
    lv_obj_delete(details_);details_=label(scroller,text.c_str(),&lv_font_montserrat_24);
    lv_obj_set_style_text_color(details_,lv_color_hex(palette_.text),0);
    lv_obj_set_width(details_,LV_PCT(100));lv_label_set_long_mode(details_,LV_LABEL_LONG_WRAP);
    lv_obj_update_layout(root_);
}
static void changedText(lv_obj_t *obj,const std::string &text) {
    if(obj && text!=lv_label_get_text(obj))lv_label_set_text(obj,text.c_str());
}
void View::spectrum(const std::string &title,bool sdr,const Palette &palette,bool history) {
    operation(title,false,palette);lv_obj_delete(details_);details_=nullptr;
    lv_obj_remove_flag(body_,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_height(state_,48);lv_obj_update_layout(root_);
    const int w=lv_obj_get_content_width(body_),h=lv_obj_get_content_height(body_);
    const int chartHeight=history?(h-152)*2/3:h-128,waterHeight=h-152-chartHeight;
    chart_=lv_chart_create(body_);panel(chart_,palette_.panel,12);
    lv_obj_set_pos(chart_,0,64);lv_obj_set_size(chart_,w,chartHeight);
    lv_chart_set_type(chart_,LV_CHART_TYPE_LINE);lv_chart_set_point_count(chart_,256);
    lv_chart_set_range(chart_,LV_CHART_AXIS_PRIMARY_Y,0,100);lv_chart_set_div_line_count(chart_,5,8);
    lv_obj_set_style_size(chart_,0,0,LV_PART_INDICATOR);
    lv_obj_set_style_line_color(chart_,lv_color_hex(palette_.border),LV_PART_MAIN);
    lv_obj_set_style_line_width(chart_,2,LV_PART_ITEMS);
    peakSeries_=lv_chart_add_series(chart_,lv_color_hex(palette_.muted),LV_CHART_AXIS_PRIMARY_Y);
    liveSeries_=lv_chart_add_series(chart_,lv_color_hex(palette_.accent),LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart_,liveSeries_,0);lv_chart_set_all_value(chart_,peakSeries_,0);
    selection_=lv_obj_create(chart_);panel(selection_,palette_.accent,0);
    lv_obj_set_style_bg_opa(selection_,LV_OPA_20,0);lv_obj_add_flag(selection_,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(selection_,LV_OBJ_FLAG_CLICKABLE);
    sdrPalette_=sdr;
    waterfallPixels_=history?static_cast<uint16_t *>(lv_malloc_zeroed(256*48*2)):nullptr;
    if(waterfallPixels_) {
        waterfall_=lv_canvas_create(body_);
        lv_canvas_set_buffer(waterfall_,waterfallPixels_,256,48,LV_COLOR_FORMAT_RGB565);
        lv_obj_set_pos(waterfall_,0,76+chartHeight);lv_obj_set_size(waterfall_,w,waterHeight);
        lv_image_set_inner_align(waterfall_,LV_IMAGE_ALIGN_STRETCH);
    }
    ruler_=lv_obj_create(body_);panel(ruler_,palette_.background);
    lv_obj_set_style_pad_all(ruler_,0,0);lv_obj_set_size(ruler_,w,48);
    lv_obj_set_pos(ruler_,0,h-48);lv_obj_remove_flag(ruler_,LV_OBJ_FLAG_SCROLLABLE);
    operationControls("Anterior","Próximo","Opções","Parar");
}
void View::logicSignals(const std::vector<std::vector<uint8_t>> &signals,int selected) {
    if(!chart_)return;
    while(auto *series=lv_chart_get_series_next(chart_,nullptr))lv_chart_remove_series(chart_,series);
    liveSeries_=peakSeries_=nullptr;
    lv_chart_set_point_count(chart_,512);
    lv_chart_set_range(chart_,LV_CHART_AXIS_PRIMARY_Y,0,400);
    lv_chart_set_div_line_count(chart_,5,9);
    if(selection_)lv_obj_add_flag(selection_,LV_OBJ_FLAG_HIDDEN);
    for(size_t lane=0;lane<signals.size() && lane<4;++lane) {
        auto *series=lv_chart_add_series(chart_,lv_color_hex(int(lane)==selected?palette_.accent:palette_.muted),LV_CHART_AXIS_PRIMARY_Y);
        auto *values=lv_chart_get_y_array(chart_,series);
        for(int x=0;x<256;++x) {
            const uint8_t level=x<int(signals[lane].size())?signals[lane][x]:0;
            const int base=int(3-lane)*100+10;
            values[2*x]=base+(level==1?65:0);
            values[2*x+1]=base+(level?65:0);
        }
    }
    lv_chart_refresh(chart_);
}
void View::spectrumTrace(const uint8_t *live,const uint8_t *peak,int left,int right,bool alert) {
    if(!chart_ || !live)return;
    std::array<uint8_t,256> current{},maximum{};
    for(size_t i=0;i<256;++i){current[i]=std::min<int>(100,live[i]);maximum[i]=peak?std::min<int>(100,peak[i]):current[i];}
    if(traceValid_ && traceLive_==current && tracePeak_==maximum && traceAlert_==alert && traceLeft_==left && traceRight_==right)return;
    traceValid_=true;traceLive_=std::move(current);tracePeak_=std::move(maximum);
    traceAlert_=alert;traceLeft_=left;traceRight_=right;
    auto *a=lv_chart_get_y_array(chart_,liveSeries_),*b=lv_chart_get_y_array(chart_,peakSeries_);
    for(size_t i=0;i<256;++i){a[i]=traceLive_[i];b[i]=tracePeak_[i];}
    lv_chart_set_series_color(chart_,liveSeries_,lv_color_hex(alert?0xD97706:palette_.accent));
    if(left<=right && right>=0 && left<256) {
        left=std::max(0,left);right=std::min(255,right);
        lv_obj_set_pos(selection_,left*lv_obj_get_content_width(chart_)/256,0);
        lv_obj_set_size(selection_,std::max<int32_t>(1,(right-left+1)*lv_obj_get_content_width(chart_)/256),lv_obj_get_content_height(chart_));
        lv_obj_remove_flag(selection_,LV_OBJ_FLAG_HIDDEN);
    } else lv_obj_add_flag(selection_,LV_OBJ_FLAG_HIDDEN);
    lv_chart_refresh(chart_);
}
void View::spectrumRow(const uint8_t *values) {
    if(!waterfallPixels_ || !values)return;
    std::memmove(waterfallPixels_+256,waterfallPixels_,256*47*2);
    for(int i=0;i<256;++i) {
        const int v=std::min<int>(100,values[i]);
        lv_color_t color;
        if(sdrPalette_ && v>0)color=lv_color_hsv_to_rgb((100-v)*240/100,100,std::max(12,v));
        else color=lv_color_mix(lv_color_hex(palette_.accent),lv_color_hex(palette_.panel),v*255/100);
        waterfallPixels_[i]=lv_color_to_u16(color);
    }
    lv_obj_invalidate(waterfall_);
}
void View::spectrumRuler(const std::vector<int> &columns,const std::vector<std::string> &labels,int selected) {
    if(!ruler_)return;
    const size_t n=std::min<size_t>(16,std::min(columns.size(),labels.size()));
    while(lv_obj_get_child_count(ruler_)>n)lv_obj_delete(lv_obj_get_child(ruler_,-1));
    for(size_t i=0;i<n;++i) {
        auto *text=i<lv_obj_get_child_count(ruler_)?lv_obj_get_child(ruler_,i):label(ruler_,"",&lv_font_montserrat_20);
        const int totalWidth=lv_obj_get_width(ruler_);
        const int left=i?std::max(0,(columns[i-1]+columns[i])*totalWidth/510):0;
        const int right=i+1<n?std::min(totalWidth,(columns[i]+columns[i+1])*totalWidth/510):totalWidth;
        const int w=std::max(1,std::min(120,right-left-4));
        lv_obj_set_width(text,w);lv_obj_set_height(text,48);lv_label_set_long_mode(text,LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(text,LV_TEXT_ALIGN_CENTER,0);
        lv_obj_set_style_text_color(text,lv_color_hex(int(i)==selected?palette_.accent:palette_.muted),0);
        lv_obj_set_pos(text,std::max(left,std::min(right-w,columns[i]*totalWidth/255-w/2)),0);
        changedText(text,labels[i]);
    }
}
void View::dial(const std::string &title,const Palette &palette) {
    operation(title,false,palette);lv_obj_delete(details_);details_=nullptr;
    lv_obj_set_height(state_,48);
    lv_obj_remove_flag(body_,LV_OBJ_FLAG_SCROLLABLE);lv_obj_update_layout(root_);
    const int w=lv_obj_get_content_width(body_),h=lv_obj_get_content_height(body_);
    const int diameter=std::min(w-24,h-160);
    dial_=lv_arc_create(body_);panel(dial_,palette_.background,LV_RADIUS_CIRCLE);lv_obj_set_size(dial_,diameter,diameter);
    lv_obj_set_pos(dial_,(w-diameter)/2,80);
    lv_arc_set_range(dial_,0,1000);lv_arc_set_rotation(dial_,270);
    lv_arc_set_bg_angles(dial_,0,360);lv_arc_set_value(dial_,0);
    lv_obj_set_style_arc_color(dial_,lv_color_hex(palette_.border),LV_PART_MAIN);
    lv_obj_set_style_arc_color(dial_,lv_color_hex(palette_.accent),LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(dial_,10,LV_PART_MAIN);lv_obj_set_style_arc_width(dial_,10,LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(dial_,LV_OPA_TRANSP,LV_PART_KNOB);
    lv_obj_set_style_border_width(dial_,0,LV_PART_KNOB);lv_obj_set_style_opa(dial_,LV_OPA_TRANSP,LV_PART_KNOB);lv_obj_remove_flag(dial_,LV_OBJ_FLAG_CLICKABLE);
    digital_=label(body_,"",instrumentFont());
    const int zoom=digitsFont?256:512;
    lv_obj_set_style_transform_scale_x(digital_,zoom,0);lv_obj_set_style_transform_scale_y(digital_,zoom,0);
    lv_obj_set_style_transform_pivot_x(digital_,LV_PCT(50),0);lv_obj_set_style_transform_pivot_y(digital_,LV_PCT(50),0);
    lv_obj_set_style_text_color(digital_,lv_color_hex(palette_.text),0);
    description_=label(body_,"",&lv_font_montserrat_24);
    lv_obj_set_width(description_,w-24);lv_obj_set_height(description_,80);
    lv_obj_set_style_text_align(description_,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(description_,lv_color_hex(palette_.muted),0);
    lv_label_set_long_mode(description_,LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(description_,12,80+diameter/2+56);
}
void View::dialValue(const std::string &value,const std::string &description,int progress) {
    if(!digital_)return;
    changedText(digital_,value);changedText(description_,description);
    lv_obj_update_layout(root_);
    const int w=lv_obj_get_content_width(body_);
    lv_obj_set_pos(digital_,(w-lv_obj_get_width(digital_))/2,lv_obj_get_y(dial_)+lv_obj_get_height(dial_)/2-lv_obj_get_height(digital_)/2);
    lv_arc_set_value(dial_,std::max(0,std::min(1000,progress)));
}
void View::media(const Palette &palette) {
    operation("Reprodutor de áudio",false,palette);lv_obj_delete(details_);details_=nullptr;
    lv_obj_set_height(state_,48);
    lv_obj_remove_flag(body_,LV_OBJ_FLAG_SCROLLABLE);lv_obj_update_layout(root_);
    const int w=lv_obj_get_content_width(body_),h=lv_obj_get_content_height(body_);
    track_=label(body_,"",&lv_font_montserrat_32);lv_obj_set_pos(track_,0,64);
    lv_obj_set_size(track_,w,100);lv_label_set_long_mode(track_,LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(track_,lv_color_hex(palette_.text),0);
    progress_=lv_bar_create(body_);lv_obj_set_pos(progress_,0,184);lv_obj_set_size(progress_,w,12);
    lv_bar_set_range(progress_,0,1000);
    lv_obj_set_style_border_width(progress_,0,0);lv_obj_set_style_radius(progress_,6,0);
    lv_obj_set_style_bg_opa(progress_,LV_OPA_COVER,LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_,LV_OPA_COVER,LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(progress_,lv_color_hex(palette_.border),LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_,lv_color_hex(palette_.accent),LV_PART_INDICATOR);
    elapsed_=label(body_,"",&lv_font_montserrat_20);lv_obj_set_pos(elapsed_,0,212);
    total_=label(body_,"",&lv_font_montserrat_20);lv_obj_set_width(total_,w/2);
    lv_obj_set_style_text_align(total_,LV_TEXT_ALIGN_RIGHT,0);lv_obj_set_pos(total_,w/2,212);
    lv_obj_set_style_text_color(elapsed_,lv_color_hex(palette_.muted),0);
    lv_obj_set_style_text_color(total_,lv_color_hex(palette_.muted),0);
    mediaGrid_=lv_obj_create(body_);panel(mediaGrid_,palette_.background);
    lv_obj_remove_flag(mediaGrid_,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(mediaGrid_,6,0);lv_obj_set_style_pad_row(mediaGrid_,16,0);
    lv_obj_set_style_pad_column(mediaGrid_,16,0);lv_obj_set_flex_flow(mediaGrid_,LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_pos(mediaGrid_,0,264);lv_obj_set_size(mediaGrid_,w,h-264);
    const char *captions[]={LV_SYMBOL_PREV "\nReiniciar",LV_SYMBOL_PLAY "\nReproduzir",LV_SYMBOL_VOLUME_MAX "\nVolume",LV_SYMBOL_LOOP "\nRepetir"};
    for(int i=0;i<4;++i){auto *button=action(mediaGrid_,captions[i],100+i);lv_obj_set_size(button,(w-28)/2,(h-292)/2);lv_obj_remove_flag(button,LV_OBJ_FLAG_SCROLL_ON_FOCUS);mediaButtons_.push_back(button);}
    operationControls("Anterior","Próximo","Selecionar","Sair");
}
void View::mediaValue(const std::string &file,const std::string &state,const std::string &elapsed,
                      const std::string &total,int progress,int volume,bool loop,int selected) {
    if(!track_)return;
    operationText(state,"");
    if(file!=mediaFile_){lv_label_set_text(track_,file.c_str());mediaFile_=file;}
    changedText(elapsed_,elapsed);changedText(total_,total);
    lv_bar_set_value(progress_,std::max(0,std::min(1000,progress)),LV_ANIM_OFF);
    const std::string captions[]={std::string(LV_SYMBOL_PREV)+"\nReiniciar",
        std::string(state=="Reproduzindo"?LV_SYMBOL_PAUSE:LV_SYMBOL_PLAY)+(state=="Reproduzindo"?"\nPausar":"\nReproduzir"),
        std::string(LV_SYMBOL_VOLUME_MAX)+"\nVolume "+std::to_string(volume)+"%",
        std::string(LV_SYMBOL_LOOP)+(loop?"\nRepetir: ligado":"\nRepetir: desligado")};
    for(size_t i=0;i<mediaButtons_.size();++i){
        changedText(lv_obj_get_child(mediaButtons_[i],0),captions[i]);
        if(selected!=mediaSelected_) {
            lv_obj_set_style_border_width(mediaButtons_[i],int(i)==selected?3:1,0);
            lv_obj_set_style_border_color(mediaButtons_[i],lv_color_hex(int(i)==selected?palette_.accent:palette_.border),0);
        }
    }
    mediaSelected_=selected;
}
void View::game(const Palette &palette) {
    operation("Megalodon",false,palette);lv_obj_delete(details_);details_=nullptr;
    lv_obj_set_height(state_,48);
    lv_obj_remove_flag(body_,LV_OBJ_FLAG_SCROLLABLE);lv_obj_update_layout(root_);
    const int availableW=lv_obj_get_content_width(body_),availableH=lv_obj_get_content_height(body_)-96;
    const int w=std::min(availableW,availableH*320/180),h=w*180/320;
    board_=lv_obj_create(body_);panel(board_,palette_.panel,16);
    lv_obj_set_style_pad_all(board_,0,0);lv_obj_set_pos(board_,(availableW-w)/2,80);
    lv_obj_set_size(board_,w,h);lv_obj_remove_flag(board_,LV_OBJ_FLAG_SCROLLABLE);
    shark_=lv_image_create(board_);lv_image_set_src(shark_,"T:/boot/shark.png");
    lv_image_set_inner_align(shark_,LV_IMAGE_ALIGN_CONTAIN);lv_obj_set_size(shark_,48*w/320,30*h/180);
    for(int i=0;i<5;++i){auto *fish=lv_obj_create(board_);panel(fish,palette_.accent,LV_RADIUS_CIRCLE);
        lv_obj_set_style_pad_all(fish,0,0);lv_obj_set_size(fish,20*w/320,8*h/180);
        auto *eye=lv_obj_create(fish);panel(eye,palette_.background,LV_RADIUS_CIRCLE);
        lv_obj_set_size(eye,std::max(3,w/160),std::max(3,w/160));lv_obj_set_pos(eye,w/160,h/60);
        auto *tail=lv_obj_create(fish);panel(tail,palette_.accent,0);
        const int fin=std::max(4,5*w/320);lv_obj_set_size(tail,fin,fin);
        lv_obj_set_pos(tail,16*w/320,4*h/180-fin/2);lv_obj_set_style_transform_rotation(tail,450,0);
        lv_obj_add_flag(fish,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_remove_flag(fish,LV_OBJ_FLAG_SCROLLABLE);fish_.push_back(fish);}
    operationControls("Subir","Descer","Pausar","Sair");
    for(int i=0;i<2;++i) {
        auto *button=lv_obj_get_child(dock_,i);
        lv_obj_remove_event_cb(button,clicked);
        lv_obj_add_event_cb(button,clicked,LV_EVENT_PRESSED,this);
        lv_obj_add_event_cb(button,clicked,LV_EVENT_LONG_PRESSED_REPEAT,this);
    }
}
void View::gameFrame(int sharkX,int sharkY,const std::vector<std::pair<int,int>> &fish,int score,bool paused) {
    if(!board_)return;
    const int w=lv_obj_get_width(board_),h=lv_obj_get_height(board_);
    operationText("Pontos: "+std::to_string(score)+(paused?" • Pausado":""),"");
    lv_obj_set_pos(shark_,sharkX*w/320,sharkY*h/180);
    for(size_t i=0;i<fish_.size();++i) {
        if(i<fish.size()){lv_obj_remove_flag(fish_[i],LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(fish_[i],fish[i].first*w/320,fish[i].second*h/180);}
        else lv_obj_add_flag(fish_[i],LV_OBJ_FLAG_HIDDEN);
    }
    operationControls("Subir","Descer",paused?"Continuar":"Pausar","Sair");
}
void View::choice(const std::string &title,const std::string &message,
                  const std::vector<std::string> &actions,const Palette &palette,bool automatic) {
    operation(title,false,palette);lv_obj_delete(details_);details_=nullptr;
    lv_obj_set_flex_flow(body_,LV_FLEX_FLOW_COLUMN);
    lv_label_set_long_mode(state_,LV_LABEL_LONG_WRAP);lv_obj_set_height(state_,LV_SIZE_CONTENT);
    lv_label_set_text(state_,pt(message).c_str());
    const int w=lv_obj_get_content_width(body_);
    for(size_t i=0;i<actions.size();++i) {
        const auto caption=std::string(iconFor(actions[i]))+"  "+pt(actions[i]);
        auto *card=action(body_,caption.c_str(),int(i));
        lv_obj_set_size(card,w,96);buttons_.push_back(card);
    }
    operationControls(actions.size()>2?pt(actions[2]):"",actions.size()>1?pt(actions[1]):"",
        actions.empty()?(automatic?"":"Continuar"):pt(actions[0]),automatic?"":"Cancelar");
    lv_obj_update_layout(root_);
}
void View::terminal(const std::string &title,const Palette &palette) {
    operation(title,false,palette);lv_obj_delete(details_);details_=nullptr;
    lv_obj_set_height(state_,48);lv_obj_remove_flag(body_,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(root_);
    const int h=lv_obj_get_content_height(body_);
    terminalScroll_=lv_obj_create(body_);panel(terminalScroll_,palette_.panel,12);
    lv_obj_set_style_pad_all(terminalScroll_,16,0);lv_obj_set_pos(terminalScroll_,0,64);
    lv_obj_set_size(terminalScroll_,LV_PCT(100),h-144);lv_obj_set_scroll_dir(terminalScroll_,LV_DIR_VER);
    details_=label(terminalScroll_,"",&lv_font_montserrat_20);
    lv_obj_set_style_text_color(details_,lv_color_hex(palette_.text),0);
    lv_obj_set_width(details_,LV_PCT(100));lv_label_set_long_mode(details_,LV_LABEL_LONG_WRAP);
    terminalPrompt_=label(body_,"",&lv_font_montserrat_24);
    lv_obj_set_style_text_color(terminalPrompt_,lv_color_hex(palette_.accent),0);
    lv_obj_set_size(terminalPrompt_,LV_PCT(100),56);lv_obj_set_pos(terminalPrompt_,0,h-56);
    lv_label_set_long_mode(terminalPrompt_,LV_LABEL_LONG_DOT);
    operationText("Terminal conectado","");operationControls("","","","Sair");
}
void View::scrollTerminal(int direction) {
    if(!terminalScroll_)return;
    const int current=lv_obj_get_scroll_y(terminalScroll_);
    const int maximum=std::max(0,current+int(lv_obj_get_scroll_bottom(terminalScroll_)));
    const int target=std::max(0,std::min(maximum,current+direction*int(lv_obj_get_content_height(terminalScroll_))/2));
    lv_obj_scroll_to_y(terminalScroll_,target,LV_ANIM_OFF);
}
void View::terminalText(const std::string &output,const std::string &prompt) {
    if(!terminalScroll_)return;
    const bool follow=detailsText_.empty() || lv_obj_get_scroll_bottom(terminalScroll_)<=2;
    if(output!=detailsText_) {
        lv_label_set_text(details_,output.c_str());detailsText_=output;lv_obj_update_layout(root_);
        if(follow)lv_obj_scroll_to_y(terminalScroll_,lv_obj_get_scroll_y(terminalScroll_)+lv_obj_get_scroll_bottom(terminalScroll_),LV_ANIM_OFF);
    }
    if(prompt!=terminalInputText_){lv_label_set_text(terminalPrompt_,prompt.c_str());terminalInputText_=prompt;}
}
void View::operationText(const std::string &state, const std::string &details) {
    const auto translated=pt(state);
    if(state_ && translated!=stateText_){lv_label_set_text(state_,translated.c_str());stateText_=translated;}
    if(details_ && details!=detailsText_){lv_label_set_text(details_,details.c_str());detailsText_=details;}
}
void View::splash(const Palette &palette) {
    palette_=palette;shell("Preparando suas ferramentas");
    lv_obj_add_flag(dock_,LV_OBJ_FLAG_HIDDEN);
    const int w=lv_display_get_horizontal_resolution(lv_obj_get_display(root_));
    const int h=lv_display_get_vertical_resolution(lv_obj_get_display(root_));
    lv_obj_set_size(body_,w,h-88);lv_obj_remove_flag(body_,LV_OBJ_FLAG_SCROLLABLE);
    lv_image_header_t info;
    if(lv_image_decoder_get_info("T:/boot/shark.png",&info)==LV_RESULT_OK) {
        auto *image=lv_image_create(body_);lv_image_set_src(image,"T:/boot/shark.png");
        lv_obj_set_size(image,440,220);lv_image_set_inner_align(image,LV_IMAGE_ALIGN_CONTAIN);
        lv_obj_align(image,LV_ALIGN_CENTER,0,-36);
        lv_obj_update_layout(root_);
        int target=lv_obj_get_x(image);
        lv_anim_t animation;lv_anim_init(&animation);lv_anim_set_var(&animation,image);
        lv_anim_set_values(&animation,-480,target);lv_anim_set_duration(&animation,560);
        lv_anim_set_path_cb(&animation,lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&animation,[](void *obj,int32_t x){lv_obj_set_x(static_cast<lv_obj_t *>(obj),x);});
        lv_anim_start(&animation);
    }
    progress_=lv_bar_create(body_);lv_obj_set_size(progress_,LV_PCT(64),8);
    lv_obj_align(progress_,LV_ALIGN_BOTTOM_MID,0,-40);
    lv_obj_set_style_bg_color(progress_,lv_color_hex(palette_.border),LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_,lv_color_hex(palette_.accent),LV_PART_INDICATOR);
    lv_obj_update_layout(root_);
}
void View::splashProgress(int percent) {if(progress_)lv_bar_set_value(progress_,percent,LV_ANIM_OFF);}
void View::status(const std::string &text) {
    if (status_ && text != lv_label_get_text(status_)) lv_label_set_text(status_, text.c_str());
}
void View::type(const std::string &text) { if (textarea_) lv_textarea_add_text(textarea_, text.c_str()); }
void View::backspace() { if (textarea_) lv_textarea_delete_char(textarea_); }
std::string View::text() const { return textarea_ ? lv_textarea_get_text(textarea_) : ""; }
int View::takeEvent() { int event = event_; event_ = None; return event; }
}
#endif

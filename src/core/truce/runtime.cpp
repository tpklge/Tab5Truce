#ifdef TRUCE_LVGL
#include "runtime.h"
#include "ui_view.h"
#include "pt_br.h"
#include <truce_hooks.h>
#include <truce_input_latch.h>
#include <truce_choice.h>
#include "core/display.h"
#include "core/main_menu.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/settings.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <atomic>
#include <esp_heap_caps.h>
#include <memory>
#include <map>
#include <truce_transaction.h>
namespace truce {
static std::atomic_bool running{false};
static lv_display_t *display;
static lv_indev_t *pointerInput;
static View view;
static Palette palette;
static String themeFile = "/themes/light.json";
static JsonDocument themeDoc;
static bool loaded = false;
static TaskHandle_t uiOwner;
static std::atomic_bool operationMode{false};
static bool dataPanelMode=false;
static bool documentMode=false;
enum class GraphicKind {None,Spectrum,Dial,Media,Game,Panel,Terminal};
static std::string terminalOutput,terminalInput;
static GraphicKind graphicKind=GraphicKind::None;
static int mediaAction=-1;
static bool uiBackEvent=false;
static portMUX_TYPE noticeMux=portMUX_INITIALIZER_UNLOCKED;
static char pendingNotice[256]={0};
static bool noticeReady=false;
static void queueNotice(const String &text) {
    size_t n=std::min<size_t>(text.length(),sizeof(pendingNotice)-1);
    while(n>0 && (static_cast<unsigned char>(text.c_str()[n])&0xC0)==0x80)--n;
    portENTER_CRITICAL(&noticeMux);memcpy(pendingNotice,text.c_str(),n);pendingNotice[n]=0;noticeReady=true;portEXIT_CRITICAL(&noticeMux);
}
static void drainNotice() {
    char message[256];bool ready;
    portENTER_CRITICAL(&noticeMux);ready=noticeReady;if(ready){memcpy(message,pendingNotice,sizeof(message));noticeReady=false;}portEXIT_CRITICAL(&noticeMux);
    if(ready)view.status(pt(message));
}
static uint32_t graphicTraceAt=0;
static bool spectrumHistory=true;
static std::string dataPanelState;
static String panelPrintLine;
static int panelPrintY=-1,panelPrintX=16;
static bool pumping=false;
static std::atomic_bool collectingRow{false};
static String rowText;
static String operationName;
static std::map<int,std::string> operationLines;
static uint32_t operationAt=0;
static bool operationDirty=false;
static std::atomic<uint32_t> flushCount{0}, flushMicros{0}, flushMaxMicros{0};
bool active() { return running.load(); }
static void flush(lv_display_t *d, const lv_area_t *area, uint8_t *pixels) {
    const uint32_t start=micros();
    // uint16_t* defaults to byte-swapped input in M5GFX. The typed overload is
    // both color-correct and a native copy into the P4's RGB565 framebuffer.
    M5.Display.pushImage(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area),
                         reinterpret_cast<const lgfx::rgb565_t *>(pixels));
    const uint32_t elapsed=micros()-start;
    flushCount.fetch_add(1);flushMicros.fetch_add(elapsed);
    if(elapsed>flushMaxMicros.load())flushMaxMicros.store(elapsed);
    lv_display_flush_ready(d);
}
static void readPointer(lv_indev_t *, lv_indev_data_t *data) {
    static lv_point_t last={0,0};
    static bool wasPressed=false;
    inputLock();
    const bool pressed=validTouch(1,touchPoint.pressed,touchPoint.x,touchPoint.y,tftWidth,tftHeight);
    if(pressed){last.x=touchPoint.x;last.y=touchPoint.y;}
    inputUnlock();
    data->point=last; // Keep the release at the last real point, never at Clear() coordinates.
    data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;
    if(pressed!=wasPressed)Serial.printf("[UI INPUT] touch=%d x=%ld y=%ld\n",pressed,long(last.x),long(last.y));
    wasPressed=pressed;
}
static void *fsOpen(lv_fs_drv_t *, const char *path, lv_fs_mode_t mode) {
    if (mode != LV_FS_MODE_RD || !sdcardMounted) return nullptr;
    String p = path;
    if (!p.startsWith("/")) p = "/" + p;
    File file = SD.open(p, FILE_READ);
    if (!file || file.isDirectory() || file.size() > 512 * 1024) return nullptr;
    return new File(file);
}
static lv_fs_res_t fsClose(lv_fs_drv_t *, void *file) { delete static_cast<File *>(file); return LV_FS_RES_OK; }
static lv_fs_res_t fsRead(lv_fs_drv_t *, void *file, void *buf, uint32_t len, uint32_t *got) {
    *got = static_cast<File *>(file)->read(static_cast<uint8_t *>(buf), len);
    return LV_FS_RES_OK;
}
static lv_fs_res_t fsSeek(lv_fs_drv_t *, void *file, uint32_t pos, lv_fs_whence_t whence) {
    SeekMode mode = whence == LV_FS_SEEK_SET ? SeekSet : whence == LV_FS_SEEK_CUR ? SeekCur : SeekEnd;
    return static_cast<File *>(file)->seek(pos, mode) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}
static lv_fs_res_t fsTell(lv_fs_drv_t *, void *file, uint32_t *pos) {
    *pos = static_cast<File *>(file)->position(); return LV_FS_RES_OK;
}
static void init() {
    if (display) {
        if (lv_display_get_horizontal_resolution(display) != tft.width() ||
            lv_display_get_vertical_resolution(display) != tft.height())
            lv_display_set_resolution(display, tft.width(), tft.height());
        return;
    }
    uiOwner=xTaskGetCurrentTaskHandle();
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });
    display = lv_display_create(tft.width(), tft.height());
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    const size_t size = 1280 * 80 * 2;
    void *buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) { Serial.println("TRUCE: LVGL buffer allocation failed"); abort(); }
    lv_display_set_buffers(display, buf, nullptr, size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    auto *pointer = lv_indev_create();
    pointerInput = pointer;
    lv_indev_set_type(pointer, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(pointer, readPointer);
    lv_indev_set_display(pointer, display);
    static lv_fs_drv_t driver;
    lv_fs_drv_init(&driver);
    driver.letter = 'T'; driver.cache_size=16*1024; driver.open_cb = fsOpen; driver.close_cb = fsClose;
    driver.read_cb = fsRead; driver.seek_cb = fsSeek; driver.tell_cb = fsTell;
    lv_fs_drv_register(&driver);
    view.init(display);
}
static uint32_t color(JsonVariantConst value, uint32_t fallback) {
    const char *s = value.as<const char *>();
    if (!s) return fallback;
    if (*s == '#') ++s;
    if (strlen(s) != 6) return fallback;
    for (int i = 0; i < 6; ++i) if (!isxdigit((unsigned char)s[i])) return fallback;
    return strtoul(s, nullptr, 16);
}
static uint16_t rgb565(uint32_t c) { return ((c >> 8) & 0xF800) | ((c >> 5) & 0x7E0) | ((c >> 3) & 31); }
static void loadPalette() {
    palette = Palette();
    themeDoc.clear();
    File file = SD.open(themeFile);
    if (file && file.size() <= 32768 && !deserializeJson(themeDoc, file)) {
        palette.name = themeDoc["name"] | "Personalizado";
        palette.background = color(themeDoc["background"], palette.background);
        palette.panel = color(themeDoc["panel"], palette.panel);
        palette.text = color(themeDoc["text"], palette.text);
        palette.muted = color(themeDoc["muted"], palette.muted);
        palette.border = color(themeDoc["border"], palette.border);
        palette.accent = color(themeDoc["accent"], palette.accent);
    }
    uint16_t secondary = rgb565(palette.muted), background = rgb565(palette.background);
    bruceConfig._setUiColor(rgb565(palette.accent), &secondary, &background);
}
static void loadState() {
    if (loaded) return;
    loaded = true;
    JsonDocument config;
    truce::recover(SD, String("/config/ui.json"));
    File file = SD.open("/config/ui.json");
    if (file && file.size() < 4096 && !deserializeJson(config, file)) {
        String path = config["theme"] | "/themes/light.json";
        if (path.startsWith("/themes/") && path.endsWith(".json")) themeFile = path;
    }
    loadPalette();
}
static void saveState() {
    JsonDocument config;
    config["theme"] = themeFile;
    File file = SD.open("/config/ui.json.tmp", FILE_WRITE);
    if (!file) return;
    size_t wrote = serializeJson(config, file);
    file.flush(); file.close();
    if (!wrote) return;
    truce::commit(SD, String("/config/ui.json.tmp"), String("/config/ui.json"));
}
class Session {
    bool previousZone;
public:
    Session() {
        endOperation();
        init(); loadState();
        previousZone = touchZoneOutsideFooterEnabled;
        touchZoneOutsideFooterEnabled = false;
        inputLock();
        // Keep the real touch state: wait_release must see a held finger,
        // not a synthetic release produced by clearing the shared snapshot.
        SelPress = EscPress = PrevPress = NextPress = UpPress = DownPress = false;
        AnyKeyPress=false;SerialCmdPress=false;NextPagePress=false;PrevPagePress=false;
        inputUnlock();
        lv_indev_reset(pointerInput, nullptr);
        lv_indev_wait_release(pointerInput);
        running.store(true);
    }
    ~Session() {
        view.clear();
        running.store(false);
        touchZoneOutsideFooterEnabled = previousZone;
        inputLock(); KeyStroke.Clear();
        SelPress = false; EscPress = false; PrevPress = false;
        NextPress = false; UpPress = false; DownPress = false;
        AnyKeyPress=false;SerialCmdPress=false;NextPagePress=false;PrevPagePress=false;
        inputUnlock();
    }
};
static void tick() {
    drainNotice();
    lv_timer_handler();
    delay(5);
}
static void updateStatus() {
#if defined(HAS_RTC)
    updateTimeStr(_rtc.getTimeStruct());
#else
    updateTimeStr(rtc.getTimeStruct());
#endif
    String text = String(timeStr) + "   " + (WiFi.isConnected() ? "Wi-Fi   " : "") +
                  String(getBattery()) + "%   SD";
    view.status(text.c_str());
}
void requireStorage() {
    // Called before the input task is created; LVGL still supplies the complete recovery screen.
    init();
    running.store(true);
    view.message("microSD necessario", "Insira um microSD FAT32.\nTodos os dados ficam em /truce.\nA flash nao e usada para salvar arquivos.", palette, "Tentar novamente");
    while (!setupSdCard()) {
        M5.update();
        auto detail = M5.Touch.getDetail();
        touchPoint.x = detail.x; touchPoint.y = detail.y; touchPoint.pressed = detail.isPressed();
        tick();
        if (view.takeEvent() == View::Submit) { delay(50); }
        else delay(100);
    }
    SD.mkdir("/config"); SD.mkdir("/themes"); SD.mkdir("/preferences"); SD.mkdir("/logs"); SD.mkdir("/apps_data");
    view.clear(); running.store(false);
}
void chooseTheme() {
    Session session;
    std::vector<Item> items;
    std::vector<String> paths;
    File dir = SD.open("/themes");
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        String path = f.path();
        if (f.isDirectory() || !path.endsWith(".json") || f.size() > 32768) continue;
        JsonDocument doc;
        if (deserializeJson(doc, f)) continue;
        items.push_back({doc["name"] | f.name(), LV_SYMBOL_IMAGE, "", true, path == themeFile});
        paths.push_back(path);
    }
    if (items.empty()) {
        view.message("Temas", "Copie a pasta sdcard/truce para a raiz do microSD.\nOs temas ficam em /truce/themes.", palette);
    } else view.menu("Temas", items, false, palette);
    for (;;) {
        tick();
        if (check(EscPress)) return;
        if (check(PrevPress) || check(UpPress)) view.move(-1);
        if (check(NextPress) || check(DownPress)) view.move(1);
        if (check(SelPress)) view.activate();
        int event = view.takeEvent();
        if (event == View::Back || event == View::Home || event == View::Submit) return;
        if (event >= 0 && static_cast<size_t>(event) < paths.size()) {
            themeFile = paths[event]; loadPalette(); saveState(); return;
        }
    }
}
int menu(std::vector<Option> &options, uint8_t type, const char *title, int index, bool interpreter, const char *icon) {
    if (options.empty()) return -1;
    // Execute operations after all LVGL state/input ownership has been released.
    std::function<void()> operation;
    int chosen = -1;
    String toolTitle;
    {
        Session session;
        bool home = type == MENU_TYPE_MAIN;
        std::vector<Item> items;
        std::vector<size_t> sourceIndices;
        auto buildItems = [&]() {
          items.clear(); sourceIndices.clear();
          for (size_t source = 0; source < options.size(); ++source) {
            auto &opt = options[source];
            if (opt.label == "LittleFS" || opt.label == "Little FS" || opt.label == "Copy->LittleFS" ||
                opt.label == "Copy->SD") continue;
            sourceIndices.push_back(source);
            std::string image;
            const char *custom = themeDoc["icons"][opt.label].as<const char *>();
            if (custom) {
                String path = custom;
                if (!path.startsWith("/")) path = themeFile.substring(0, themeFile.lastIndexOf('/') + 1) + path;
                image = std::string("T:") + path.c_str();
            } else if (home && opt.hoverPointer) {
                auto *item = static_cast<MenuItemInterface *>(opt.hoverPointer);
                if (item->checkTheme()) image = std::string("T:") + item->themePath().c_str();
            }
            items.push_back({opt.label.c_str(), icon?iconFor(icon):"", image, opt.enabled, opt.selected});
          }
        };
        bool initialRender = true;
        auto render = [&]() {
            buildItems();
            if (initialRender) {
                int visibleIndex = 0;
                for (size_t i = 0; i < sourceIndices.size(); ++i)
                    if (sourceIndices[i] == static_cast<size_t>(std::max(0, index))) visibleIndex = i;
                index = visibleIndex;
                initialRender = false;
            }
            menuOptionType = type;
            menuOptionLabel = title;
            view.menu(home ? "Ferramentas" : title, items, home, palette);
            view.focus(index); updateStatus();
        };
        render();
        uint32_t statusAt = millis();
        for (;;) {
            tick();
            if (millis() - statusAt > 1000) { updateStatus(); statusAt = millis(); }
            if (check(EscPress) && !home) break;
            if (check(PrevPress) || check(UpPress)) view.move(-1);
            if (check(NextPress) || check(DownPress)) view.move(1);
            inputLock();const bool serialKey=SerialCmdPress;inputUnlock();
            const bool keySelect=check(SelPress);
            if (keySelect) view.activate();
            int event = view.takeEvent();
            const int originalEvent=event;
            index = view.focused();
            const int requestedOption = forceMenuOption;
            if (requestedOption >= 0) {
                event = View::None;
                for (size_t i = 0; i < sourceIndices.size(); ++i)
                    if (sourceIndices[i] == static_cast<size_t>(requestedOption)) event = i;
                forceMenuOption = -1;
            }
            if (event == View::Back) { chosen = -1; break; }
            if (event == View::Home) {
                if (home) { view.focus(0); continue; }
                returnToMenu = true; chosen = -1; break;
            }
            if (event == View::Themes) {
                // End this LVGL session before entering another one, then redraw it.
                view.clear(); running.store(false);
                chooseTheme(); running.store(true);
                render(); continue;
            }
            if (event == View::Files || event == View::Settings) {
                if (!home) continue;
                const char *name = event == View::Files ? "Files" : "Config";
                for (size_t i = 0; i < sourceIndices.size(); ++i)
                    if (options[sourceIndices[i]].label == name) event = i;
            }
            if (event >= 0 && static_cast<size_t>(event) < sourceIndices.size() &&
                options[sourceIndices[event]].enabled) {
                chosen = sourceIndices[event];
                for (size_t i = 0; i < options.size(); ++i) options[i].hovered = static_cast<int>(i) == chosen;
                Serial.printf("[UI] selected=%d label=%s origin=%s event=%d\n",chosen,options[chosen].label.c_str(),
                    requestedOption>=0?"serial":keySelect?(serialKey?"serial-key":"keyboard"):"touch",originalEvent);
                operation = options[chosen].operation;
                toolTitle=options[chosen].label;
                break;
            }
            if (returnToMenu || (interpreter_state > 0 && !interpreter)) break;
        }
    }
    if (operation) {
        beginDataPanel(toolTitle);
        operationControls("Anterior","Próximo","Selecionar","Voltar");
        operation();
    }
    endOperation();
    return chosen;
}
void bootIntro() {
    Session session;
    view.splash(palette);
    const uint32_t start=millis();
    while(millis()-start<640) {
        view.splashProgress(std::min<uint32_t>(100,(millis()-start)*100/640));
        tick();
    }
}
bool captureDisplay() { return running.load() || collectingRow; }
void beginOperation(const String &title, bool stopOnly) {
    init();loadState();
    if(xTaskGetCurrentTaskHandle()!=uiOwner)return;
    if(operationMode && operationName==title && !documentMode && graphicKind==GraphicKind::None)return;
    dataPanelMode=false;documentMode=false;graphicKind=GraphicKind::None;mediaAction=-1;uiBackEvent=false;
    operationMode=true;running.store(true);touchZoneOutsideFooterEnabled=false;
    operationName=title;operationLines.clear();operationDirty=false;operationAt=0;
    view.operation(pt(title.c_str()),stopOnly,palette);
}
void endOperation() {
    if(uiOwner && xTaskGetCurrentTaskHandle()!=uiOwner)return;
    if(!operationMode)return;
    operationMode=false;dataPanelMode=false;documentMode=false;graphicKind=GraphicKind::None;mediaAction=-1;uiBackEvent=false;
    std::string().swap(terminalOutput);std::string().swap(terminalInput);operationName="";operationLines.clear();
    running.store(false);touchZoneOutsideFooterEnabled=true;
    view.clear();
}
bool drawDocument(const String &title,const std::vector<String> &lines,size_t first,size_t count) {
    if(uiOwner && xTaskGetCurrentTaskHandle()!=uiOwner)return false;
    const String heading=title.isEmpty() ? (operationName.isEmpty()?String("Detalhes"):operationName) : title;
    first=std::min(first,lines.size());
    const size_t last=first+std::min(count,lines.size()-first);
    std::string text;
    for(size_t i=first;i<last;++i){if(i>first)text+="\n";text+=lines[i].c_str();}
    const std::string page="Linhas "+std::to_string(lines.empty()?0:first+1)+"–"+
        std::to_string(last)+" de "+std::to_string(lines.size());
    if(!documentMode || operationName!=heading) {
        endOperation();beginOperation(heading);documentMode=true;
        view.document(pt(heading.c_str()),text,page,palette);
    } else view.operationText(page,text);
    view.operationControls(first?"Subir":"",last<lines.size()?"Descer":"","Continuar");
    return true;
}
void readDocument(const String &title,const std::vector<String> &lines) {
    Session session;
    constexpr size_t pageSize=16;
    const size_t pages=std::max<size_t>(1,(lines.size()+pageSize-1)/pageSize);
    size_t page=0;
    auto render=[&]() {
        std::string text;
        for(size_t i=page*pageSize;i<std::min(lines.size(),(page+1)*pageSize);++i) {
            if(i>page*pageSize)text+="\n";
            text+=lines[i].c_str();
        }
        view.document(title.isEmpty()?"Detalhes":pt(title.c_str()),text,
            "Página "+std::to_string(page+1)+" de "+std::to_string(pages),palette);
        view.operationControls(page?"Página anterior":"",page+1<pages?"Próxima página":"","Continuar");
    };
    render();
    for(;;) {
        tick();
        int event=view.takeEvent();
        if(check(EscPress) || check(SelPress) || event==View::Back || event==View::Submit)return;
        if((check(PrevPress) || check(UpPress) || event==View::Previous) && page){--page;render();}
        if((check(NextPress) || check(DownPress) || event==View::Next) && page+1<pages){++page;render();}
    }
}
void beginDataPanel(const String &title) {
    if(uiOwner && xTaskGetCurrentTaskHandle()!=uiOwner)return;
    const bool same=operationMode && dataPanelMode && operationName==title;
    if(!same) {
        if(documentMode)endOperation();
        beginOperation(title);
        dataPanelMode=true;documentMode=false;dataPanelState="Aguardando dados";
        view.dataPanel(pt(title.c_str()),palette);
    }
    // A legacy banner marks a new sample. Keep widgets/cache and drop stale rows.
    operationLines.clear();operationDirty=true;panelPrintLine="";panelPrintY=-1;
    tft.setCursor(16,96);
}
void operationControls(const char *previous,const char *next,const char *action,const char *back) {
    if(operationMode && xTaskGetCurrentTaskHandle()==uiOwner)
        view.operationControls(previous,next,action,back);
}
bool operationSubtitle(const String &text) {
    if(uiOwner && xTaskGetCurrentTaskHandle()!=uiOwner)return false;
    if(!operationMode || !dataPanelMode)return false;
    dataPanelState=pt(text.c_str());operationDirty=true;return true;
}
bool operationBorder(bool clear) {
    if(uiOwner && xTaskGetCurrentTaskHandle()!=uiOwner)return captureDisplay();
    if(operationMode && (graphicKind!=GraphicKind::None || documentMode))return true;
    const String title=operationName.isEmpty()?String(menuOptionLabel):operationName;
    if(!operationMode || clear)beginDataPanel(title.isEmpty()?String("Ferramenta"):title);
    return true;
}
bool operationTitle(const String &title) {
    if(uiOwner && xTaskGetCurrentTaskHandle()!=uiOwner)return false;
    const char *panels[]={"GPS Tracker","Wardriving","Wigle Upload","WDG Upload",
        "TAG-O-MATIC","RFID 125kHz","SRIX TOOL","AMIIBOLINK","PN532 BLE",
        "CHAMELEON","Read EMV Card","PN532 UART","PN532Killer","IR Read","FILE INFO","I2C Finder",
        "WiFi Connect","EVIL PORTAL","RESPONDER","pcap sniffer","Handshake Capture",
        "WiFi: Beacon SPAM","WiFi Password Recover","WiFi Password Cracker",
        "NetCut Scan","Scanning Clients","Device Info","LISTEN TCP","TCP CLIENT",
        "SOCKS4 PROXY","WIREGUARD","BLE SNIFFER","CAPTURED PACKETS","PACKET DETAILS",
        "BLE Suite","BLE Send","BLE Keyboard","USB Keyboard","BAD USB/BLE",
        "Select AP","Select Channel","Select Target","SELECT TEMPLATE","KARMA SETUP",
        "LOAD FROM","BROWSE SD","SELECTED","ENHANCED KARMA ATK","MODERN KARMA ATTACK",
        "ADVANCED STATS","SSID DATABASE","BROADCAST STATS","KARMA STATS"};
    for(const char *name:panels)if(title==name){
        beginDataPanel(title);
        if(title=="GPS Tracker" || title=="Wardriving")operationControls("","","","Parar");
        else if(title=="FILE INFO" || title=="I2C Finder")operationControls("","","Continuar");
        else if(title=="PN532 UART" || title=="PN532Killer")operationControls("Anterior","Próximo","Opções");
        else if(title=="BLE SNIFFER" || title=="CAPTURED PACKETS")operationControls("Anterior","Próximo","Selecionar");
        else if(title=="WiFi Connect" || title=="SOCKS4 PROXY" || title=="WIREGUARD")operationControls("","","","Voltar");
        else if(title=="WiFi: Beacon SPAM")operationControls("","","Pausar","Parar");
        else if(title=="RESPONDER")operationControls("","","","Parar");
        else if(title=="pcap sniffer")operationControls("Canal anterior","Próximo canal","Menu","Parar");
        else if(title=="TCP CLIENT" || title=="LISTEN TCP")operationControls("","","Escrever","Sair");
        else if(title=="EVIL PORTAL")operationControls("","","Menu","Parar");
        return true;
    }
    const char *names[]={"iBeacon","Deauth Flood","Station Deauth","Deauth All","Deauth List",
                         "Beacon Attack","Beacon Spam","DHCP Starvation","ARP Poisoning",
                         "ARP Spoofing","MAC Flooding","NETCUT ACTIVE","NetCut ARP"};
    for(const char *name:names)if(title==name){beginOperation(title,title=="iBeacon");return true;}
    // Every titled tool uses native widgets, including dynamic/plugin titles.
    beginDataPanel(title.isEmpty()?String("Ferramenta"):title);
    operationControls("Anterior","Próximo","Selecionar","Voltar");
    return true;
}
String collectRow(const std::function<void()> &draw) {
    init();endOperation();rowText="";
    collectingRow.store(true);draw();collectingRow.store(false);
    rowText.trim();
    int prefix=0;
    while(prefix<(int)rowText.length() && isdigit((unsigned char)rowText[prefix]))++prefix;
    if(prefix>0 && prefix<(int)rowText.length() && rowText[prefix]=='.')rowText.remove(0,prefix+1);
    rowText.trim();return rowText;
}
bool captureText(const String &text, int x, int y) {
    if(collectingRow) {
        if(xTaskGetCurrentTaskHandle()==uiOwner && rowText.length()<240){
            if(rowText.length())rowText+=rowText.indexOf('\n')<0?"\n":" • ";
            rowText+=text.substring(0,240-rowText.length());
        }
        return true;
    }
    if(!operationMode || xTaskGetCurrentTaskHandle()!=uiOwner)return running.load();
    if(graphicKind!=GraphicKind::None)return true;
    if(y<0 || text.isEmpty())return true;
    String line=text;line.trim();
    if(line.isEmpty() || line.indexOf("ESC=")>=0 || line.startsWith("Press ") ||
       line.startsWith("---"))return true;
    const int key=(y/8)*2048+std::max(0,std::min(x,2047));
    std::string translated=pt(line.c_str());
    if(translated.size()>240)translated.resize(240);
    if(operationLines[key]!=translated) {
        operationLines[key]=translated;operationDirty=true;
        while(operationLines.size()>48)operationLines.erase(operationLines.begin());
    }
    return true;
}
bool capturePrint(const String &text) {
    if(!operationMode && !collectingRow)return running.load();
    if(xTaskGetCurrentTaskHandle()!=uiOwner)return true;
    const int x=tft.getCursorX(),y=tft.getCursorY();
    if(dataPanelMode) {
        int cursorY=y;
        if(panelPrintY!=y){panelPrintLine="";panelPrintY=y;panelPrintX=x;}
        for(unsigned i=0;i<text.length();++i) {
            const char c=text[i];
            if(c=='\r')continue;
            if(c=='\n') {
                captureText(panelPrintLine,panelPrintX,cursorY);
                panelPrintLine="";cursorY+=32;panelPrintY=cursorY;panelPrintX=16;
            } else if(panelPrintLine.length()<240)panelPrintLine+=c;
        }
        if(panelPrintLine.length())captureText(panelPrintLine,panelPrintX,cursorY);
        tft.setCursor(panelPrintX+panelPrintLine.length()*12,cursorY);
        return true;
    }
    captureText(text,x,y);
    if(text.indexOf('\n')>=0)tft.setCursor(16,y+32);
    else tft.setCursor(x+std::min(240,(int)text.length()*12),y);
    return true;
}
void pumpOperation() {
    if(!operationMode || pumping || xTaskGetCurrentTaskHandle()!=uiOwner || millis()-operationAt<32)return;
    pumping=true;operationAt=millis();drainNotice();
    if(operationDirty && graphicKind==GraphicKind::None) {
        std::string details;
        for(const auto &line:operationLines){if(!details.empty())details+="\n";details+=line.second;}
        const std::string state=operationLines.empty()?"Em execução":operationLines.rbegin()->second;
        if(dataPanelMode) {
            std::vector<std::string> lines;
            for(const auto &line:operationLines)lines.push_back(line.second);
            std::string state=dataPanelState;
            if(state=="Aguardando dados" && !lines.empty())
                state=lines.back().find(':')==std::string::npos && lines.back().size()<80 ? lines.back() : "Dados disponíveis";
            view.dataText(state,lines);
        } else view.operationText(state,details);
        operationDirty=false;
    }
    lv_timer_handler();
    const int event=view.takeEvent();
    if(event!=View::None) {
        inputLock();
        AnyKeyPress=true;
        if(event==View::Back){EscPress=true;uiBackEvent=true;}
        if(event==View::Previous) {
            if(graphicKind==GraphicKind::Terminal)view.scrollTerminal(-1);
            else if(documentMode && operationName=="PN532 BLE")PrevPagePress=true;
            else if(graphicKind==GraphicKind::Spectrum && (operationName=="Channel Analyzer" || operationName=="Jam Detect"))UpPress=true;
            else PrevPress=true;
        }
        if(event==View::Next) {
            if(graphicKind==GraphicKind::Terminal)view.scrollTerminal(1);
            else if(documentMode && operationName=="PN532 BLE")NextPagePress=true;
            else if(graphicKind==GraphicKind::Spectrum && (operationName=="Channel Analyzer" || operationName=="Jam Detect"))DownPress=true;
            else NextPress=true;
        }
        if(event==View::Submit)SelPress=true;
        if(graphicKind==GraphicKind::Media && event>=100 && event<104) {
            mediaAction=event-100;SelPress=true;
        }
        inputUnlock();
    }
    pumping=false;
}
bool notice(const String &text, const char *title) {
    if(uiOwner && xTaskGetCurrentTaskHandle()!=uiOwner){queueNotice(text);return true;}
    Session session;
    view.message(title,pt(text.c_str()),palette);
    for(;;){
        tick();
        if(check(AnyKeyPress) || check(EscPress) || check(SelPress))return true;
        const int event=view.takeEvent();if(event==View::Submit || event==View::Back)return true;
    }
}
static bool graphicOwner() {return !uiOwner || xTaskGetCurrentTaskHandle()==uiOwner;}
static bool prepareGraphic(GraphicKind kind,const String &title) {
    if(!graphicOwner())return false;
    if(graphicKind==kind && operationName==title)return false;
    endOperation();beginOperation(title);graphicKind=kind;operationDirty=false;
    return true;
}
bool beginSpectrum(const String &title,bool sdr,bool history) {
    if(!graphicOwner())return false;
    prepareGraphic(GraphicKind::Spectrum,title);view.spectrum(pt(title.c_str()),sdr,palette,history);
    spectrumHistory=history;
    if(title=="RF Waterfall")view.operationControls("Faixa -","Faixa +","","Parar");
    else if(title=="Channel Analyzer")view.operationControls("Tempo +","Tempo -","","Parar");
    else if(title=="Jam Detect")view.operationControls("Limiar +","Limiar -","","Parar");
    else view.operationControls("","","","Parar");
    graphicTraceAt=0;return true;
}
void spectrumTrace(const uint8_t *live,const uint8_t *peak,int left,int right,bool alert) {
    if(!graphicOwner() || graphicKind!=GraphicKind::Spectrum || (spectrumHistory && millis()-graphicTraceAt<32))return;
    graphicTraceAt=millis();view.spectrumTrace(live,peak,left,right,alert);
}
void logicFrame(const std::vector<std::vector<uint8_t>> &signals,int selected,const String &state) {
    if(!graphicOwner())return;
    if(prepareGraphic(GraphicKind::Spectrum,"Analisador lógico"))view.spectrum("Analisador lógico",false,palette,false);
    view.logicSignals(signals,selected);view.operationText(state.c_str(),"");
    view.operationControls("Esquerda","Direita","Zoom / canal","Sair");
}
void spectrumRow(const uint8_t *values) {
    if(graphicOwner() && graphicKind==GraphicKind::Spectrum)view.spectrumRow(values);
}
void spectrumRuler(const int *columns,const String *labels,int count,int selected) {
    if(!graphicOwner() || graphicKind!=GraphicKind::Spectrum || !columns || !labels)return;
    std::vector<int> positions;std::vector<std::string> captions;
    for(int i=0;i<std::min(count,16);++i){positions.push_back(columns[i]);captions.emplace_back(labels[i].c_str());}
    view.spectrumRuler(positions,captions,selected);
}
void graphicStatus(const String &text) {
    if(graphicOwner() && graphicKind!=GraphicKind::None)view.operationText(pt(text.c_str()),"");
}
void dialFrame(const String &title,const String &value,const String &description,int progress) {
    if(!graphicOwner())return;
    if(prepareGraphic(GraphicKind::Dial,title))view.dial(pt(title.c_str()),palette);
    view.dialValue(value.c_str(),description.c_str(),progress);
}
void mediaFrame(const String &file,const String &state,const String &elapsed,const String &total,
                int progress,int volume,bool loop,int selected) {
    if(!graphicOwner())return;
    if(prepareGraphic(GraphicKind::Media,"Reprodutor de áudio"))view.media(palette);
    view.mediaValue(file.c_str(),state.c_str(),elapsed.c_str(),total.c_str(),progress,volume,loop,selected);
}
int takeMediaAction() {pumpOperation();const int action=mediaAction;mediaAction=-1;return action;}
int selectValue(const String &title,int value,int minimum,int maximum,const std::function<void(int)> &preview) {
    Session session;view.slider(pt(title.c_str()),value,minimum,maximum,palette);int previous=value;
    for(;;){
        tick();
        if(check(PrevPress) || check(DownPress))view.adjustSlider(-5);
        if(check(NextPress) || check(UpPress))view.adjustSlider(5);
        const int current=view.sliderValue();if(current!=previous){preview(current);previous=current;}
        const int event=view.takeEvent();
        if(event==View::Back || check(EscPress))return -1;
        if(event==View::Submit || check(SelPress))return current;
    }
}
void gameFrame(int sharkX,int sharkY,const std::vector<std::pair<int,int>> &fish,int score,bool paused) {
    if(!graphicOwner())return;
    if(prepareGraphic(GraphicKind::Game,"Megalodon"))view.game(palette);
    view.gameFrame(sharkX,sharkY,fish,score,paused);
}
void panelFrame(const String &title,const String &state,const std::vector<String> &fields) {
    if(!graphicOwner())return;
    if(prepareGraphic(GraphicKind::Panel,title))view.dataPanel(pt(title.c_str()),palette);
    std::vector<std::string> lines;for(const auto &field:fields)lines.emplace_back(field.c_str());
    view.dataText(pt(state.c_str()),lines);
}
bool inspect(const String &title,const String &state,const std::vector<String> &fields,const char *action) {
    if(!graphicOwner())return false;
    Session session;view.dataPanel(pt(title.c_str()),palette);
    std::vector<std::string> lines;for(const auto &field:fields)lines.emplace_back(field.c_str());
    view.dataText(pt(state.c_str()),lines);view.operationControls("","",action,"Voltar");
    for(;;){tick();const int event=view.takeEvent();
        if(check(EscPress) || event==View::Back)return false;
        if(check(SelPress) || event==View::Submit)return true;
    }
}
int prompt(const String &title,const String &message,const std::vector<String> &actions,uint32_t automaticMs,bool cycleKeys) {
    if(!graphicOwner())return -1;
    Session session;std::vector<std::string> captions;for(const auto &a:actions)captions.emplace_back(a.c_str());
    view.choice(pt(title.c_str()),message.c_str(),captions,palette,automaticMs>0);
    ChoiceState choice(actions.size(),cycleKeys);
    if(cycleKeys && !actions.empty()){view.operationControls("Anterior","Próximo","Selecionar","");view.focus(0);}
    const uint32_t start=millis();
    for(;;){tick();const int event=view.takeEvent();
        if(automaticMs){if(millis()-start>=automaticMs)return 0;continue;}
        if(event>=0 && size_t(event)<actions.size())return event;
        if(actions.empty() && (check(AnyKeyPress) || check(SelPress) || check(NextPress) || check(PrevPress) || event==View::Submit || event==View::Back))return 0;
        int result=ChoiceState::Pending;
        if(cycleKeys && !actions.empty()) {
            const int before=choice.selected();
            if(check(PrevPress) || check(EscPress) || event==View::Previous || event==View::Back)choice.apply(ChoiceState::Previous);
            if(check(NextPress) || event==View::Next)choice.apply(ChoiceState::Next);
            if(check(SelPress) || event==View::Submit)result=choice.apply(ChoiceState::Accept);
            if(before!=choice.selected())view.focus(choice.selected());
        } else {
            if(check(EscPress) || event==View::Back)result=choice.apply(ChoiceState::Cancel);
            else if(check(SelPress) || event==View::Submit)result=choice.apply(ChoiceState::Accept);
            else if(check(NextPress) || event==View::Next)result=choice.apply(ChoiceState::Next);
            else if(check(PrevPress) || event==View::Previous)result=choice.apply(ChoiceState::Previous);
        }
        if(result!=ChoiceState::Pending)return result;
    }
}

bool statusNotice(const String &text) {
    if(!uiOwner || xTaskGetCurrentTaskHandle()!=uiOwner){queueNotice(text);return true;}
    if(operationMode) {
        if(dataPanelMode){dataPanelState=pt(text.c_str());operationDirty=true;}
        else view.operationText(pt(text.c_str()),"");
    } else {panelFrame("Estado",text,{});view.operationControls("","","","");}
    if(text=="Pairing..." || text=="Waiting Victim")view.operationControls("","","","Cancelar");
    lv_timer_handler();return true;
}
bool takeBackEvent() {
    if(!graphicOwner())return false;
    pumpOperation();const bool result=uiBackEvent;uiBackEvent=false;return result;
}
void terminalWindow(const std::vector<String> &lines,const String &prompt) {
    if(!graphicOwner())return;
    if(graphicKind!=GraphicKind::Terminal)terminalReset("Terminal UART");
    size_t start=lines.size(),bytes=0;
    while(start && bytes+lines[start-1].length()+1<=4096){--start;bytes+=lines[start].length()+1;}
    std::string output;output.reserve(bytes);
    for(size_t i=start;i<lines.size();++i){output+=lines[i].c_str();output+='\n';}
    view.terminalText(output,prompt.c_str());
    view.operationControls("Rolar acima","Rolar abaixo","","Sair");
}
void terminalReset(const String &title,const String &context) {
    if(!graphicOwner())return;
    if(prepareGraphic(GraphicKind::Terminal,title))view.terminal(title.c_str(),palette);
    terminalOutput=context.c_str();terminalInput.clear();view.operationText("Terminal conectado","");view.terminalText(terminalOutput,terminalInput);
}
void terminalAppend(const String &title,const String &text,bool newline) {
    if(!graphicOwner())return;
    if(graphicKind!=GraphicKind::Terminal)terminalReset(title);
    terminalOutput+=text.c_str();if(newline)terminalOutput+="\n";
    if(terminalOutput.size()>4096){size_t start=terminalOutput.find('\n',terminalOutput.size()-4096);if(start==std::string::npos){start=terminalOutput.size()-4096;while(start<terminalOutput.size() && (static_cast<unsigned char>(terminalOutput[start])&0xC0)==0x80)++start;}else ++start;terminalOutput.erase(0,start);}
    view.terminalText(terminalOutput,terminalInput);
}
void terminalPrompt(const String &text) {
    if(!graphicOwner() || graphicKind!=GraphicKind::Terminal)return;
    terminalInput=text.c_str();view.terminalText(terminalOutput,terminalInput);
}
void diagnostics() {
    Serial.printf("[TRUCE UI] flushes=%lu total_us=%lu max_us=%lu internal_free=%u internal_largest=%u psram_free=%u\n",
                  (unsigned long)flushCount.load(),(unsigned long)flushMicros.load(),(unsigned long)flushMaxMicros.load(),
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
void brightness() {
    Session session;
    const int original = bruceConfig.bright;
    view.slider("Brilho da tela", original, 1, 100, palette);
    int current = original;
    for (;;) {
        tick();
        if (check(PrevPress) || check(DownPress)) view.adjustSlider(-5);
        if (check(NextPress) || check(UpPress)) view.adjustSlider(5);
        if (view.sliderValue() != current) {
            current = view.sliderValue(); setBrightness(current, false);
        }
        int event = view.takeEvent();
        if (event == View::Submit || check(SelPress)) { setBrightness(current, true); return; }
        if (event == View::Back || check(EscPress)) { setBrightness(original, false); return; }
    }
}
String keyboard(const String &text, int limit, const String &title, bool password, int mode) {
    Session session;
    view.input(title.c_str(), text.c_str(), std::max(0, limit), password, mode, palette);
    for (;;) {
        tick();
        auto keys = _getKeyPress();
        if (keys.exit_key || check(EscPress)) return String((char)0x1B);
        if (keys.del) view.backspace();
        for (char c : keys.word) view.type(std::string(1, c));
        if (keys.enter || check(SelPress)) return String(view.text().c_str());
        int event = view.takeEvent();
        if (event == View::Back) return String((char)0x1B);
        if (event == View::Submit) return String(view.text().c_str());
    }
}
}
#endif

#include "ui_view.h"
#include <truce_path.h>
#include <truce_choice.h>
#include <truce_input_latch.h>
#include <truce_signal_view.h>
#include <truce_radio_policy.h>
#include "pt_br.h"
#include <truce_transaction.h>
#include <src/widgets/buttonmatrix/lv_buttonmatrix_private.h>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using namespace truce;
static int width, height, taps;
static size_t flushedPixels=0, fileOpens=0;
static void *(*originalOpen)(lv_fs_drv_t *,const char *,lv_fs_mode_t)=nullptr;
static void *countOpen(lv_fs_drv_t *driver,const char *path,lv_fs_mode_t mode) {
    ++fileOpens;return originalOpen(driver,path,mode);
}
static bool pressed;
static lv_point_t touch;
static std::vector<uint16_t> framebuffer;
static std::vector<std::string> capturedFrames;
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    flushedPixels+=lv_area_get_size(area);
    auto *src = reinterpret_cast<uint16_t *>(pixels);
    for (int y = area->y1; y <= area->y2; ++y)
        for (int x = area->x1; x <= area->x2; ++x)
            framebuffer[y * width + x] = *src++;
    lv_display_flush_ready(display);
}
static void advance(int ms = 60) {
    for (int i = 0; i < ms; i += 5) { lv_tick_inc(5); lv_timer_handler(); }
}
static void read(lv_indev_t *, lv_indev_data_t *data) {
    data->point = touch;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static void tapPoint(lv_indev_t *pointer, int x, int y) {
    touch = {x,y};
    pressed = true; lv_indev_read(pointer); advance();
    pressed = false; lv_indev_read(pointer); advance(); ++taps;
}
static void tap(lv_indev_t *pointer, lv_obj_t *object) {
    lv_area_t a; lv_obj_get_coords(object, &a);
    tapPoint(pointer, (a.x1+a.x2)/2, (a.y1+a.y2)/2);
}
static bool overlaps(lv_obj_t *a, lv_obj_t *b) {
    lv_area_t x,y; lv_obj_get_coords(a,&x); lv_obj_get_coords(b,&y);
    return x.x1<=y.x2 && y.x1<=x.x2 && x.y1<=y.y2 && y.y1<=x.y2;
}
static void checkLayout(View &view) {
    lv_obj_update_layout(view.root());
    const auto n = view.count();
    for (size_t i=0;i<n;++i) {
        auto *a=view.button(i);
        assert(lv_obj_get_width(a)>=48 && lv_obj_get_height(a)>=48);
        for (size_t j=i+1;j<n;++j) assert(!overlaps(a,view.button(j)));
        // An icon and a long label inside the same card must never intersect.
        if (lv_obj_get_child_count(a)>1) assert(!overlaps(lv_obj_get_child(a,0),lv_obj_get_child(a,1)));
    }
    auto *dock=view.dock();
    assert(!overlaps(view.body(),dock));
    for (unsigned i=0;i<lv_obj_get_child_count(dock);++i) {
        auto *a=lv_obj_get_child(dock,i);
        assert(lv_obj_get_width(a)>=48 && lv_obj_get_height(a)>=48);
        lv_area_t rect; lv_obj_get_coords(a,&rect);
        assert(rect.x1>=0 && rect.x2<width && rect.y1>=0 && rect.y2<height);
        for(unsigned j=i+1;j<lv_obj_get_child_count(dock);++j) assert(!overlaps(a,lv_obj_get_child(dock,j)));
    }
}
static void screenshot(const std::filesystem::path &path) {
    advance(); capturedFrames.push_back(path.filename().string());std::ofstream out(path,std::ios::binary);
    out<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(auto c:framebuffer) { char rgb[]={char(((c>>11)&31)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)}; out.write(rgb,3); }
}
struct MemoryFS {
    std::map<std::string,std::string> files;
    bool failPromote=false;
    bool exists(const std::string &s){return files.count(s);}
    bool remove(const std::string &s){return files.erase(s);}
    bool rename(const std::string &a,const std::string &b){
        if(failPromote && a=="/config.tmp")return false;
        if(!exists(a)||exists(b))return false;
        files[b]=files[a];files.erase(a);return true;
    }
};
static void storageTests() {
    for(const char *p:{"/", "/config/ui.json", "/themes/icons/wifi.png", "/apps_data/test.log"}) assert(safePath(p));
    for(const char *p:{"../escape", "/../escape", "/config/../../escape", "/config/./file", "/themes/..", "/config\\..\\escape", "/.. /escape", "/.../escape", "/config/x./escape", "/0:/escape"}) assert(!safePath(p));
    MemoryFS fs; fs.files["/config"]="old";fs.files["/config.tmp"]="new";
    assert(commit(fs,std::string("/config.tmp"),std::string("/config")));
    assert(fs.files["/config"]=="new" && fs.files["/config.bak"]=="old");
    fs.files["/config.tmp"]="newer";fs.failPromote=true;
    assert(!commit(fs,std::string("/config.tmp"),std::string("/config")));
    assert(fs.files["/config"]=="new");
    fs.failPromote=false;fs.rename("/config","/config.bak");
    fs.files.erase("/config");fs.files["/config.bak"]="recoverable";
    assert(recover(fs,std::string("/config")) && fs.files["/config"]=="recoverable");
}
#include "screen_titles.h"
int main(int argc,char **argv) {
    std::filesystem::path output=argc>1?argv[1]:"screenshots"; std::filesystem::create_directories(output);
    storageTests();
    assert(!hostedBleHeadroom(63*1024,64*1024));
    assert(!hostedBleHeadroom(128*1024,15*1024));
    assert(hostedBleHeadroom(96*1024,24*1024));
    assert(pt("BLE Scan")=="Buscar dispositivos BLE");
    assert(pt("my-SSID-Read")=="my-SSID-Read");
    lv_init();
    auto *fs=lv_fs_get_drv('T');assert(fs);originalOpen=fs->open_cb;fs->open_cb=countOpen;
    for(auto *font:{&lv_font_montserrat_20,&lv_font_montserrat_24,&lv_font_montserrat_32}) {
        for(uint32_t code:{0x00e7u,0x00e3u,0x00f3u,0x00edu,0x00e9u,0x00eau}) {
            lv_font_glyph_dsc_t glyph{};
            assert(lv_font_get_glyph_dsc(localeFont(font),&glyph,code,0));
            assert(!glyph.is_placeholder);
        }
    }
    {
        uint8_t values[256];
        const std::vector<int> empty;
        signalEnvelope(values,0,[](size_t){return 0;},-95,-20);
        for(auto v:values)assert(v==0);
        const std::vector<int> rssi={-120,-95,-60,-20,5};
        signalEnvelope(values,rssi.size(),[&](size_t i){return rssi[i];},-95,-20);
        assert(values[0]==0 && values[255]==100);
        digitalEnvelope(values,{100,100,200,200},0,600,1);
        assert(values[0]==100 && values[64]==0 && values[128]==100 && values[255]==0);
        digitalEnvelope(values,{100,100,200,200},100,200,1);
        assert(values[0]==0 && values[255]==100);
        digitalEnvelope(values,empty,0,600,1);for(auto v:values)assert(v==0);
        digitalEnvelope(values,{100,100},0,600,0);for(auto v:values)assert(v==0);
    }
    {
        assert(!validTouch(0,true,100,100,1280,720));
        assert(!validTouch(1,true,-1,100,1280,720));
        assert(!validTouch(1,true,1280,100,1280,720));
        assert(!validTouch(1,true,100,720,1280,720));
        assert(!validTouch(1,false,100,100,1280,720));
        assert(validTouch(1,true,1279,719,1280,720));
        NavigationLatch input;
        assert(input.retain(true,true,1000));
        assert(input.retain(true,true,1050)); // timer's 50 ms polling retains the key
        assert(!input.retain(true,false,1060)); // consumed keys never leak
        assert(input.retain(true,true,1070));
        assert(!input.retain(true,true,1220)); // unsupported actions expire
        assert(!input.retain(false,true,1230)); // legacy screens retain original policy
        assert(input.retain(true,true,UINT32_MAX-25));
        assert(input.retain(true,true,20)); // millis() rollover
        assert(!input.retain(true,true,200));
    }
    {
        ChoiceState confirmation(2,false);
        assert(confirmation.apply(ChoiceState::Cancel)==-1);
        assert(confirmation.apply(ChoiceState::Next)==1);
        assert(confirmation.apply(ChoiceState::Previous)==ChoiceState::Pending);
        assert(confirmation.apply(ChoiceState::Accept)==0);
        ChoiceState three(3,false);assert(three.apply(ChoiceState::Previous)==2);
        ChoiceState cursor(3,true);
        assert(cursor.apply(ChoiceState::Next)==ChoiceState::Pending && cursor.selected()==1);
        assert(cursor.apply(ChoiceState::Accept)==1);
        assert(cursor.apply(ChoiceState::Cancel)==ChoiceState::Pending && cursor.selected()==0);
        assert(cursor.apply(ChoiceState::Previous)==ChoiceState::Pending && cursor.selected()==2);
        assert(cursor.apply(ChoiceState::Accept)==2);
        ChoiceState empty(0,false);assert(empty.apply(ChoiceState::Cancel)==0);
    }
    int scenarios=0;
    for(auto resolution:std::vector<std::pair<int,int>>{{1280,720},{720,1280}}) {
        width=resolution.first;height=resolution.second;framebuffer.assign(width*height,0);
        auto *display=lv_display_create(width,height);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
        std::vector<uint16_t> buffer(width*40);
        lv_display_set_buffers(display,buffer.data(),nullptr,buffer.size()*2,LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(display,flush);
        auto *pointer=lv_indev_create();lv_indev_set_type(pointer,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(pointer,read);
        lv_indev_set_display(pointer,display);
        View view;view.init(display);
        std::vector<Palette> palettes(3);
        palettes[0].name="Grafite";palettes[0].background=0x151B23;palettes[0].panel=0x243041;
        palettes[0].text=0xF8FAFC;palettes[0].muted=0xCBD5E1;palettes[0].border=0x64748B;palettes[0].accent=0xFF6B83;
        palettes[1].name="Oceano";palettes[1].background=0x0B1B2D;palettes[1].panel=0x17354D;
        palettes[1].text=0xF1F9FF;palettes[1].muted=0xB8D8ED;palettes[1].border=0x5D849D;palettes[1].accent=0x67D8EF;
        for(int theme=0;theme<3;++theme) {
            for(int count:{0,1,2,8,16,40}) {
                std::vector<Item> items;
                for(int i=0;i<count;++i) items.push_back({i==0?"WiFi":"Categoria com um nome muito longo para testar truncamento e evitar sobreposicao "+std::to_string(i),"",i==0?"T:/themes/icons/graphite/wifi.png":"",i!=1,i==2});
                for(bool home:{false,true}) {
                    view.menu("Ferramentas",items,home,palettes[theme]);advance();checkLayout(view);++scenarios;
                    if(count>0) {view.focus(0);advance();tap(pointer,view.button(0));assert(view.takeEvent()==0);}
                    if(count>1) {view.focus(1);advance();tap(pointer,view.button(1));assert(view.takeEvent()==View::None);}
                    if(count>2) {view.focus(0);view.move(1);assert(view.focused()==2);view.activate();assert(view.takeEvent()==2);}
                    if(count>3) {view.focus(count-1);advance();checkLayout(view);tap(pointer,view.button(count-1));assert(view.takeEvent()==count-1);}
                    if(count==16&&home) {view.focus(0);advance();screenshot(output/(std::to_string(width)+"-theme-"+std::to_string(theme)+".ppm"));}
                    tap(pointer,lv_obj_get_child(view.dock(),home?2:2));assert(view.takeEvent()==View::Themes);
                }
            }
        }
        const std::vector<std::pair<std::string,std::string>> apps = {
            {"WiFi","wifi"},{"BLE","bluetooth"},{"RF","radio"},{"NRF24","radio"},
            {"LoRa","antenna"},{"FM","radio"},{"IR","remote"},{"Ethernet","network"},
            {"GPS","gps"},{"RFID","card"},{"Files","folder"},{"JS Interpreter","script"},
            {"Clock","clock"},{"Others","apps"},{"Wire Tools","wire"},{"Config","settings"}};
        for(int theme=0;theme<3;++theme) {
            const char *id=theme==0?"graphite":theme==1?"ocean":"light";
            std::vector<Item> real;
            for(auto &app:apps) real.push_back({app.first,"",std::string("T:/themes/icons/")+id+"/"+app.second+".png",true,false});
            view.menu("Ferramentas",real,true,palettes[theme]);advance();checkLayout(view);
            const size_t opensBeforeFocus=fileOpens;
            view.focus(1);advance();view.focus(0);advance();
            assert(fileOpens==opensBeforeFocus); // Cached icons cannot reopen SD files on focus.
            screenshot(output/(std::to_string(width)+"-home-"+id+".ppm"));
        }
        for(int mode=0;mode<3;++mode) {
            view.input("Digite sua mensagem","",8,true,mode,palettes[0]);advance();checkLayout(view);
            // The actual textarea and keyboard rectangles may never intersect.
            auto *textarea=lv_obj_get_child(view.body(),0),*keyboard=lv_obj_get_child(view.body(),1);
            assert(!overlaps(textarea,keyboard));
            assert(lv_obj_get_height(keyboard)>=200);
            auto *matrix=reinterpret_cast<lv_buttonmatrix_t *>(keyboard);
            lv_area_t origin; lv_obj_get_coords(keyboard,&origin);
            const char *target=mode==0?"q":mode==1?"a":"1";
            bool typed=false;
            for(unsigned b=0;b<matrix->btn_cnt;++b) {
                auto a=matrix->button_areas[b];
                assert(a.x1<=a.x2 && a.y1<=a.y2);
                for(unsigned c=b+1;c<matrix->btn_cnt;++c) {
                    auto z=matrix->button_areas[c];
                    assert(!(a.x1<=z.x2&&z.x1<=a.x2&&a.y1<=z.y2&&z.y1<=a.y2));
                }
                if(std::string(lv_buttonmatrix_get_button_text(keyboard,b))==target) {
                    tapPoint(pointer,origin.x1+(a.x1+a.x2)/2,origin.y1+(a.y1+a.y2)/2);
                    assert(view.text()==target);typed=true;
                }
            }
            assert(typed);
            view.type("abcdef0123456789");assert(view.text().length()<=8);
            if(mode==1) {for(char c:view.text())assert(isxdigit(c));}
            if(mode==2) {for(char c:view.text())assert(isdigit(c)||c=='.'||c=='-');}
            view.backspace();
            tap(pointer,lv_obj_get_child(view.dock(),1));assert(view.takeEvent()==View::Submit);
            screenshot(output/(std::to_string(width)+"-keyboard-"+std::to_string(mode)+".ppm"));++scenarios;
        }
        view.slider("Brilho da tela",75,1,100,palettes[0]);advance();checkLayout(view);
        view.adjustSlider(100);assert(view.sliderValue()==100);
        view.adjustSlider(-200);assert(view.sliderValue()==1);
        tap(pointer,lv_obj_get_child(view.body(),1));assert(view.sliderValue()>40&&view.sliderValue()<60);
        tap(pointer,lv_obj_get_child(view.dock(),1));assert(view.takeEvent()==View::Submit);
        screenshot(output/(std::to_string(width)+"-brightness.ppm"));++scenarios;
        for (bool stopOnly : {false,true}) {
            view.operation("Operação simulada",stopOnly,palettes[0]);advance();checkLayout(view);
            view.operationText("Em execução","Dispositivos: 12\nCanal: 6\nAtualização parcial");advance();
            assert(lv_obj_get_x(view.dock())>lv_obj_get_x(view.body()));
            for (unsigned i=0;i<4;++i) {
                tap(pointer,lv_obj_get_child(view.dock(),i));
                const int expected[]={View::Previous,View::Next,View::Submit,View::Back};
                assert(view.takeEvent()==(stopOnly&&i!=3?View::None:expected[i]));
            }
            flushedPixels=0;
            view.operationText("Em execução","Dispositivos: 12\nCanal: 6\nAtualização parcial");advance();
            assert(flushedPixels==0); // Identical status must not trigger any redraw.
            view.operationText("Concluído","Dispositivos: 12\nCanal: 6\nAtualização parcial");advance();
            assert(flushedPixels<size_t(width*height/2));
            screenshot(output/(std::to_string(width)+"-operation-"+std::to_string(stopOnly)+".ppm"));++scenarios;
        }
        // Reader panels reuse their widgets, preserve long data, and scroll
        // independently of the fixed right-hand controls in every theme.
        for(int theme=0;theme<3;++theme) {
            const std::vector<std::pair<std::string,std::vector<std::string>>> samples={
                {"Leitor infravermelho",{"Signals captured: 12","Protocol: NEC","Bits: 32","RAW Data Captured: 9000 4500 560 560 560 1690"}},
                {"TAG-O-MATIC",{"Device type: MIFARE Classic","UID: 01 23 45 67 89 AB CD EF","Pages read: 64/64","Payload type: Texto"}},
                {"Rastreamento GPS",{"File: percurso-2026-11-15.gpx","Latitude: -15.601410","Longitude: -56.097891","Distance: 12.45 km","Sat: 14","HDOP: 0.91"}}
            };
            int sampleIndex=0;
            for(const auto &sample:samples) {
                view.dataPanel(sample.first,palettes[theme]);
                view.dataText("Leitura em andamento",sample.second);advance();checkLayout(view);
                auto *grid=view.dataGrid();assert(grid);
                assert(lv_obj_get_child_count(grid)==sample.second.size());
                for(unsigned i=0;i<lv_obj_get_child_count(grid);++i) {
                    auto *card=lv_obj_get_child(grid,i);
                    assert(!overlaps(lv_obj_get_child(card,0),lv_obj_get_child(card,1)));
                    for(unsigned j=i+1;j<lv_obj_get_child_count(grid);++j)
                        assert(!overlaps(card,lv_obj_get_child(grid,j)));
                }
                auto *first=lv_obj_get_child(grid,0);
                flushedPixels=0;view.dataText("Leitura em andamento",sample.second);advance();
                assert(flushedPixels==0);assert(lv_obj_get_child(grid,0)==first);
                view.operationControls("Descartar","Salvar","Emular");advance();
                for(unsigned i=0;i<4;++i){tap(pointer,lv_obj_get_child(view.dock(),i));const int ids[]={View::Previous,View::Next,View::Submit,View::Back};assert(view.takeEvent()==ids[i]);}
                if(sampleIndex==1)view.operationControls("","","Modo");
                if(sampleIndex==2)view.operationControls("","","","Parar");
                screenshot(output/(std::to_string(width)+"-panel-"+std::to_string(theme)+"-"+std::to_string(sampleIndex)+".ppm"));++scenarios;++sampleIndex;
                // Fewer rows must remove old content, including after long dumps.
                std::vector<std::string> longRows(12,"UID: "+std::string(240,'A'));
                view.dataText("Dados completos",longRows);advance();assert(lv_obj_get_scroll_bottom(grid)>0);
                lv_obj_scroll_to_y(grid,lv_obj_get_scroll_bottom(grid),LV_ANIM_OFF);advance();checkLayout(view);
                view.dataText("Aguardando dados",{});advance();assert(lv_obj_get_child_count(grid)==0);
            }
            view.document("Arquivo de texto",std::string(3000,'X')+"\nFim do documento","Página 1 de 2",palettes[theme]);advance();checkLayout(view);
            view.operationControls("","Próxima página","Continuar");advance();
            tap(pointer,lv_obj_get_child(view.dock(),0));assert(view.takeEvent()==View::None);
            tap(pointer,lv_obj_get_child(view.dock(),1));assert(view.takeEvent()==View::Next);
            auto *scroller=lv_obj_get_child(view.body(),1);
            auto *text=lv_obj_get_child(scroller,0);
            assert(lv_color_eq(lv_obj_get_style_text_color(text,0),lv_color_hex(palettes[theme].text)));
            assert(std::string(lv_label_get_text(text)).find("Fim do documento")!=std::string::npos);
            view.document("Informações do dispositivo","Interface: PN532\nEstado: aguardando leitura\nUID: 01 23 45 67 89 AB CD EF\nProtocolo: ISO 14443-A\n\nOs dados completos podem ser consultados por rolagem.\nOs arquivos permanecem no microSD em /truce.","Página 1 de 2",palettes[theme]);
            view.operationControls("","Próxima página","Continuar");
            screenshot(output/(std::to_string(width)+"-document-"+std::to_string(theme)+".ppm"));++scenarios;
        }
        // Specialized instruments exercise the same native widgets used on Tab5.
        for(int theme=0;theme<3;++theme) {
            const auto &palette=palettes[theme];
            for(bool sdr:{false,true}) {
                view.spectrum("Analisador de sinais",sdr,palette);advance();checkLayout(view);
                std::array<uint8_t,256> live{},peak{};
                for(int i=0;i<256;++i){live[i]=(i*7)%101;peak[i]=std::min(100,int(live[i])+12);}
                live[0]=255;view.spectrumTrace(live.data(),peak.data(),-10,300,false);advance();
                auto *series=lv_chart_get_series_next(view.graph(),nullptr);
                series=lv_chart_get_series_next(view.graph(),series);
                assert(lv_chart_get_y_array(view.graph(),series)[0]==100);
                flushedPixels=0;view.spectrumTrace(live.data(),peak.data(),-10,300,false);advance();assert(flushedPixels==0);
                std::vector<int> positions;std::vector<std::string> labels;
                for(int i=0;i<14;++i){positions.push_back(i*255/13);labels.push_back(std::to_string(i+1));}
                view.spectrumRuler(positions,labels,13);advance();
                assert(lv_obj_get_child_count(view.ruler())==14);
                for(unsigned i=0;i<14;++i)for(unsigned j=i+1;j<14;++j)
                    assert(!overlaps(lv_obj_get_child(view.ruler(),i),lv_obj_get_child(view.ruler(),j)));
                for(int row=0;row<48;++row){for(int i=0;i<256;++i)live[i]=(i+row*5)%101;view.spectrumRow(live.data());}
                view.spectrumTrace(live.data(),peak.data(),50,80,false);view.operationText("Sinal recebido • canal 6","");advance();
                for(unsigned i=0;i<lv_obj_get_child_count(view.body());++i)
                    for(unsigned j=i+1;j<lv_obj_get_child_count(view.body());++j)
                        assert(!overlaps(lv_obj_get_child(view.body(),i),lv_obj_get_child(view.body(),j)));
                for(unsigned i=0;i<4;++i){tap(pointer,lv_obj_get_child(view.dock(),i));const int ids[]={View::Previous,View::Next,View::Submit,View::Back};assert(view.takeEvent()==ids[i]);}
                screenshot(output/(std::to_string(width)+"-spectrum-"+std::to_string(theme)+"-"+std::to_string(sdr)+".ppm"));++scenarios;
            }
            view.spectrum("Forma de onda RF",false,palette,false);
            std::array<uint8_t,256> digital{};
            digitalEnvelope(digital.data(),{100,100,200,200},0,600,1);
            view.spectrumTrace(digital.data(),nullptr,1,0,false);
            view.spectrumRuler({0,128,255},{"0 ms","0,3 ms","0,6 ms"},-1);
            view.operationText("433,92 MHz • níveis 0 / 1","");advance();checkLayout(view);
            assert(view.waterfall()==nullptr);
            screenshot(output/(std::to_string(width)+"-waveform-"+std::to_string(theme)+".ppm"));++scenarios;
            for(int instrument=0;instrument<3;++instrument) {
                const std::string titles[]={"Relógio","Temporizador","Rádio FM"};
                view.dial(titles[instrument],palette);
                lv_font_glyph_dsc_t digits{};
                assert(instrumentFont()!=&lv_font_montserrat_32);
                assert(lv_font_get_glyph_dsc(instrumentFont(),&digits,'0',0) && digits.box_h>=40);
                view.dialValue(instrument==2?"102.30 MHz":"23:59:59",instrument==2?"Ruído medido: 18":"Horário local",600);
                view.operationControls("","",instrument==0?"Opções":"Encerrar","Sair");advance();checkLayout(view);
                flushedPixels=0;view.dialValue(instrument==2?"102.30 MHz":"23:59:59",instrument==2?"Ruído medido: 18":"Horário local",600);advance();assert(flushedPixels==0);
                tap(pointer,lv_obj_get_child(view.dock(),0));assert(view.takeEvent()==View::None);
                tap(pointer,lv_obj_get_child(view.dock(),2));assert(view.takeEvent()==View::Submit);
                tap(pointer,lv_obj_get_child(view.dock(),3));assert(view.takeEvent()==View::Back);
                screenshot(output/(std::to_string(width)+"-dial-"+std::to_string(theme)+"-"+std::to_string(instrument)+".ppm"));++scenarios;
            }
            view.media(palette);
            const std::string filename="Uma música com título longo e acentos para testar a quebra de linhas.mp3";
            view.mediaValue(filename,"Reproduzindo","01:35","04:20",365,75,true,1);advance();checkLayout(view);
            auto *grid=view.mediaGrid();assert(lv_obj_get_child_count(grid)==4);
            for(unsigned i=0;i<4;++i){
                for(unsigned j=i+1;j<4;++j)assert(!overlaps(lv_obj_get_child(grid,i),lv_obj_get_child(grid,j)));
                tap(pointer,lv_obj_get_child(grid,i));assert(view.takeEvent()==100+int(i));
            }
            flushedPixels=0;view.mediaValue(filename,"Reproduzindo","01:35","04:20",365,75,true,1);advance();assert(flushedPixels==0);
            screenshot(output/(std::to_string(width)+"-media-"+std::to_string(theme)+".ppm"));++scenarios;
            view.mediaValue(filename,"Pausado","01:35","04:20",365,0,false,3);advance();
            view.game(palette);std::vector<std::pair<int,int>> fish={{80,15},{130,55},{210,105},{275,150},{350,80}};
            view.gameFrame(26,73,fish,12,false);advance();checkLayout(view);
            assert(!overlaps(lv_obj_get_child(view.body(),0),view.gameBoard()));
            lv_area_t rail;lv_obj_get_coords(view.dock(),&rail);
            std::vector<uint16_t> railPixels;
            for(int y=rail.y1;y<=rail.y2;++y)for(int x=rail.x1;x<=rail.x2;++x)railPixels.push_back(framebuffer[y*width+x]);
            for(int frame=0;frame<30;++frame){for(auto &f:fish)--f.first;view.gameFrame(26,73,fish,12,false);advance();}
            size_t offset=0;
            for(int y=rail.y1;y<=rail.y2;++y)for(int x=rail.x1;x<=rail.x2;++x)assert(railPixels[offset++]==framebuffer[y*width+x]);
            for(unsigned i=0;i<4;++i){tap(pointer,lv_obj_get_child(view.dock(),i));const int ids[]={View::Previous,View::Next,View::Submit,View::Back};assert(view.takeEvent()==ids[i]);}
            // Directional touch controls repeat while held and stop on release.
            lv_area_t up;lv_obj_get_coords(lv_obj_get_child(view.dock(),0),&up);
            touch={(up.x1+up.x2)/2,(up.y1+up.y2)/2};pressed=true;lv_indev_read(pointer);advance(650);
            assert(view.takeEvent()==View::Previous);advance(250);assert(view.takeEvent()==View::Previous);
            pressed=false;lv_indev_read(pointer);advance();assert(view.takeEvent()==View::None);
            view.gameFrame(26,73,fish,12,true);advance();
            screenshot(output/(std::to_string(width)+"-megalodon-"+std::to_string(theme)+".ppm"));++scenarios;
        }
        for(int theme=0;theme<3;++theme) {
            for(bool ble:{false,true}) {
                std::vector<Item> devices={
                    {"Clock\n-53 dBm • WPA2 • Canal 6","","",true,false},
                    {"Rede com nome longo e acentos São Paulo\n-78 dBm • WPA3 • Canal 11","","",true,false},
                    {"Dispositivo de teste\n-65 dBm • 01:23:45:67:89:AB","","",true,false},
                    {"Main Menu","","",true,false}};
                if(ble) {
                    devices[0].label="Clock\n-53 dBm • 01:23:45:67:89:AB";
                    devices[1].label="Sensor de temperatura\n-78 dBm • 12:34:56:78:9A:BC";
                } else devices[2].label="Rede de visitantes\n-65 dBm • Aberta • Canal 1";
                view.menu(ble?"Dispositivos BLE":"Redes Wi-Fi",devices,false,palettes[theme]);advance();checkLayout(view);
                assert(std::string(lv_label_get_text(lv_obj_get_child(view.button(0),1)))=="Clock");
                for(unsigned i=0;i<3;++i) {
                    auto *card=view.button(i);assert(lv_obj_get_child_count(card)==3);
                    for(unsigned x=0;x<3;++x)for(unsigned y=x+1;y<3;++y)assert(!overlaps(lv_obj_get_child(card,x),lv_obj_get_child(card,y)));
                }
                tap(pointer,view.button(0));assert(view.takeEvent()==0);
                screenshot(output/(std::to_string(width)+"-discovery-"+std::to_string(theme)+"-"+std::to_string(ble)+".ppm"));++scenarios;
            }
            for(int count=0;count<=3;++count) {
                std::vector<std::string> choices;
                const char *labels[]={"Sim","Não","Detalhes"};for(int i=0;i<count;++i)choices.emplace_back(labels[i]);
                view.choice("Confirmar operação","Dispositivo selecionado: teste simulado. Nenhuma ação de rádio será executada.",choices,palettes[theme]);advance();checkLayout(view);
                assert(view.takeEvent()==View::None); // Opening a dialog never approves it.
                for(unsigned i=0;i<view.count();++i){tap(pointer,view.button(i));assert(view.takeEvent()==int(i));}
                for(unsigned i=0;i<4;++i) {
                    tap(pointer,lv_obj_get_child(view.dock(),i));
                    const int expected[]={count>2?View::Previous:View::None,count>1?View::Next:View::None,View::Submit,View::Back};
                    assert(view.takeEvent()==expected[i]);
                }
                screenshot(output/(std::to_string(width)+"-confirmation-"+std::to_string(theme)+"-"+std::to_string(count)+".ppm"));++scenarios;
            }
            view.choice("Bluetooth","Buscando dispositivos",{},palettes[theme],true);advance(1600);checkLayout(view);
            for(unsigned i=0;i<4;++i){tap(pointer,lv_obj_get_child(view.dock(),i));assert(view.takeEvent()==View::None);}++scenarios;
            view.dataPanel("Bluetooth",palettes[theme]);
            view.dataText("Informações do dispositivo",{"Nome: Clock","Endereço: 01:23:45:67:89:AB","Sinal: -53 dBm"});
            view.operationControls("","","Continuar","Voltar");advance();checkLayout(view);
            assert(std::string(lv_label_get_text(lv_obj_get_child(lv_obj_get_child(view.dataGrid(),0),1)))==" Clock");
            screenshot(output/(std::to_string(width)+"-bluetooth-info-"+std::to_string(theme)+".ppm"));++scenarios;
            view.terminal("Terminal SSH",palettes[theme]);
            std::string transcript;for(int i=0;i<60;++i)transcript+="Linha "+std::to_string(i)+": saída sintética, sem conexão remota.\n";
            const std::string command="> "+std::string(200,'x');view.terminalText(transcript,command);advance();checkLayout(view);
            auto *scroller=lv_obj_get_child(view.body(),1);assert(lv_obj_get_scroll_y(scroller)>0);
            flushedPixels=0;view.terminalText(transcript,command);advance();assert(flushedPixels==0);
            for(int step=0;step<32;++step)view.scrollTerminal(-1);advance();
            view.terminalText(transcript+"Nova linha\n",command);advance();assert(lv_obj_get_scroll_y(scroller)==0);
            screenshot(output/(std::to_string(width)+"-terminal-"+std::to_string(theme)+".ppm"));++scenarios;
        }
        for(int theme=0;theme<3;++theme) {
            size_t catalogIndex=0;
            for(const auto &title:screenTitles) {
                view.dataPanel(title,palettes[theme]);
                std::vector<std::string> rows;
                for(int row=0;row<48;++row)rows.push_back("Campo "+std::to_string(row)+": dado preservado 01:23:45:67:89:AB");
                view.dataText("Aguardando dados do dispositivo",rows);
                view.operationControls("Anterior","Próximo","Selecionar","Sair");advance();checkLayout(view);
                assert(lv_obj_get_child_count(view.dataGrid())==48);
                screenshot(output/(std::to_string(width)+"-catalog-"+std::to_string(theme)+"-"+std::to_string(catalogIndex++)+".ppm"));
                tap(pointer,lv_obj_get_child(view.dock(),3));assert(view.takeEvent()==View::Back);++scenarios;
            }
            struct Fixture {const char *id;const char *title;const char *state;std::vector<std::string> fields;};
            const std::vector<Fixture> fixtures={
                {"wifi-connect","Conectar Wi-Fi","Conectando: tentativa 3 de 20",{"Rede: Oficina","Segurança: WPA2","Canal: 6","Sinal: -53 dBm"}},
                {"ble-config","Ajustes Bluetooth","Selecionado: Intervalo",{"Intervalo: 100 ms","Pausa: 50 ms","Potência: Normal","MAC: Fixo"}},
                {"ble-running","Bluetooth","Enviados: 128 | 12,0 pacotes/s",{"Dispositivo: Sensor de teste","Intervalo: 100 ms","Pausa: 50 ms","Potência: Normal","MAC: Fixo","Selecionado: Intervalo"}},
                {"lora","LoRa","Mensagens de demonstração",{"Usuário: Oficina","Recebido: Olá, mensagem de teste!","Sinal: -72 dBm","Frequência: 915 MHz"}},
                {"continuity","Continuidade: pinos","Contato detectado",{"Pino A: GPIO 5","Pino B: GPIO 6","Contatos: 3","Duração: 1,2 s"}},
                {"ibutton","iButton","Aguardando leitura",{"Tipo: DS1990A","Identificador: 01 23 45 67 89 AB CD EF","Estado: Nenhum dispositivo conectado"}},
                {"rf-record","Gravar RF","Aguardando sinal",{"Frequência: 433,92 MHz","Sinal: -85 dBm","Pulsos capturados: 0","Arquivo: /truce/recordings/teste.sub"}},
                {"clicker","Auto Clicker","Selecionado: Intervalo",{"Intervalo: 100 ms","Botão: Esquerdo","Cliques: 20"}},
                {"infrared","Infravermelho","Pausado",{"Ajuste selecionado: 1","Modo: Básico","Frequência: 38000 Hz","Densidade: 5","Mark: 12 us","Space: 12 us","Duty cycle: 50%","Contagem: 0"}},
                {"webui","WebUI","Aguardando conexão",{"Endereço: http://192.168.4.1","Rede: Oficina","Armazenamento: /truce"}},
                {"file-transfer","Enviar arquivo","Transferência simulada",{"Arquivo: exemplo.txt","Tamanho: 12 KB","Progresso: 75%"}},
                {"spi","Console SPI","Aguardando comando",{"Barramento: SPI","Frequência: 1 MHz","Modo: 0","Resultado: 00 FF 12 34"}}
            };
            for(const auto &fixture:fixtures) {
                view.dataPanel(fixture.title,palettes[theme]);view.dataText(fixture.state,fixture.fields);
                const std::string id=fixture.id;
                if(id=="wifi-connect")view.operationControls("","","","");
                else if(id=="ble-running")view.operationControls("Anterior","Próximo","Editar","Parar");
                else if(id=="infrared")view.operationControls("Diminuir","Aumentar","Próximo ajuste","Sair");
                else view.operationControls("Anterior","Próximo","Selecionar",id=="clicker"?"Sair":"Voltar");
                advance();checkLayout(view);
                auto *grid=view.dataGrid();assert(lv_obj_get_child_count(grid)==fixture.fields.size());
                for(unsigned i=0;i<lv_obj_get_child_count(grid);++i) {
                    auto *card=lv_obj_get_child(grid,i);
                    assert(!overlaps(lv_obj_get_child(card,0),lv_obj_get_child(card,1)));
                }
                screenshot(output/(std::to_string(width)+"-review-"+std::to_string(theme)+"-"+fixture.id+".ppm"));++scenarios;
            }
            view.spectrum("Analisador lógico",false,palettes[theme],false);
            std::vector<std::vector<uint8_t>> lanes(4,std::vector<uint8_t>(256));
            for(int c=0;c<4;++c)for(int x=0;x<256;++x)lanes[c][x]=(x/(8*(c+1)))%2;
            lanes[2][100]=2;view.logicSignals(lanes,2);
            auto *logicChart=lv_obj_get_child(view.body(),1);assert(lv_chart_get_point_count(logicChart)==512);
            bool narrowPulse=false;int seriesCount=0;
            for(auto *series=lv_chart_get_series_next(logicChart,nullptr);series;series=lv_chart_get_series_next(logicChart,series)) {
                ++seriesCount;auto *values=lv_chart_get_y_array(logicChart,series);
                narrowPulse|=values[200]==110 && values[201]==175;
            }
            assert(seriesCount==4 && narrowPulse);view.operationText("GPIO 2 | Janela: 10 ms | Cursor: 5 ms","");
            view.operationControls("Esquerda","Direita","Zoom / canal","Sair");advance();checkLayout(view);
            screenshot(output/(std::to_string(width)+"-logic-"+std::to_string(theme)+".ppm"));++scenarios;
        }
        view.splash(palettes[0]);advance(700);
        view.splashProgress(100);advance();
        screenshot(output/(std::to_string(width)+"-splash.ppm"));++scenarios;
        view.message("microSD necessario","Insira o microSD. Dados em /truce.",palettes[0],"Tentar novamente");advance();checkLayout(view);
        tap(pointer,lv_obj_get_child(view.dock(),0));assert(view.takeEvent()==View::Submit);
        screenshot(output/(std::to_string(width)+"-storage.ppm"));
        view.clear();lv_obj_delete(view.root());lv_indev_delete(pointer);lv_display_delete(display);
    }
    std::ofstream captureIndex(output/"capturas.txt");
    for(const auto &file:capturedFrames)captureIndex<<file<<"\n";
    releaseLocaleFonts();lv_deinit();std::cout<<scenarios<<" real LVGL layout scenarios; "<<taps<<" simulated touch taps; storage recovery and confinement passed.\n";
}

# Truce para M5Stack Tab5

A navegação compartilhada usa **LVGL 9.3.0**: tela inicial, listas de opções,
teclados de texto/hexadecimal/números, seletor de temas, ajuste de brilho e
recuperação de microSD ausente. A referência visual é o projeto local
`Tab5LauncherPaludo`: painéis, cartões, cores de destaque e navegação inferior.
As telas de execução textuais adaptadas usam status ampliado e uma barra
lateral com Anterior, Próximo, Ação e Parar. Os controles encaminham somente
os eventos já existentes no módulo; não criam novas ações de rádio. No iBeacon,
apenas Parar fica habilitado. Foram adaptadas as telas compartilhadas dos
módulos BLE, iBeacon e as operações textuais que usam os títulos reconhecidos
em `runtime.cpp`. O espectro compartilhado, os instrumentos de relógio/temporizador e FM,
o reprodutor de áudio e o jogo nativo Megalodon também têm telas LVGL.
Scripts externos com desenhos próprios conservam sua API gráfica.

## Continuação da migração de telas

Os painéis de IR Read, RFID 125 kHz, Tag-O-Matic, SRIX, Amiibolink,
Chameleon, PN532 BLE/UART, leitura EMV, GPS Tracker, Wardriving e envio
WiGLE/WDG usam a estrutura LVGL de dados: status ampliado, cartões de
campos com ícones, rolagem e controles laterais. Informações de arquivo e
busca I²C usam o mesmo painel. A mudança é de apresentação e navegação;
as rotinas de comunicação e os formatos dos arquivos permanecem os mesmos.

No IR, os controles mudam conforme a etapa: descartar/salvar/emular,
salvar/novo sinal/enviar, ou aguardar captura. GPS oferece Parar; leitores
com seleção de modo oferecem Opções/Modo. Os eventos seguem os controles
que cada módulo já reconhecia.

O leitor compartilhado `ScrollableTextArea` passou para LVGL tanto na
consulta bloqueante quanto em atualizações ao vivo. Textos completos são
acessíveis por rolagem e navegação em janelas de 16 linhas; os dados do
arquivo não são traduzidos ou alterados. Isso abrange, por exemplo,
informações de AP/host, dumps e terminais que usam esse componente.
A atualização ao vivo preserva os objetos e redesenha somente o texto alterado.

A etapa de painéis cobriu 110 cenários e 334 toques simulados, incluindo dados
longos, cartões removidos após nova leitura, controles desabilitados e os
três temas nas duas orientações. Capturas: `artifacts/truce-ui/gps-panel.png`,
`reader-portrait.png` e `document.png`. Esses cenários usam dados sintéticos;
não validam leitores externos, GPS ou rotinas de ataque em hardware.

### Instrumentos e jogo nativo

Os caminhos de pulsos RF, onda quadrada e RSSI foram migrados preservando
as capturas e a navegação temporal. A conversão para 256 pontos é apenas
visual; testes cobrem saturação, dados vazios e deslocamento da janela.
A onda quadrada ocupa a área gráfica sem cascata.

O `SpectrumPlot` usado por Wi-Fi, NRF24 e cascata RF agora oferece gráfico
LVGL com traço, retenção de pico, faixa selecionada, até 16 legendas e
cascata 256 × 48 RGB565 (24 KiB em PSRAM). A atualização do traço é limitada
a 32 ms e dados idênticos não provocam novo desenho. O espectro do microfone
usa a mesma tela, preservando a aquisição/FFT e dispensando o framebuffer
legado do tamanho da tela.

Relógio, temporizador e FM usam mostradores dedicados. A fonte numérica de
64 px fica em `/truce/fonts/digits64.bin`, com fallback para o recurso base
se o arquivo não estiver disponível. O temporizador mantém as etapas de
horas, minutos, segundos e alarme; a tela de execução mostra o tempo restante.
A tela FM informa frequência e ruído/estado do RDS sem mudar os comandos
executados pelo módulo físico.

O reprodutor oferece tiles de reinício, reprodução/pausa, volume e repetição,
barra de progresso e navegação lateral. O seletor de volume tem prévia e
restaura o valor anterior ao cancelar. Nomes longos usam reticências sem
provocar refresh repetido.

O Megalodon é o jogo nativo deste firmware. Ele usa objetos LVGL, o tubarão
do microSD, peixes vetoriais, placar e pausa, em uma área lógica 320 × 180
que se adapta à orientação. A animação permanece dentro do campo e não
repinta a barra lateral. Subir/descer respondem ao toque inicial e à
pressão contínua. A leitura rápida do touch preserva teclas pendentes por
até 150 ms, evitando que telas com polling de 50 ms percam a navegação.
O ponteiro conserva a última posição na liberação, rejeita coordenadas
inválidas e exige uma liberação real após a troca de tela. Os logs de
seleção identificam eventos de serial, teclado e toque.
O Snake fornecido em `sd_files/interpreter` é um
script externo; sua interface de desenho não foi convertida em aplicativo nativo.

**Disponibilidade:** no perfil atual do Tab5, `HAS_SPEAKER` e `HAS_MICROPHONE`
ainda não estão habilitados. As interfaces correspondentes foram simuladas e
os arquivos C++ foram verificados com esses recursos habilitados, mas isso
não integra nem valida o codec físico do Tab5. FM, RF e NRF24 dependem dos
respectivos módulos externos. Esta migração não habilita transmissores nem
altera a lógica de rádio.

A suíte passou com **158 cenários e 484 toques simulados**, além das verificações
de armazenamento e transporte BLE. Ela cobre instrumentos, 14 legendas sem
sobreposição, valores de gráfico fora da escala, estados de áudio, textos
longos, pausa do jogo e ausência de redesenho para atualizações idênticas.
Capturas adicionais estão em `artifacts/truce-ui`: `spectrum.png`,
`audio-portrait.png`, `clock-portrait.png`, `timer.png`, `fm.png`, `waveform.png` e `megalodon.png`.

## Correção de boot para o Tab5 revisão 1.x

O alvo seleciona explicitamente `board_build.chip_variant = esp32p4_es`.
O Tab5 conectado ao diagnóstico usa ESP32-P4 v1.3. O SDK genérico `esp32p4`
corresponde ao silício ECO3 (v3.01–v3.99) e não é compatível com essa unidade.
A auditoria verifica também a variante registrada no mapa de link.

Em 2026-11-15, o boot foi medido no Tab5 v1.3 conectado usando o
Launcher_do_Paludo. O menu apareceu em cerca de 2,2 s após o início da aplicação,
contra 11,2 s da versão anterior; esse tempo não inclui a passagem pelo launcher.
A memória interna livre no menu passou de aproximadamente 75 kB para 229 kB.
O relatório com o SHA-256, slot atual e limites da validação está em
`artifacts/truce-ui/boot-hardware.json`. O endereço do slot é obtido novamente
em cada gravação e não deve ser copiado deste relatório para outra instalação.

### Instalação pelo Launcher_do_Paludo

Instale **`Truce-Tab5-app.bin`**, que contém somente a aplicação.
Não grave o binário combinado no offset zero de um aparelho com launcher:
ele substituiria o bootloader e a tabela de partições do launcher.
Para manutenção por USB sem trocar o launcher, `tools/flash_truce_slot.py`
lê a tabela real do dispositivo antes de calcular o endereço da aplicação.
Ele recusa slots inexistentes, imagens combinadas, falta de espaço e regiões
sobrepostas. Não reutilize endereços de um layout anterior: os slots do launcher
podem mudar de posição.

Os dados do Truce continuam no microSD em `/truce`; as partições do launcher
pertencem ao launcher e não são alteradas por esse pacote de aplicação.

## Instalação direta, sem launcher

1. Copie a pasta `sdcard/truce` inteira para a raiz de um microSD FAT32.
2. Grave o binário combinado `Truce-Tab5.bin` (ou `Bruce-m5stack-tab5.bin`) no
   ESP32-P4 com offset **0x0**. Ele contém bootloader, tabela de partições e app.
3. Insira o cartão e inicie o Tab5. Sem cartão, a aplicação mostra a tela de
   recuperação e aguarda; não cria armazenamento alternativo na flash.

O pacote altera a tabela de partições: há somente a aplicação factory.
O firmware executável e as constantes compiladas (por exemplo, o motor de UI e
as fontes) permanecem na flash, como necessário para executar no ESP32.
**Configurações e arquivos persistentes não usam flash.** Não há partições
NVS, SPIFFS, LittleFS, OTA metadata ou coredump. A aplicação bloqueia as entradas
de gravação/apagamento de partições; atualizadores internos não gravam firmware.
Para atualizar esta variante, use a gravação externa por USB.

## Estrutura do cartão

```text
/truce/
  config/
    ui.json                 # tema selecionado
    bruce.conf              # configurações criadas no primeiro boot
    brucePins.conf          # configuração de pinos
  boot/shark.png             # abertura animada
  fonts/latin{20,24,32}.bin  # acentos pt-BR
  fonts/digits64.bin         # mostradores nativos
  themes/
    graphite.json
    ocean.json
    light.json
    icons/<tema>/*.png
  preferences/<namespace>/  # chaves binárias dos módulos, incluindo relógio
  logs/
  apps_data/
```

Os caminhos antigos dos módulos (por exemplo, `/infrared`, `/interpreter` ou
`/chats.txt`) são relativos a `/truce`. Recursos já existentes de `sd_files`
podem ser copiados **para dentro de `/truce`**, mantendo suas subpastas.
O navegador de arquivos abre essa raiz virtual e não oferece acesso a arquivos
externos a ela. A camada VFS rejeita `..`, `.` e barras invertidas. A interface
não oferece USB Mass Storage; o acesso bruto ao cartão está bloqueado nesta
variante, pois ele escaparia da raiz de arquivos.

LittleFS não é montado: uma fachada de compatibilidade redireciona os acessos
antigos para o mesmo diretório no microSD. As preferências dos módulos usam
arquivos no cartão. Inicialização e abertura de NVS são interceptadas antes
dos SDKs; não há fallback de persistência na flash. Isso também impede que
bibliotecas de terceiros persistam pareamentos/calibrações em NVS. A validação
com rádio, acessórios e cartão físico continua necessária.

As configurações JSON e preferências usam arquivo temporário e versão `.bak`.
Após interrupção entre renomeações, a versão anterior é recuperada; um `.tmp`
órfão não é aceito como configuração completa. Isso reduz o risco de perda,
mas não torna FAT32 imune a remoção durante uma gravação.

## Temas e navegação

Use **Temas** na barra inferior ou **Config → Display & UI → UI Theme**.
Os três temas são Grafite, Oceano e Claro; Claro é o padrão para novas
configurações. O tema já selecionado no cartão é preservado. Os temas escuros
usam contraste ampliado e o envio de pixels usa o tipo RGB565 nativo do M5GFX,
evita a inversão dos bytes que distorcia as cores. Cada JSON pode definir
`background`, `panel`, `text`, `muted`, `border`, `accent` em `#RRGGBB`, além de
`icons`, que associa o nome da categoria a um arquivo relativo ao tema.

PNG transparente é recomendado para ícones. O LVGL também decodifica JPG e
BMP. A imagem é ajustada sem distorção; na falta de imagem válida, a categoria
recebe um símbolo interno. GIF animado não é utilizado nos cartões.

O splash do tubarão usa `/truce/boot/shark.png`, com uma entrada de 640 ms
em vez da espera fixa anterior de 7 segundos. O boot não inicia uma varredura
Wi-Fi obrigatória; a conectividade é inicializada quando uma função a solicita
ou quando a configuração explícita de Wi-Fi no boot está habilitada.

O cache de imagens é de 2 MiB em PSRAM, com leituras de arquivos em blocos.
Objetos LVGL e buffers de decodificação também usam PSRAM, preservando DRAM
interna para as pilhas de rádio. A verificação de memória no P4 considera o
host BLE e mantém reservas internas; nos demais alvos permanece o limite
original do controlador. O enlace e o controlador C6 são preparados por `hostedInitBLE()` antes de
inicializar o host NimBLE, evitando o abort do SDK quando o transporte ainda
não está configurado. Falhas são apresentadas como indisponibilidade do C6 ou
falta de memória, conforme a causa, antes de acessar o anunciante.

Fontes complementares de pt-BR ficam em `/truce/fonts/latin{20,24,32}.bin`,
com licença OFL incluída. Elas são lidas para RAM antes da interpretação para
não executar buscas por glifo no microSD. A tradução afeta a apresentação de
rótulos conhecidos, sem modificar comandos ou dados transmitidos. Mensagens
específicas de módulos ainda podem estar em inglês.

A tela usa a resolução completa, com cabeçalho e barra inferior reservados.
Cartões e listas rolam apenas na área central; textos longos são truncados.
Toque abre uma opção, e o teclado físico permite navegar e confirmar.
Cancelar o teclado retorna ESC ao módulo chamador. O brilho permite prévia,
aplicar ou cancelar restaurando o valor anterior.

## Compilar e simular

```sh
pio run -e m5stack-tab5
cmake -S tests/truce -B /tmp/truce-ui-host
cmake --build /tmp/truce-ui-host -j 6
ctest --test-dir /tmp/truce-ui-host --output-on-failure
python3 tools/check_truce_storage.py
python3 tools/package_truce.py
```

O simulador compila **o mesmo `ui_view.cpp` e LVGL 9.3.0 do firmware**, com
AddressSanitizer e UndefinedBehaviorSanitizer. Ele exercita as duas orientações,
três paletas, menus com 0 a 40 itens, rolagem, opções desabilitadas, rótulos
longos, teclado e controles de confirmação. Os testes comparam retângulos de
controles, ícones e legendas; simulam eventos de toque reais do LVGL; verificam
limites de texto, travessia de diretórios e recuperação de configurações.
As capturas PPM são geradas no diretório de build; exemplos PNG ficam em
`artifacts/truce-ui`.

No macOS, se o SDK selecionado tiver incompatibilidade com o linker instalado,
informe um SDK compatível ao CMake via `-DCMAKE_OSX_SYSROOT=/caminho/do/SDK`.

A suíte também verifica acentos nas três fontes, ausência de novas leituras
no SD ao focar ícones já em cache, nenhum redesenho para status idêntico e uma
área parcial para mudanças de status. O comando serial `ui_metrics` informa
tempos de cópia de pixels e os blocos livres de memória interna/PSRAM.

A simulação não emula a eletrônica do Tab5. Ainda é necessário validar tela,
toque, SD, consumo de memória e rádio na revisão física do dispositivo.

## Telas de rede e Bluetooth

Listas de redes e dispositivos usam cartões LVGL com ícone, sinal e metadados.
Informações, confirmações do BLE Suite e pacotes capturados usam painéis nativos.
SSH/Telnet têm saída rolável e comando separado; apresentador, teclado HID,
Karma e companheiro Wi-Fi receberam painéis. Os nomes de redes, endereços e
payloads são preservados. Os controles mantêm as ações dos módulos existentes.

Validação: 212 cenários e 652 toques simulados, três temas e duas orientações,
com ASan/UBSan. Isso valida apresentação e navegação; não valida transmissões
de rádio, dispositivos externos nem execução de ataques.

## Cobertura geral da interface

No Tab5, toda opção nativa de menu começa em um painel LVGL. A moldura
compartilhada, os títulos desconhecidos e os títulos dinâmicos também usam
painéis nativos, sem depender de uma lista de nomes. Desenhos legados não
podem sobrepor menus LVGL ativos. Informações capturadas são exibidas em até
48 cartões roláveis; as rotinas de rádio e de periféricos permanecem as mesmas.

LoRa, RF bruto, NRF24, Auto Clicker, infravermelho, ferramentas de fio, iButton,
WebUI e transferências usam os componentes temáticos. O terminal UART mantém
a entrada separada da saída e oferece rolagem. O analisador lógico exibe até
quatro canais, janela e cursor, preservando transições menores que uma coluna.
O navegador de arquivos mostra o caminho da pasta no cabeçalho nativo.

A suíte inclui 87 títulos, representantes dinâmicos, painéis de 48 campos,
três temas, duas orientações e pulsos curtos: 740 cenários e 1.174 toques.
O inventário de chamadas está em `artifacts/truce-ui/screen-inventory.json`.
Scripts JavaScript de terceiros continuam controlando seus próprios desenhos;
a interface de menus oferecida pelo firmware usa LVGL.

As telas próprias de ajustes, execução e encerramento do módulo BLE também
usam painéis e seletores LVGL, com confirmação explícita e ajuste selecionado
visível. A navegação mantém os comandos existentes do módulo.

## Capturas da emulação

A pasta `imagens` contém 748 PNGs e a galeria local `imagens/index.html`, com
busca e filtros de tema e orientação. A rodada de revisão ampliada passou em
812 cenários e 1.174 toques. Há 154 capturas de componentes, 72 exemplos das
telas revisadas e 522 capturas de estresse do painel compartilhado.

As imagens vêm do framebuffer real do LVGL, com dados sintéticos. Os exemplos
não executam os módulos de rádio nem os periféricos. O catálogo de 48 campos
valida o componente compartilhado com diferentes títulos. O manifesto registra
a origem e os hashes, e `capturas.txt` evita exportar arquivos de execuções antigas.

Após rodar a simulação, exporte novamente com:
`python3 tools/export_truce_images.py --source /tmp/truce-ui-host/screenshots`.

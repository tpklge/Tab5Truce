#!/usr/bin/env python3
"""Export the actual LVGL simulator framebuffer as PNG and a local gallery."""
from pathlib import Path
import argparse
import hashlib
import html
import json
import re
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
THEMES = ['grafite', 'oceano', 'claro']
LABELS = {
    'panel': ['Leitor infravermelho', 'TAG-O-MATIC', 'Rastreamento GPS'],
    'dial': ['Relógio', 'Temporizador', 'Rádio FM'],
    'discovery': ['Redes Wi-Fi', 'Dispositivos BLE'],
}
NAMES = {'home': 'Menu inicial', 'theme': 'Menu com muitos itens',
         'keyboard': 'Teclado', 'brightness': 'Brilho', 'operation': 'Operação',
         'document': 'Leitor de texto', 'spectrum': 'Espectro e cascata',
         'waveform': 'Forma de onda RF', 'media': 'Reprodutor de áudio',
         'megalodon': 'Megalodon', 'confirmation': 'Confirmação',
         'bluetooth-info': 'Informações Bluetooth', 'terminal': 'Terminal SSH',
         'logic': 'Analisador lógico', 'splash': 'Inicialização', 'storage': 'microSD ausente'}


def png(ppm):
    """Lossless format conversion only; pixels come from the LVGL flush callback."""
    with ppm.open('rb') as file:
        if file.readline().strip() != b'P6':
            raise ValueError(f'Not a P6 framebuffer: {ppm}')
        width, height = map(int, file.readline().split())
        if file.readline().strip() != b'255':
            raise ValueError('Expected 8-bit RGB')
        pixels = file.read()
    if len(pixels) != width * height * 3:
        raise ValueError(f'Incomplete framebuffer: {ppm}')
    def chunk(kind, data):
        return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind + data))
    stride = width * 3
    scanlines = b''.join(b'\0' + pixels[y * stride:(y + 1) * stride] for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('!2I5B', width, height, 8, 2, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(scanlines, 6)) + chunk(b'IEND', b'')), width, height


def describe(stem, titles):
    width, rest = stem.split('-', 1)
    orientation = 'horizontal' if width == '1280' else 'vertical'
    theme = 'grafite'
    category = 'telas'
    if rest.startswith('home-'):
        theme = dict(graphite='grafite', ocean='oceano', light='claro')[rest[5:]]
        title = NAMES['home']
    elif rest.startswith('catalog-'):
        _, index, number = rest.split('-')
        theme = THEMES[int(index)]
        title = titles[int(number)]
        category = 'catalogo-de-estresse'
    elif rest.startswith('review-'):
        _, index, name = rest.split('-', 2)
        theme = THEMES[int(index)]
        title = name.replace('-', ' ').title()
        category = 'revisao'
    else:
        match = re.fullmatch(r'(.+?)(?:-(\d+))?(?:-(\d+))?', rest)
        kind, index, number = match.groups()
        if kind in LABELS:
            theme = THEMES[int(index)]
            title = LABELS[kind][int(number)]
        else:
            title = NAMES[kind]
            if kind in {'theme', 'document', 'spectrum', 'waveform', 'media', 'megalodon',
                        'confirmation', 'bluetooth-info', 'terminal', 'logic'}:
                theme = THEMES[int(index)]
            if number is not None:
                title += f' — variante {number}'
            elif index is not None and kind in {'keyboard', 'operation'}:
                title += f' — variante {index}'
    return orientation, theme, category, title


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True, help='Fresh simulator PPM directory')
    parser.add_argument('--output', type=Path, default=ROOT / 'imagens')
    args = parser.parse_args()
    capture_index = args.source / 'capturas.txt'
    if not capture_index.is_file():
        raise SystemExit('Run the updated simulator first: capturas.txt is required to exclude stale frames.')
    files = sorted(args.source / name for name in capture_index.read_text().splitlines())
    if not files:
        raise SystemExit('Run the LVGL simulator first: no framebuffer captures found.')
    titles = json.loads((ROOT / 'artifacts/truce-ui/screen-inventory.json').read_text())['tested_titles']
    records = []
    for file in files:
        orientation, theme, category, title = describe(file.stem, titles)
        relative = Path(category) / theme / orientation / (file.stem + '.png')
        target = args.output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        data, width, height = png(file)
        target.write_bytes(data)
        records.append(dict(arquivo=relative.as_posix(), titulo=title, tema=theme,
                            orientacao=orientation, categoria=category, largura=width, altura=height,
                            sha256=hashlib.sha256(data).hexdigest()))
    manifest = dict(origem='Framebuffer RGB565 do mesmo ui_view.cpp e LVGL 9.3 do firmware',
                    dados='Sintéticos; sem conexões ou transmissões de rádio',
                    catalogo='Testes de estresse do painel compartilhado, não execução dos módulos',
                    codigo_ui_sha256=hashlib.sha256((ROOT / 'src/core/truce/ui_view.cpp').read_bytes()).hexdigest(),
                    total=len(records), imagens=records)
    (args.output / 'manifesto.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    cards = []
    for item in records:
        path = html.escape(item['arquivo'], quote=True)
        title = html.escape(item['titulo'])
        category = item['categoria']
        caption = f"{item['tema']} · {item['orientacao']} · {item['largura']}×{item['altura']}"
        cards.append(f'<article data-category="{category}" data-theme="{item["tema"]}" '
                     f'data-orientation="{item["orientacao"]}" data-search="{html.escape(item["titulo"].lower(), quote=True)}">'
                     f'<a href="{path}"><img loading="lazy" src="{path}" alt="{title}"></a>'
                     f'<h2>{title}</h2><p>{caption}</p></article>')
    page = '''<!doctype html><html lang="pt-BR"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Truce — imagens da emulação</title>
<style>*{box-sizing:border-box}body{margin:0;padding:24px;background:#edf1f7;color:#142238;font:16px system-ui}
header{max-width:1100px}h1{margin-top:0}p{line-height:1.5}nav{display:flex;gap:12px;flex-wrap:wrap;position:sticky;top:0;background:#edf1f7;padding:16px 0;z-index:1}
label{display:grid;gap:4px}input,select{font:inherit;padding:10px;border:1px solid #526176;border-radius:8px;max-width:100%}
main{display:grid;grid-template-columns:repeat(auto-fill,minmax(min(100%,340px),1fr));gap:20px}
article{background:white;border:1px solid #b7c2d3;border-radius:12px;overflow:hidden;align-self:start}
img{display:block;width:100%;height:270px;object-fit:contain;background:#f8fafc}h2{font-size:18px;margin:16px 16px 6px}article p{margin:0 16px 16px;color:#526176}
[hidden]{display:none!important}#count{margin:0 0 16px}</style>
<header><h1>Truce — imagens da emulação</h1><p>Capturas reais do framebuffer LVGL em 1280×720 e 720×1280. Dados de demonstração; nenhum ataque ou periférico foi executado.
O catálogo de estresse mostra o painel compartilhado com 48 campos sintéticos, sem executar os módulos. Clique numa imagem para abrir o PNG completo.</p></header>
<nav><label>Buscar tela<input id="search" type="search" placeholder="Wi-Fi, Bluetooth, LoRa…"></label>
<label>Conjunto<select id="category"><option value="principal">Telas e revisão</option value="telas">Telas</option><option value="revisao">Revisão</option><option value="catalogo-de-estresse">Catálogo de estresse</option><option value="">Todas</option></select></label>
<label>Tema<select id="theme"><option value="">Todos</option><option>grafite</option><option>oceano</option><option>claro</option></select></label>
<label>Orientação<select id="orientation"><option value="">Todas</option><option>horizontal</option><option>vertical</option></select></label></nav><p id="count" aria-live="polite"></p><main>'''
    page += '\n'.join(cards) + r'''</main><script>
const cards=[...document.querySelectorAll('article')];
function norm(s){return s.toLowerCase().normalize('NFD').replace(/[\u0300-\u036f]/g,'').replace(/[-\s]/g,'');}
function filter(){const q=norm(document.getElementById('search').value), c=document.getElementById('category').value,t=document.getElementById('theme').value,o=document.getElementById('orientation').value;let n=0;
for(const card of cards){const d=card.dataset;card.hidden=!(norm(d.search).includes(q)&&(!t||d.theme===t)&&(!o||d.orientation===o)&&(!c||(c==='principal'?d.category!=='catalogo-de-estresse':d.category===c)));if(!card.hidden)n++;}
document.getElementById('count').textContent=n+' imagens visíveis de '+cards.length;}
for(const input of document.querySelectorAll('input,select'))input.addEventListener('input',filter);filter();</script></html>'''
    (args.output / 'index.html').write_text(page)
    (args.output / 'LEIA-ME.txt').write_text(f'''Truce — capturas da emulação LVGL

Abra index.html para visualizar e filtrar as {len(records)} imagens PNG.
Organização: conjunto / tema / orientação / imagem.png.

Telas: cenários dos componentes nativos, incluindo entradas longas e estados-limite.
Revisão: exemplos legíveis das telas revisadas com dados sintéticos.
Catálogo de estresse: títulos dos módulos sobre o painel compartilhado de 48 campos.
Essas capturas não comprovam execução dos rádios ou dos periféricos físicos.
São pixels exportados do LVGL real; não são mockups desenhados à parte.

Para regenerar a partir de uma execução recente:
  cmake --build /tmp/truce-ui-host -j 6
  ctest --test-dir /tmp/truce-ui-host --output-on-failure
  python3 tools/export_truce_images.py --source /tmp/truce-ui-host/screenshots

manifesto.json registra títulos, dimensões, temas e SHA-256 dos PNGs.
''')
    print(f'{len(records)} PNGs; gallery: {args.output / "index.html"}')


if __name__ == '__main__':
    main()

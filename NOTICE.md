# Truce / Bruce — notices and modification history

## License and upstream attribution

Truce is a modified version of Bruce, based on
https://github.com/BruceDevices/Firmware at commit
`2b6ba4d4` (upstream version 1.17).

Bruce and this derived work are distributed under the **GNU Affero General
Public License, version 3 or any later version (AGPL-3.0-or-later)**, as declared
in the upstream `THIRD_PARTY.md`. The complete upstream license text is retained
unchanged in [LICENSE](LICENSE). Existing copyright, attribution and license
notices in the source tree are retained. See [README_BRUCE.md](README_BRUCE.md)
and [THIRD_PARTY.md](THIRD_PARTY.md) for upstream credits and dependency notices.
Third-party components retain their respective licenses; supplementary fonts
include their license in `sdcard/truce/fonts/OFL.txt`.

## Modified version — 2026-11-16

The Tab5Truce adaptation maintained at https://github.com/tpklge/Tab5Truce
changes the M5Stack Tab5 interface to LVGL, adds themed tile navigation and
native instrument/data views, redirects persistent application files to
`/truce` on microSD, adapts memory and input handling, and adds emulator tests,
image export tools, captures, and Portuguese/English documentation.

The source code, build configuration, submodule references and installation
instructions are published in this repository. Clone with `--recurse-submodules`
to retrieve the referenced dependencies. Build outputs are generated locally;
the application itself runs from flash, while its persistent data uses microSD.

This is a modified version, not an official Bruce release. The work is provided
without warranty as described in LICENSE. Redistribution and modification are
permitted under that license.

## Aviso em português

Truce é uma versão modificada do Bruce, distribuída sob a mesma licença
**AGPL-3.0-or-later**. O arquivo LICENSE foi preservado integralmente, assim como
os créditos e avisos dos autores originais e das dependências.

Alterações do Tab5Truce registradas em **16/11/2026**: interface LVGL com temas
e cartões, telas nativas, persistência no microSD em `/truce`, ajustes de memória
e entrada, testes simulados, capturas e documentação em pt-BR e inglês.

O código e as instruções de compilação estão disponíveis neste repositório.
Esta versão modificada não é uma distribuição oficial do Bruce e é fornecida
sem garantia, conforme a licença. Sua redistribuição e modificação são permitidas
nos termos da AGPL.

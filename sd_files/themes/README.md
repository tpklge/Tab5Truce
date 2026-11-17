# THEME pack

Themes are supposed to change the UI experience of the User by allowing you to choose:
* Different sets of images of the Main Menu
* If the UI will display borders or not
* If displaying the Labels on the Main Menu
* Setting the Primary Font color
* Setting the Secondary Font Color (Non selected submenu item)
* Setting the Background Color
* Setting the LED color and effects

## Images
Bruce accepts **.bmp** **.jpg** **.gif** and **.png** (not available on LITE_VERSION) to be used in the Main menu, but the bigger they are, greater will be the time to draw them on screen, **please, try to keep them small**.

The recommended height of the images is:

| Device | Display size | Height |
| --- | --- | --- |
| T-Embed | 320x170 | 140px |
| Cardputer StickCPlus | 240x135 | 105px |
| Core / CYD | 320x240 | 180 |

## Theme file
Theme settings are stored in a **.json** file, following this structure:
```
{
  "wifi":"wifi.png",
  "ble":"ble.png",
  "rf":"rf.png",
  "rfid":"rfid.png",
  "fm":"fm.png",
  "ir":"ir.png",
  "files":"files.png",
  "gps":"gps.png",
  "nrf":"nrf.png",
  "interpreter":"interpreter.png",
  "clock":"clock.png",
  "others":"others.png",
  "connect":"connect.png",
  "config":"config.png",
  "priColor":"ffff",
  "secColor":"aaaa",
  "bgColor":"0",
  "border":0,
  "label":0,
  "boot_img":"boot.gif",
  "boot_sound":"boot.wav",
  "ledBright": 100,
  "ledColor": "960064",
  "ledEffect": 0,
  "ledEffectSpeed": 3,
  "ledEffectDirection": 1
}
```
* Colors (excluding LED): Codes are in **RGB565**, 16bit pattern, so you need to convert it using [this tool](https://rgbcolorpicker.com/565).
* LED Colors: Codes are in HEX.
* border: 0 or 1, 1 to keep UI Borders
* label: 0 or 1, 1 to use UI Labels (Use 50px smaller images in this case)

## Theme creator
You can use [Bruce Theme Builder](https://bruce.computer/build_theme.html) to setup and prepare your images, settings and .json file, it will give you a .zip file that you **need to unzip** somewhere in your device, LittleFS or SD Card.

## Setting a Theme
Config > UI Theme > (Choose FS) > select the .json file and the theme will be set.



## Tab5 card grid

The Tab5 defaults to the grid on a fresh configuration. Existing configurations keep
their saved menu style; choose **Config > Display & UI > Menu Layout > Grid** to switch.

On M5GFX devices, grid cells load the same per-category theme paths as the carousel.
PNG (including transparency), JPG/JPEG and BMP are fitted proportionally into a bounded
canvas. Missing files, decode failures, allocation failures and unsupported formats
fall back to the built-in category icon. GIF animation remains available in the
carousel; the grid uses the built-in icon for GIF entries to keep navigation responsive.

For Tab5, use square PNG icons around 128 x 128 pixels. Transparent icons adapt to the
normal and selected card backgrounds. Labels remain visible in the grid for navigation.
The canvas uses at most 32 KiB per 128 x 128 icon and is released after drawing.
Card and status colors follow the existing primary, secondary and background colors.
The saved carousel option and the layouts of other boards remain available.

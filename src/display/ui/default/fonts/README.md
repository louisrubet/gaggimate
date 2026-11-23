# Standby haiku fonts

Montserrat Medium, 4 bpp, generated with [lv_font_conv](https://github.com/lvgl/lv_font_conv) from
`Montserrat-Medium.ttf` (https://github.com/JulietaUla/Montserrat, SIL OFL 1.1, see `Montserrat-OFL.txt`).

| File | Size | Used for |
|---|---|---|
| `standby_haiku_font_26.c` | 26 px | haiku verses |
| `standby_haiku_font_18.c` | 18 px | haiku author, and verses too wide at 26 px |

Covered characters: ASCII, Latin-1 (French accents, ç, «, », °...), ō Ō ū Ū (romanized Japanese: Bashō,
Hōryū-ji), œ Œ Ÿ, – —, ‘ ’ “ ”, … and €. A character outside this set is not drawn; a downloaded haiku that
uses one is not shown.

Regenerate after changing a size or the character set (SIZE = 26 or 18):

```shell
npx lv_font_conv@1.5.3 --bpp 4 --size SIZE --no-compress \
  --font Montserrat-Medium.ttf \
  --range 0x20-0x7E,0xA0-0xFF,0x14C-0x14D,0x152-0x153,0x16A-0x16B,0x178,0x2013-0x2014,0x2018-0x2019,0x201C-0x201D,0x2026,0x20AC \
  --format lvgl --lv-include lvgl.h -o standby_haiku_font_SIZE.c
```

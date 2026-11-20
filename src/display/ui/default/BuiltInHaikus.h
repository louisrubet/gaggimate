#ifndef BUILTINHAIKUS_H
#define BUILTINHAIKUS_H

#include <cstddef>

// Haikus built into the firmware, shown in order (one per standby entry) when the haiku of the day is off or
// unavailable. Same works as https://haiku.rubet.fr (github.com/louisrubet/haiku): Japanese originals in the
// public domain, French translations CC0.
// One line per verse ('\n'). Characters must be covered by standby_haiku_font_26 / _18 (see fonts/README.md).
struct BuiltInHaiku {
    const char *text;
    const char *author;
};

constexpr BuiltInHaiku BUILT_IN_HAIKUS[] = {
    {"Le vieil étang\nune grenouille plonge\nle bruit de l'eau", "Matsuo Bashō"},
    {"Fleurs de colza\nla lune à l'est\nle soleil à l'ouest", "Yosa Buson"},
    {"Ce monde de rosée\nn'est qu'un monde de rosée\net pourtant… et pourtant…", "Kobayashi Issa"},
    {"Herbes d'été\nc'est tout ce qui reste\ndes rêves des guerriers", "Matsuo Bashō"},
    {"Je croque un kaki\net la cloche résonne\nau temple Hōryū-ji", "Masaoka Shiki"},
    {"Malade en voyage\nmes rêves vagabondent encore\nsur la lande desséchée", "Matsuo Bashō"},
    {"Petit escargot\ngravis tout doucement\nle mont Fuji", "Kobayashi Issa"},
};

constexpr size_t BUILT_IN_HAIKU_COUNT = sizeof(BUILT_IN_HAIKUS) / sizeof(BUILT_IN_HAIKUS[0]);

#endif // BUILTINHAIKUS_H

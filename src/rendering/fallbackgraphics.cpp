#include "fallbackgraphics.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QDebug>
#include <QHash>
#include <QSet>

// 5x7 pixel font definitions (each character is 7 rows of 5-bit patterns)
// Bit order: MSB is leftmost pixel
struct GlyphData {
    uint8_t rows[7];
};

// Static pixel font data
static const QHash<QChar, GlyphData>& getPixelFont() {
    static QHash<QChar, GlyphData> font;
    static bool initialized = false;

    if (!initialized) {
        // Space
        font[' '] = {{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}};

        // Numbers
        font['0'] = {{0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}};
        font['1'] = {{0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}};
        font['2'] = {{0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111}};
        font['3'] = {{0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110}};
        font['4'] = {{0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}};
        font['5'] = {{0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}};
        font['6'] = {{0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}};
        font['7'] = {{0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}};
        font['8'] = {{0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}};
        font['9'] = {{0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}};

        // Uppercase letters
        font['A'] = {{0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}};
        font['B'] = {{0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}};
        font['C'] = {{0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110}};
        font['D'] = {{0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110}};
        font['E'] = {{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}};
        font['F'] = {{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}};
        font['G'] = {{0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110}};
        font['H'] = {{0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}};
        font['I'] = {{0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}};
        font['J'] = {{0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100}};
        font['K'] = {{0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}};
        font['L'] = {{0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}};
        font['M'] = {{0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}};
        font['N'] = {{0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001}};
        font['O'] = {{0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};
        font['P'] = {{0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}};
        font['Q'] = {{0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}};
        font['R'] = {{0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}};
        font['S'] = {{0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110}};
        font['T'] = {{0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}};
        font['U'] = {{0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};
        font['V'] = {{0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}};
        font['W'] = {{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001}};
        font['X'] = {{0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}};
        font['Y'] = {{0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}};
        font['Z'] = {{0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111}};

        // Lowercase letters
        font['a'] = {{0b00000, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111}};
        font['b'] = {{0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b10001, 0b11110}};
        font['c'] = {{0b00000, 0b00000, 0b01110, 0b10000, 0b10000, 0b10001, 0b01110}};
        font['d'] = {{0b00001, 0b00001, 0b01111, 0b10001, 0b10001, 0b10001, 0b01111}};
        font['e'] = {{0b00000, 0b00000, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110}};
        font['f'] = {{0b00110, 0b01000, 0b11110, 0b01000, 0b01000, 0b01000, 0b01000}};
        font['g'] = {{0b00000, 0b00000, 0b01111, 0b10001, 0b01111, 0b00001, 0b01110}};
        font['h'] = {{0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b10001, 0b10001}};
        font['i'] = {{0b00100, 0b00000, 0b01100, 0b00100, 0b00100, 0b00100, 0b01110}};
        font['j'] = {{0b00010, 0b00000, 0b00110, 0b00010, 0b00010, 0b10010, 0b01100}};
        font['k'] = {{0b10000, 0b10000, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010}};
        font['l'] = {{0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}};
        font['m'] = {{0b00000, 0b00000, 0b11010, 0b10101, 0b10101, 0b10101, 0b10101}};
        font['n'] = {{0b00000, 0b00000, 0b11110, 0b10001, 0b10001, 0b10001, 0b10001}};
        font['o'] = {{0b00000, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};
        font['p'] = {{0b00000, 0b00000, 0b11110, 0b10001, 0b11110, 0b10000, 0b10000}};
        font['q'] = {{0b00000, 0b00000, 0b01111, 0b10001, 0b01111, 0b00001, 0b00001}};
        font['r'] = {{0b00000, 0b00000, 0b10110, 0b11001, 0b10000, 0b10000, 0b10000}};
        font['s'] = {{0b00000, 0b00000, 0b01110, 0b10000, 0b01110, 0b00001, 0b11110}};
        font['t'] = {{0b01000, 0b01000, 0b11110, 0b01000, 0b01000, 0b01001, 0b00110}};
        font['u'] = {{0b00000, 0b00000, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101}};
        font['v'] = {{0b00000, 0b00000, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}};
        font['w'] = {{0b00000, 0b00000, 0b10001, 0b10001, 0b10101, 0b10101, 0b01010}};
        font['x'] = {{0b00000, 0b00000, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001}};
        font['y'] = {{0b00000, 0b00000, 0b10001, 0b10001, 0b01111, 0b00001, 0b01110}};
        font['z'] = {{0b00000, 0b00000, 0b11111, 0b00010, 0b00100, 0b01000, 0b11111}};

        // Basic punctuation
        font['!'] = {{0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100}};
        font['?'] = {{0b01110, 0b10001, 0b00010, 0b00100, 0b00100, 0b00000, 0b00100}};
        font['.'] = {{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100}};
        font[','] = {{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b01000}};
        font['-'] = {{0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000}};
        font['\''] = {{0b00100, 0b00100, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000}};
        font[':'] = {{0b00000, 0b00100, 0b00000, 0b00000, 0b00000, 0b00100, 0b00000}};
        font['/'] = {{0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000}};
        font['('] = {{0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010}};
        font[')'] = {{0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000}};
        font['%'] = {{0b11000, 0b11001, 0b00010, 0b00100, 0b01000, 0b10011, 0b00011}};
        font['+'] = {{0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000}};
        font['='] = {{0b00000, 0b00000, 0b11111, 0b00000, 0b11111, 0b00000, 0b00000}};
        font['*'] = {{0b00000, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00000}};
        font['&'] = {{0b01100, 0b10010, 0b10100, 0b01000, 0b10101, 0b10010, 0b01101}};
        font['$'] = {{0b00100, 0b01111, 0b10100, 0b01110, 0b00101, 0b11110, 0b00100}};

        // Curly quotes (Unicode)
        font[QChar(0x201c)] = {{0b01010, 0b10100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}};  // "
        font[QChar(0x201d)] = {{0b01010, 0b00101, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}};  // "
        font[QChar(0x2018)] = {{0b00100, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}};  // '
        font[QChar(0x2019)] = {{0b00100, 0b00010, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}};  // '
        font[QChar(0x2026)] = {{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b10101}};  // …
        font[QChar(0x00b7)] = {{0b00000, 0b00000, 0b00000, 0b00100, 0b00000, 0b00000, 0b00000}};  // ·

        // Spanish punctuation
        font[QChar(0x00bf)] = {{0b00100, 0b00000, 0b00100, 0b01000, 0b10001, 0b10001, 0b01110}};  // ¿
        font[QChar(0x00a1)] = {{0b00100, 0b00000, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}};  // ¡

        // Gender symbols
        font[QChar(0x2642)] = {{0b00011, 0b00101, 0b01110, 0b10100, 0b10100, 0b01000, 0b00000}};  // ♂
        font[QChar(0x2640)] = {{0b01110, 0b10001, 0b10001, 0b01110, 0b00100, 0b01110, 0b00100}};  // ♀

        // Arrow
        font[QChar(0x25b6)] = {{0b10000, 0b11000, 0b11100, 0b11110, 0b11100, 0b11000, 0b10000}};  // ▶

        // Ordinals
        font[QChar(0x00ba)] = {{0b01110, 0b10001, 0b10001, 0b01110, 0b00000, 0b00000, 0b00000}};  // º
        font[QChar(0x00aa)] = {{0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000, 0b00000}};  // ª

        // German sharp S
        font[QChar(0x00df)] = {{0b01110, 0b10001, 0b10010, 0b10100, 0b10010, 0b10001, 0b10110}};  // ß

        // Accented uppercase - grave
        font[QChar(0x00c0)] = {{0b00100, 0b00010, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001}};  // À
        font[QChar(0x00c8)] = {{0b00100, 0b00010, 0b11111, 0b10000, 0b11110, 0b10000, 0b11111}};  // È
        font[QChar(0x00cc)] = {{0b00100, 0b00010, 0b01110, 0b00100, 0b00100, 0b00100, 0b01110}};  // Ì
        font[QChar(0x00d2)] = {{0b00100, 0b00010, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // Ò
        font[QChar(0x00d9)] = {{0b00100, 0b00010, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};  // Ù

        // Accented uppercase - acute
        font[QChar(0x00c1)] = {{0b00010, 0b00100, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001}};  // Á
        font[QChar(0x00c9)] = {{0b00010, 0b00100, 0b11111, 0b10000, 0b11110, 0b10000, 0b11111}};  // É
        font[QChar(0x00cd)] = {{0b00010, 0b00100, 0b01110, 0b00100, 0b00100, 0b00100, 0b01110}};  // Í
        font[QChar(0x00d3)] = {{0b00010, 0b00100, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // Ó
        font[QChar(0x00da)] = {{0b00010, 0b00100, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};  // Ú

        // Accented uppercase - circumflex
        font[QChar(0x00c2)] = {{0b00100, 0b01010, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001}};  // Â
        font[QChar(0x00ca)] = {{0b00100, 0b01010, 0b11111, 0b10000, 0b11110, 0b10000, 0b11111}};  // Ê
        font[QChar(0x00ce)] = {{0b00100, 0b01010, 0b01110, 0b00100, 0b00100, 0b00100, 0b01110}};  // Î
        font[QChar(0x00d4)] = {{0b00100, 0b01010, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // Ô
        font[QChar(0x00db)] = {{0b00100, 0b01010, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};  // Û

        // Accented uppercase - diaeresis/umlaut
        font[QChar(0x00cb)] = {{0b01010, 0b00000, 0b11111, 0b10000, 0b11110, 0b10000, 0b11111}};  // Ë
        font[QChar(0x00cf)] = {{0b01010, 0b00000, 0b01110, 0b00100, 0b00100, 0b00100, 0b01110}};  // Ï
        font[QChar(0x00c4)] = {{0b01010, 0b00000, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001}};  // Ä
        font[QChar(0x00d6)] = {{0b01010, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // Ö
        font[QChar(0x00dc)] = {{0b01010, 0b00000, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};  // Ü

        // Other accented uppercase
        font[QChar(0x00c7)] = {{0b01110, 0b10001, 0b10000, 0b10000, 0b10001, 0b01110, 0b00100}};  // Ç
        font[QChar(0x00d1)] = {{0b01010, 0b10100, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001}};  // Ñ
        font[QChar(0x0152)] = {{0b01111, 0b10100, 0b10100, 0b10111, 0b10100, 0b10100, 0b01111}};  // Œ

        // Accented lowercase - grave
        font[QChar(0x00e0)] = {{0b00100, 0b00010, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111}};  // à
        font[QChar(0x00e8)] = {{0b00100, 0b00010, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110}};  // è
        font[QChar(0x00ec)] = {{0b00100, 0b00010, 0b00000, 0b01100, 0b00100, 0b00100, 0b01110}};  // ì
        font[QChar(0x00f2)] = {{0b00100, 0b00010, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // ò
        font[QChar(0x00f9)] = {{0b00100, 0b00010, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101}};  // ù

        // Accented lowercase - acute
        font[QChar(0x00e1)] = {{0b00010, 0b00100, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111}};  // á
        font[QChar(0x00e9)] = {{0b00010, 0b00100, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110}};  // é
        font[QChar(0x00ed)] = {{0b00010, 0b00100, 0b00000, 0b01100, 0b00100, 0b00100, 0b01110}};  // í
        font[QChar(0x00f3)] = {{0b00010, 0b00100, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // ó
        font[QChar(0x00fa)] = {{0b00010, 0b00100, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101}};  // ú

        // Accented lowercase - circumflex
        font[QChar(0x00ea)] = {{0b00100, 0b01010, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110}};  // ê
        font[QChar(0x00ee)] = {{0b00100, 0b01010, 0b00000, 0b01100, 0b00100, 0b00100, 0b01110}};  // î
        font[QChar(0x00f4)] = {{0b00100, 0b01010, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // ô
        font[QChar(0x00fb)] = {{0b00100, 0b01010, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101}};  // û

        // Accented lowercase - diaeresis/umlaut
        font[QChar(0x00eb)] = {{0b01010, 0b00000, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110}};  // ë
        font[QChar(0x00ef)] = {{0b01010, 0b00000, 0b01100, 0b00100, 0b00100, 0b00100, 0b01110}};  // ï
        font[QChar(0x00e4)] = {{0b01010, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111}};  // ä
        font[QChar(0x00f6)] = {{0b01010, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110}};  // ö
        font[QChar(0x00fc)] = {{0b01010, 0b00000, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101}};  // ü

        // Other accented lowercase
        font[QChar(0x00e7)] = {{0b00000, 0b00000, 0b01110, 0b10000, 0b10001, 0b01110, 0b00100}};  // ç
        font[QChar(0x00f1)] = {{0b01010, 0b10100, 0b11110, 0b10001, 0b10001, 0b10001, 0b10001}};  // ñ
        font[QChar(0x0153)] = {{0b00000, 0b00000, 0b01011, 0b10101, 0b10111, 0b10100, 0b01011}};  // œ

        initialized = true;
    }

    return font;
}

// Character position mappings (Gen3 font positions)
static const QHash<QChar, int>& getCharPositions() {
    static QHash<QChar, int> positions;
    static bool initialized = false;

    if (!initialized) {
        // Space
        positions[' '] = 0;

        // Accented uppercase at low positions
        positions[QChar(0x00c0)] = 2;   // À
        positions[QChar(0x00c1)] = 4;   // Á
        positions[QChar(0x00c2)] = 6;   // Â
        positions[QChar(0x00c7)] = 8;   // Ç
        positions[QChar(0x00c8)] = 10;  // È
        positions[QChar(0x00c9)] = 12;  // É
        positions[QChar(0x00ca)] = 14;  // Ê
        positions[QChar(0x00cb)] = 16;  // Ë
        positions[QChar(0x00cc)] = 18;  // Ì
        positions[QChar(0x00ce)] = 22;  // Î
        positions[QChar(0x00cf)] = 24;  // Ï
        positions[QChar(0x00d2)] = 26;  // Ò
        positions[QChar(0x00d3)] = 28;  // Ó
        positions[QChar(0x00d4)] = 30;  // Ô
        positions[QChar(0x0152)] = 32;  // Œ
        positions[QChar(0x00d9)] = 34;  // Ù
        positions[QChar(0x00da)] = 36;  // Ú
        positions[QChar(0x00db)] = 38;  // Û
        positions[QChar(0x00d1)] = 40;  // Ñ
        positions[QChar(0x00df)] = 42;  // ß

        // Accented lowercase
        positions[QChar(0x00e0)] = 44;  // à
        positions[QChar(0x00e1)] = 46;  // á
        positions[QChar(0x00e7)] = 50;  // ç
        positions[QChar(0x00e8)] = 52;  // è
        positions[QChar(0x00e9)] = 54;  // é
        positions[QChar(0x00ea)] = 56;  // ê
        positions[QChar(0x00eb)] = 58;  // ë
        positions[QChar(0x00ec)] = 60;  // ì
        positions[QChar(0x00ee)] = 64;  // î
        positions[QChar(0x00ef)] = 66;  // ï
        positions[QChar(0x00f2)] = 68;  // ò
        positions[QChar(0x00f3)] = 70;  // ó
        positions[QChar(0x00f4)] = 72;  // ô
        positions[QChar(0x0153)] = 74;  // œ
        positions[QChar(0x00f9)] = 76;  // ù
        positions[QChar(0x00fa)] = 78;  // ú
        positions[QChar(0x00fb)] = 80;  // û
        positions[QChar(0x00f1)] = 82;  // ñ

        // Ordinals
        positions[QChar(0x00ba)] = 84;  // º
        positions[QChar(0x00aa)] = 86;  // ª

        // Symbols
        positions['&'] = 90;
        positions['+'] = 92;
        positions['='] = 106;

        // Spanish punctuation
        positions[QChar(0x00bf)] = 162; // ¿
        positions[QChar(0x00a1)] = 164; // ¡

        // Other punctuation/symbols
        positions[QChar(0x00ed)] = 180; // í
        positions['%'] = 182;
        positions['('] = 184;
        positions[')'] = 186;

        // Numbers
        positions['0'] = 322;
        positions['1'] = 324;
        positions['2'] = 326;
        positions['3'] = 328;
        positions['4'] = 330;
        positions['5'] = 332;
        positions['6'] = 334;
        positions['7'] = 336;
        positions['8'] = 338;
        positions['9'] = 340;

        // Basic punctuation
        positions['!'] = 342;
        positions['?'] = 344;
        positions['.'] = 346;
        positions['-'] = 348;
        positions[QChar(0x00b7)] = 350; // ·
        positions[QChar(0x2026)] = 352; // …
        positions[QChar(0x201c)] = 354; // "
        positions[QChar(0x201d)] = 356; // "
        positions[QChar(0x2018)] = 358; // '
        positions[QChar(0x2019)] = 360; // '

        // Symbols
        positions[QChar(0x2642)] = 362; // ♂
        positions[QChar(0x2640)] = 364; // ♀
        positions['$'] = 366;
        positions[','] = 368;
        positions['*'] = 370;
        positions['/'] = 372;

        // Uppercase A-Z (374-424)
        for (int i = 0; i < 26; i++) {
            positions[QChar('A' + i)] = 374 + i * 2;
        }

        // Lowercase a-z (426-476)
        for (int i = 0; i < 26; i++) {
            positions[QChar('a' + i)] = 426 + i * 2;
        }

        // More symbols and accented
        positions[QChar(0x25b6)] = 478; // ▶
        positions[':'] = 480;
        positions[QChar(0x00c4)] = 482; // Ä
        positions[QChar(0x00d6)] = 484; // Ö
        positions[QChar(0x00dc)] = 486; // Ü
        positions[QChar(0x00e4)] = 488; // ä
        positions[QChar(0x00f6)] = 490; // ö
        positions[QChar(0x00fc)] = 492; // ü

        initialized = true;
    }

    return positions;
}

// Draw a single glyph onto the font sheet
static void drawGlyph(QImage &img, QChar ch, int position, int sheetWidth, int sheetHeight) {
    const int GLYPH_WIDTH = 8;
    const int GLYPH_HEIGHT = 16;
    const int CHARS_PER_ROW = 32;
    const int COLOR_TRANSPARENT = 0;
    const int COLOR_SHADOW = 1;
    const int COLOR_WHITE = 2;

    const auto& font = getPixelFont();
    if (!font.contains(ch)) {
        return;
    }

    const GlyphData& glyph = font[ch];

    int col = position % CHARS_PER_ROW;
    int row = position / CHARS_PER_ROW;
    int baseX = col * GLYPH_WIDTH;
    int baseY = row * GLYPH_HEIGHT;

    // Offset to center the 5x7 glyph in the 8x16 cell
    int offsetX = 1;
    int offsetY = 4;

    // First pass: collect glyph pixels and draw white
    QSet<QPair<int,int>> glyphPixels;
    for (int gy = 0; gy < 7; gy++) {
        uint8_t rowBits = glyph.rows[gy];
        for (int gx = 0; gx < 5; gx++) {
            if (rowBits & (0b10000 >> gx)) {
                int px = baseX + offsetX + gx;
                int py = baseY + offsetY + gy;
                glyphPixels.insert(qMakePair(px, py));
                img.setPixel(px, py, COLOR_WHITE);
            }
        }
    }

    // Second pass: add outline/shadow
    for (const auto& pixel : glyphPixels) {
        int px = pixel.first;
        int py = pixel.second;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int nx = px + dx;
                int ny = py + dy;
                if (!glyphPixels.contains(qMakePair(nx, ny))) {
                    if (nx >= 0 && nx < sheetWidth && ny >= 0 && ny < sheetHeight) {
                        if (img.pixelIndex(nx, ny) == COLOR_TRANSPARENT) {
                            img.setPixel(nx, ny, COLOR_SHADOW);
                        }
                    }
                }
            }
        }
    }
}

QImage FallbackGraphics::generatePlaceholderFont()
{
    const int SHEET_WIDTH = CHARS_PER_ROW * GLYPH_WIDTH;   // 256
    const int SHEET_HEIGHT = 16 * GLYPH_HEIGHT;            // 256

    // Create indexed image
    QImage fontSheet(SHEET_WIDTH, SHEET_HEIGHT, QImage::Format_Indexed8);

    // Set palette: 0=transparent, 1=shadow, 2=white
    QVector<QRgb> palette;
    palette << qRgba(0, 0, 0, 0)      // 0: Transparent
            << qRgb(48, 48, 48)       // 1: Dark gray (shadow)
            << qRgb(255, 255, 255)    // 2: White (main glyph)
            << qRgb(128, 128, 128);   // 3: Unused

    // Fill rest of palette
    for (int i = palette.size(); i < 256; i++) {
        palette << qRgb(0, 0, 0);
    }
    fontSheet.setColorTable(palette);
    fontSheet.fill(0);

    // Draw all characters
    const auto& positions = getCharPositions();
    for (auto it = positions.begin(); it != positions.end(); ++it) {
        drawGlyph(fontSheet, it.key(), it.value(), SHEET_WIDTH, SHEET_HEIGHT);
    }

    qDebug() << "Generated fallback font:" << SHEET_WIDTH << "x" << SHEET_HEIGHT
             << "with" << positions.size() << "characters";

    return fontSheet;
}

QByteArray FallbackGraphics::generateDefaultGlyphWidths()
{
    // Width indices are pos/2, need enough for max position ~492
    // Use uniform 6px width for all characters for clean fallback rendering
    QByteArray widths(256, 6);

    // Space is narrower
    widths[0] = 4;

    return widths;
}

QImage FallbackGraphics::generatePlaceholderPokemonIcon(int index)
{
    // Create RGB32 image first for clean drawing, convert to indexed after
    QImage icon(ICON_WIDTH, ICON_HEIGHT, QImage::Format_ARGB32);
    icon.fill(Qt::transparent);

    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw gray oval
    painter.setPen(QPen(QColor(48, 48, 48), 1));  // Dark outline
    painter.setBrush(QColor(128, 128, 128));      // Gray fill
    painter.drawEllipse(2, 4, 28, 24);            // Oval shape

    // Draw index number
    painter.setRenderHint(QPainter::TextAntialiasing, false);
    QString indexStr = QString::number(index);

    // Choose font size based on number of digits
    int fontSize = 10;
    if (index >= 100) {
        fontSize = 8;
    } else if (index >= 10) {
        fontSize = 9;
    }

    QFont font("Arial", fontSize, QFont::Bold);
    painter.setFont(font);

    // Draw text shadow
    painter.setPen(QColor(32, 32, 32));
    painter.drawText(QRect(1, 1, 32, 32), Qt::AlignCenter, indexStr);

    // Draw text
    painter.setPen(Qt::white);
    painter.drawText(QRect(0, 0, 32, 32), Qt::AlignCenter, indexStr);

    painter.end();

    // Convert to indexed format
    QImage indexed(ICON_WIDTH, ICON_HEIGHT, QImage::Format_Indexed8);
    QVector<QRgb> palette;
    palette << qRgba(0, 0, 0, 0)       // 0: Transparent
            << qRgb(32, 32, 32)        // 1: Dark outline
            << qRgb(48, 48, 48)        // 2: Darker gray
            << qRgb(64, 64, 64)        // 3: Dark gray
            << qRgb(80, 80, 80)        // 4: Medium-dark gray
            << qRgb(96, 96, 96)        // 5: Medium gray
            << qRgb(112, 112, 112)     // 6: Gray
            << qRgb(128, 128, 128)     // 7: Mid gray
            << qRgb(144, 144, 144)     // 8: Light-mid gray
            << qRgb(160, 160, 160)     // 9: Light gray
            << qRgb(176, 176, 176)     // 10: Lighter gray
            << qRgb(192, 192, 192)     // 11: Very light gray
            << qRgb(208, 208, 208)     // 12: Near white
            << qRgb(224, 224, 224)     // 13: Almost white
            << qRgb(240, 240, 240)     // 14: Off-white
            << qRgb(255, 255, 255);    // 15: White

    indexed.setColorTable(palette);

    // Convert pixels
    for (int y = 0; y < ICON_HEIGHT; y++) {
        for (int x = 0; x < ICON_WIDTH; x++) {
            QRgb pixel = icon.pixel(x, y);
            int alpha = qAlpha(pixel);

            if (alpha < 128) {
                indexed.setPixel(x, y, 0);  // Transparent
            } else {
                // Map grayscale to palette index
                int gray = qGray(pixel);
                int paletteIndex = (gray * 15) / 255;
                if (paletteIndex < 1) paletteIndex = 1;
                if (paletteIndex > 15) paletteIndex = 15;
                indexed.setPixel(x, y, paletteIndex);
            }
        }
    }

    return indexed;
}

QImage FallbackGraphics::generatePlaceholderBackground(int index)
{
    QImage background(BG_WIDTH, BG_HEIGHT, QImage::Format_RGB32);
    QPainter painter(&background);

    // Color schemes based on index (similar to actual Wonder Card backgrounds)
    QColor baseColor, lightColor, darkColor, accentColor;
    switch (index % 8) {
        case 0: // Blue (default)
            baseColor = QColor(72, 96, 144);
            lightColor = QColor(112, 136, 184);
            darkColor = QColor(48, 64, 104);
            accentColor = QColor(200, 184, 96);
            break;
        case 1: // Green
            baseColor = QColor(72, 128, 96);
            lightColor = QColor(112, 168, 136);
            darkColor = QColor(48, 88, 64);
            accentColor = QColor(200, 200, 96);
            break;
        case 2: // Red
            baseColor = QColor(144, 72, 72);
            lightColor = QColor(184, 112, 112);
            darkColor = QColor(104, 48, 48);
            accentColor = QColor(248, 208, 96);
            break;
        case 3: // Purple
            baseColor = QColor(112, 72, 144);
            lightColor = QColor(152, 112, 184);
            darkColor = QColor(72, 48, 104);
            accentColor = QColor(200, 168, 216);
            break;
        case 4: // Yellow
            baseColor = QColor(168, 152, 72);
            lightColor = QColor(208, 192, 112);
            darkColor = QColor(128, 112, 48);
            accentColor = QColor(248, 240, 168);
            break;
        case 5: // Cyan
            baseColor = QColor(72, 136, 152);
            lightColor = QColor(112, 176, 192);
            darkColor = QColor(48, 96, 112);
            accentColor = QColor(200, 232, 240);
            break;
        case 6: // Orange
            baseColor = QColor(176, 112, 56);
            lightColor = QColor(216, 152, 96);
            darkColor = QColor(136, 72, 32);
            accentColor = QColor(248, 216, 152);
            break;
        default: // Gray
            baseColor = QColor(104, 104, 112);
            lightColor = QColor(144, 144, 152);
            darkColor = QColor(64, 64, 72);
            accentColor = QColor(200, 200, 208);
            break;
    }

    // Fill base background
    painter.fillRect(background.rect(), baseColor);

    // Header area (top section with gradient)
    QLinearGradient headerGrad(0, 0, 0, 40);
    headerGrad.setColorAt(0.0, lightColor);
    headerGrad.setColorAt(1.0, baseColor);
    painter.fillRect(0, 0, BG_WIDTH, 40, headerGrad);

    // Header separator line
    painter.setPen(QPen(darkColor, 2));
    painter.drawLine(4, 40, BG_WIDTH - 4, 40);
    painter.setPen(QPen(lightColor, 1));
    painter.drawLine(4, 41, BG_WIDTH - 4, 41);

    // Body area (subtle texture pattern)
    painter.setPen(QPen(darkColor.lighter(110), 1));
    for (int y = 44; y < 115; y += 8) {
        for (int x = 4; x < BG_WIDTH - 4; x += 8) {
            painter.drawPoint(x, y);
        }
    }

    // Footer separator line
    painter.setPen(QPen(darkColor, 2));
    painter.drawLine(4, 115, BG_WIDTH - 4, 115);
    painter.setPen(QPen(lightColor, 1));
    painter.drawLine(4, 116, BG_WIDTH - 4, 116);

    // Footer area (bottom gradient)
    QLinearGradient footerGrad(0, 118, 0, BG_HEIGHT);
    footerGrad.setColorAt(0.0, baseColor);
    footerGrad.setColorAt(1.0, darkColor);
    painter.fillRect(0, 118, BG_WIDTH, BG_HEIGHT - 118, footerGrad);

    // Outer border
    painter.setPen(QPen(darkColor, 2));
    painter.drawRect(1, 1, BG_WIDTH - 3, BG_HEIGHT - 3);

    // Inner highlight border
    painter.setPen(QPen(lightColor.lighter(120), 1));
    painter.drawRect(3, 3, BG_WIDTH - 7, BG_HEIGHT - 7);

    // Corner accents
    painter.setPen(QPen(accentColor, 1));
    // Top-left
    painter.drawLine(4, 4, 12, 4);
    painter.drawLine(4, 4, 4, 12);
    // Top-right
    painter.drawLine(BG_WIDTH - 13, 4, BG_WIDTH - 5, 4);
    painter.drawLine(BG_WIDTH - 5, 4, BG_WIDTH - 5, 12);
    // Bottom-left
    painter.drawLine(4, BG_HEIGHT - 13, 4, BG_HEIGHT - 5);
    painter.drawLine(4, BG_HEIGHT - 5, 12, BG_HEIGHT - 5);
    // Bottom-right
    painter.drawLine(BG_WIDTH - 5, BG_HEIGHT - 13, BG_WIDTH - 5, BG_HEIGHT - 5);
    painter.drawLine(BG_WIDTH - 13, BG_HEIGHT - 5, BG_WIDTH - 5, BG_HEIGHT - 5);

    // Icon area placeholder (top-right)
    painter.setPen(QPen(darkColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(BG_WIDTH - 40, 4, 36, 36);

    painter.end();

    return background;
}

QVector<QColor> FallbackGraphics::getDefaultTextPalette()
{
    QVector<QColor> palette;

    // Standard text palette (similar to Gen3 stdpal_3)
    palette << QColor(0, 0, 0, 0)       // 0: Transparent
            << QColor(64, 64, 64)       // 1: Dark shadow
            << QColor(255, 255, 255)    // 2: White (main text)
            << QColor(128, 128, 128)    // 3: Gray (secondary)
            << QColor(80, 80, 80)       // 4: Dark gray
            << QColor(200, 200, 200)    // 5: Light gray
            << QColor(160, 160, 160)    // 6: Medium gray
            << QColor(240, 240, 240);   // 7: Near white

    // Fill remaining slots
    for (int i = palette.size(); i < 16; i++) {
        palette << QColor(i * 16, i * 16, i * 16);
    }

    return palette;
}

QVector<QColor> FallbackGraphics::getDefaultIconPalette()
{
    QVector<QColor> palette;

    // Generic Pokemon icon palette
    palette << QColor(0, 0, 0, 0)       // 0: Transparent
            << QColor(16, 16, 16)       // 1: Black outline
            << QColor(248, 248, 248)    // 2: White
            << QColor(168, 168, 168)    // 3: Light gray
            << QColor(104, 104, 104)    // 4: Medium gray
            << QColor(64, 64, 64)       // 5: Dark gray
            << QColor(200, 48, 48)      // 6: Red
            << QColor(248, 176, 176)    // 7: Light red
            << QColor(48, 80, 200)      // 8: Blue
            << QColor(176, 192, 248)    // 9: Light blue
            << QColor(248, 208, 48)     // 10: Yellow
            << QColor(248, 232, 168)    // 11: Light yellow
            << QColor(48, 168, 72)      // 12: Green
            << QColor(168, 232, 176)    // 13: Light green
            << QColor(168, 88, 48)      // 14: Brown
            << QColor(232, 184, 136);   // 15: Tan

    return palette;
}

QVector<QColor> FallbackGraphics::getDefaultBackgroundPalette()
{
    QVector<QColor> palette;

    // Wonder Card background palette
    palette << QColor(0, 0, 0, 0)       // 0: Transparent
            << QColor(40, 56, 88)       // 1: Dark blue
            << QColor(56, 80, 120)      // 2: Medium blue
            << QColor(72, 96, 144)      // 3: Blue
            << QColor(88, 112, 160)     // 4: Light blue
            << QColor(104, 128, 176)    // 5: Lighter blue
            << QColor(120, 144, 192)    // 6: Even lighter
            << QColor(136, 160, 208)    // 7: Very light blue
            << QColor(248, 248, 248)    // 8: White
            << QColor(200, 208, 224)    // 9: Light gray-blue
            << QColor(168, 176, 200)    // 10: Gray-blue
            << QColor(248, 224, 48)     // 11: Gold
            << QColor(200, 176, 32)     // 12: Dark gold
            << QColor(152, 128, 24)     // 13: Bronze
            << QColor(80, 80, 80)       // 14: Dark gray
            << QColor(32, 32, 32);      // 15: Near black

    return palette;
}

QImage FallbackGraphics::convertToIndexed(const QImage &source, int numColors)
{
    QImage indexed(source.width(), source.height(), QImage::Format_Indexed8);

    // Simple color quantization
    QVector<QRgb> palette;
    for (int i = 0; i < numColors; i++) {
        int gray = (255 * i) / (numColors - 1);
        palette << qRgb(gray, gray, gray);
    }
    indexed.setColorTable(palette);

    for (int y = 0; y < source.height(); y++) {
        for (int x = 0; x < source.width(); x++) {
            QRgb pixel = source.pixel(x, y);
            int gray = qGray(pixel);
            int index = (gray * (numColors - 1)) / 255;
            indexed.setPixel(x, y, index);
        }
    }

    return indexed;
}

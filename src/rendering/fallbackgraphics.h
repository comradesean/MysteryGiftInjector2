#ifndef FALLBACKGRAPHICS_H
#define FALLBACKGRAPHICS_H

#include <QImage>
#include <QColor>
#include <QByteArray>
#include <QVector>

class FallbackGraphics
{
public:
    // Font constants matching GBA format (must match Gen3FontRenderer)
    static constexpr int GLYPH_WIDTH = 8;
    static constexpr int GLYPH_HEIGHT = 16;
    static constexpr int CHARS_PER_ROW = 32;  // Match Gen3FontRenderer
    static constexpr int TOTAL_CHARS = 512;   // Need more for Gen3 positions

    // Pokemon icon size
    static constexpr int ICON_WIDTH = 32;
    static constexpr int ICON_HEIGHT = 32;

    // Wonder Card background size (240x160 scaled 2x)
    static constexpr int BG_WIDTH = 240;
    static constexpr int BG_HEIGHT = 160;

    // Generate a simple bitmap font using Qt's built-in rendering
    // Returns an indexed image with 4 colors (matching 2bpp format)
    static QImage generatePlaceholderFont();

    // Generate default glyph widths (fixed width for most chars)
    // Returns 256 bytes, one per character
    static QByteArray generateDefaultGlyphWidths();

    // Generate a placeholder Pokemon icon (gray oval with index number)
    // Index 0-412 for species numbers
    static QImage generatePlaceholderPokemonIcon(int index = 0);

    // Generate a simple Wonder Card background
    // Index 0-7 for different card types (all same for fallback)
    static QImage generatePlaceholderBackground(int index = 0);

    // Default 16-color palette for text rendering
    static QVector<QColor> getDefaultTextPalette();

    // Default 16-color palette for Pokemon icons
    static QVector<QColor> getDefaultIconPalette();

    // Default 16-color palette for Wonder Card background
    static QVector<QColor> getDefaultBackgroundPalette();

private:
    // Helper to render a character to the font image
    static void renderCharToFontSheet(QImage &fontSheet, int charIndex, QChar character);

    // Helper to create indexed image from grayscale
    static QImage convertToIndexed(const QImage &source, int numColors);
};

#endif // FALLBACKGRAPHICS_H

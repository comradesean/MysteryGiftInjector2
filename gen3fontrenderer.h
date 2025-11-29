#ifndef GEN3FONTRENDERER_H
#define GEN3FONTRENDERER_H

#include <QImage>
#include <QPixmap>
#include <QMap>
#include <QVector>
#include <QString>
#include <QColor>
#include <cstdint>
#include "gbaromreader.h"

/**
 * @brief Renders text using ROM-extracted Gen3 Pokemon font
 *
 * This class replicates the Python WonderCardRenderer functionality in C++/Qt.
 * It uses ROM-extracted 2bpp font glyphs and glyph width tables for
 * pixel-accurate text rendering matching the original GBA game.
 *
 * Character mappings are loaded from JSON resource files:
 * - FONT_NORMAL_COPY_2: FireRed/LeafGreen (font_normal_copy_2_latin.json)
 * - FONT_SHORT_COPY_1: Emerald (font_short_copy_1_latin.json)
 *
 * Text color tables from mystery_gift_show_card.c:
 * - Color table 0 [0, 2, 3]: for body/footer text
 * - Color table 1 [0, 1, 2]: for title/header text
 */
class Gen3FontRenderer
{
public:
    // Font type enumeration for character mapping selection
    enum FontType {
        FontNormalCopy2,   // FRLG: FONT_NORMAL_COPY_2
        FontShortCopy1     // Emerald: FONT_SHORT_COPY_1
    };

    Gen3FontRenderer();
    ~Gen3FontRenderer();

    // Initialize from ROM
    bool loadFromROM(GBAROReader *reader, QString &errorMessage);
    bool isLoaded() const { return m_loaded; }

    // Emerald-specific support
    bool isEmerald() const { return m_isEmerald; }
    bool hasIdFont() const { return !m_idFontSheet.isNull(); }

    // Character mapping
    bool loadCharacterMappingFromJson(FontType fontType);
    bool loadCharacterMappingFromJson(const QString &resourcePath);
    int getCharPosition(QChar ch) const;
    bool canEncodeChar(QChar ch) const;
    int getEncodedLength(const QString &text) const;

    // Text color schemes (from mystery_gift_show_card.c sTextColorTable)
    enum ColorScheme {
        BodyFooter = 0,  // [0, 2, 3] - used for body and footer text
        TitleHeader = 1  // [0, 1, 2] - used for title/header text
    };

    // Create colored font image using stdpal_3 and color table
    QImage createColoredFont(ColorScheme scheme);

    // Create colored ID font for Emerald (uses FONT_NORMAL instead of FONT_SHORT)
    QImage createColoredIdFont(ColorScheme scheme);

    // Get glyph width for a character
    int getCharWidth(QChar ch) const;

    // Get glyph width for ID font (Emerald only, falls back to main font otherwise)
    int getIdCharWidth(QChar ch) const;

    // Measure text dimensions
    QSize measureText(const QString &text, int charSpacing = 0) const;

    // Extract a single character glyph from the font sheet
    QImage getCharacter(QChar ch, const QImage &coloredFont) const;

    // Render a single line of text
    QImage renderLine(const QString &text, const QImage &coloredFont, int charSpacing = 0);

    // Render multi-line text area
    QImage renderTextArea(const QStringList &lines, const QImage &coloredFont,
                          int width, int height,
                          Qt::Alignment alignment = Qt::AlignLeft,
                          int lineSpacing = 2, int charSpacing = 0,
                          int paddingLeft = 0, int paddingTop = 0);

    // Get text palette (stdpal_3) from ROM
    QVector<QRgb> getTextPalette() const { return m_textPalette; }

    // Set fallback glyph widths (when ROM is not available)
    void setFallbackGlyphWidths(const QByteArray &widths);

    // Font constants
    static const int CHAR_WIDTH = 8;
    static const int CHAR_HEIGHT = 16;
    static const int RENDER_HEIGHT = 14;  // Render height reduced by 2px
    static const int CHARS_PER_ROW = 32;

private:
    bool m_loaded;
    bool m_isEmerald;

    // ROM data
    GBAROReader *m_romReader;

    // Main font sheet (2bpp indexed)
    // FRLG: FONT_NORMAL, Emerald: FONT_SHORT
    QImage m_fontSheet;

    // Glyph widths from ROM (for main font)
    QVector<uint8_t> m_glyphWidths;

    // ID font sheet for Emerald (FONT_NORMAL - only used for ID number)
    QImage m_idFontSheet;

    // ID font glyph widths (Emerald only)
    QVector<uint8_t> m_idGlyphWidths;

    // Text palette (stdpal_3)
    QVector<QRgb> m_textPalette;

    // Character mapping: Unicode char -> font sheet position
    QMap<QChar, int> m_charToPos;

    // Text color tables: [background, foreground, shadow]
    static const int TEXT_COLOR_TABLE_0[3];  // Body/footer: [0, 2, 3]
    static const int TEXT_COLOR_TABLE_1[3];  // Title/header: [0, 1, 2]

    // Apply palette remapping to font
    QImage applyPaletteToFont(const int colorTable[3]);

    // Current font type
    FontType m_fontType;
};

#endif // GEN3FONTRENDERER_H

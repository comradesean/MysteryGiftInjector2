/**
 * @file gbaromreader.h
 * @brief GBA ROM reader for extracting Pokemon Generation III graphics and data.
 *
 * This file provides the GBAROReader class which handles all ROM access for the
 * Mystery Gift Injector application. It supports:
 *
 * ## Graphics Extraction
 * - **4bpp tiles**: Standard GBA tile format (32 bytes per 8x8 tile)
 * - **2bpp tiles**: Font tiles (16 bytes per 8x8 tile)
 * - **LZ77 decompression**: GBA standard compression for sprites
 * - **Wonder Card backgrounds**: 8 different event card backgrounds
 * - **Pokemon icons**: 32x32 sprite icons for all Pokemon
 *
 * ## Palette Handling
 * - BGR555 to RGB conversion (GBA uses 15-bit color)
 * - Palette extraction from ROM offsets
 * - Standard palette lookup for UI elements
 *
 * ## Font System
 * - Multiple font support (normal, short, Japanese variants)
 * - Glyph width tables for proper text rendering
 * - Gen3 character encoding to Unicode conversion
 *
 * ## ROM Version Support
 * - Pokemon FireRed (USA) 1.0, 1.1
 * - Pokemon LeafGreen (USA) 1.0, 1.1
 * - Pokemon Emerald (USA, Europe)
 *
 * The class uses dynamic offset loading from RomDatabase (YAML configuration)
 * to support different ROM versions without hardcoding offsets.
 *
 * @see RomDatabase for ROM identification and offset data
 * @see WonderCardRenderer for graphics rendering
 * @see Gen3FontRenderer for text rendering
 *
 * @author ComradeSean
 * @version 1.0
 */

#ifndef GBAROMREADER_H
#define GBAROMREADER_H

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QString>
#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QColor>
#include <QVector>
#include <QMap>
#include <cstdint>

// Forward declarations
class RomDatabase;

/**
 * @class GBAROReader
 * @brief Utility class for reading and extracting graphics from GBA ROMs.
 *
 * This class handles all ROM file access including:
 * - ROM loading and version identification via MD5 checksum
 * - Graphics extraction (tiles, sprites, backgrounds)
 * - Palette conversion from GBA BGR555 format
 * - Font and glyph width extraction
 * - Name table loading for script disassembly
 * - LZ77 decompression for compressed graphics
 *
 * ## GBA Graphics Formats
 *
 * **4bpp (4 bits per pixel)**:
 * Each pixel is an index (0-15) into a 16-color palette.
 * Stored as 2 pixels per byte: lower nibble = first pixel, upper nibble = second.
 * An 8x8 tile requires 32 bytes.
 *
 * **2bpp (2 bits per pixel)**:
 * Each pixel is an index (0-3) into a 4-color palette.
 * Used primarily for fonts. An 8x8 tile requires 16 bytes.
 *
 * **BGR555 Palette Format**:
 * 16-bit color: bits 0-4 = Red, bits 5-9 = Green, bits 10-14 = Blue.
 * Each channel is 5 bits (0-31), scaled to 8-bit (0-255) for display.
 *
 * All offsets are loaded dynamically from RomDatabase based on the
 * detected ROM version (no hardcoded offsets).
 */
class GBAROReader
{
public:
    GBAROReader();
    ~GBAROReader();

    // Load a ROM file and identify version using RomDatabase
    bool loadROM(const QString &path, QString &errorMessage);
    bool loadROM(const QString &path, RomDatabase *database, QString &errorMessage);
    bool isLoaded() const { return !m_romData.isEmpty() && m_versionIdentified; }
    bool isVersionIdentified() const { return m_versionIdentified; }
    QString versionName() const { return m_versionName; }
    QString gameFamily() const { return m_gameFamily; }

    // ROM info
    QString gameTitle() const;
    QString gameCode() const;
    qint64 romSize() const { return m_romData.size(); }

    // Graphics extraction
    QImage extractTile4bpp(uint32_t offset, const QVector<QRgb> &palette, int width = 8, int height = 8);
    QImage extractTileset4bpp(uint32_t offset, int tileCount, const QVector<QRgb> &palette, int tilesPerRow = 16);
    QImage extractPokemonIcon(uint16_t iconIndex);

    // Palette extraction
    QVector<QRgb> extractPalette(uint32_t offset, int colorCount = 16);
    static QRgb gbaColorToQRgb(uint16_t gbaColor);

    // Font extraction (2bpp)
    QImage extractFont2bpp(uint32_t offset, int numChars, int charWidth = 8, int charHeight = 16, int sourceTileColumns = 2);
    QImage extractFont();  // Uses default offsets (main font for current game)
    QImage extractFontByIndex(int fontIndex);  // Extract specific font by index

    // Glyph width extraction
    QVector<uint8_t> extractGlyphWidths(uint32_t offset, int count);
    QVector<uint8_t> getDefaultGlyphWidths();
    QVector<uint8_t> getGlyphWidthsByIndex(int fontIndex);

    // Check if this is Emerald ROM
    bool isEmerald() const { return m_gameFamily == "Emerald"; }

    // Name table extraction for script disassembly
    QString getItemName(uint16_t id) const;
    QString getPokemonName(uint16_t id) const;
    QString getMoveName(uint16_t id) const;
    bool hasNameTables() const { return m_hasNameTables; }
    int getItemCount() const { return m_hasNameTables ? m_itemTable.count : 0; }
    QStringList getAllItemNames() const;

    // Raw data reading
    uint8_t readByte(uint32_t offset) const;
    uint16_t readHalfWord(uint32_t offset) const;
    uint32_t readWord(uint32_t offset) const;
    QByteArray readBytes(uint32_t offset, int length) const;

    // LZ77 decompression
    QByteArray decompressLZ77(uint32_t offset, QString &errorMessage) const;

    // Wonder Card graphics extraction
    QImage extractWonderCardBackground(int index = 0);
    QImage extractWonderCardFrame();

    // Wonder Card table entry structure
    struct WonderCardGraphicsEntry {
        uint32_t tilesetPtr;
        uint32_t tilemapPtr;
        uint32_t palettePtr;
        uint32_t padding;
    };

    // Load Wonder Card entry from table
    WonderCardGraphicsEntry loadWonderCardEntry(int index);

    // Render Wonder Card from entry
    QImage renderWonderCard(const WonderCardGraphicsEntry &entry);

    // Dynamic offset getters (from RomDatabase based on detected version)
    uint32_t getIconSpritesOffset() const { return m_iconSprites; }
    uint32_t getIconPalettesOffset() const { return m_iconPalettes; }
    uint32_t getIconPaletteIndicesOffset() const { return m_iconPaletteIndices; }
    uint32_t getWonderCardTableOffset() const { return m_wondercardTable; }
    int getWonderCardCount() const { return m_wondercardCount; }
    uint32_t getStdPalOffset(int index) const;
    uint32_t getFontOffset() const { return m_fontOffset; }
    uint32_t getGlyphWidthsOffset() const { return m_glyphWidthsOffset; }

    // Glyph width table size (constant across versions)
    static const uint32_t GLYPH_WIDTHS_SIZE = 0x200;  // 512 entries

    // Tile/sprite constants
    static const int TILE_SIZE_4BPP = 32;  // 8x8 tile in 4bpp = 32 bytes
    static const int TILE_SIZE_2BPP = 16;  // 8x8 tile in 2bpp = 16 bytes
    static const int ICON_SIZE = 32;       // Pokemon icons are 32x32 pixels
    static const int ICON_TILES = 16;      // 32x32 = 4x4 tiles = 16 tiles

private:
    QByteArray m_romData;
    QString m_filePath;

    // ROM version info
    bool m_versionIdentified;
    QString m_versionName;
    QString m_gameFamily;

    // Dynamic offsets (loaded from RomDatabase)
    uint32_t m_iconSprites;
    uint32_t m_iconPalettes;
    uint32_t m_iconPaletteIndices;
    uint32_t m_wondercardTable;
    int m_wondercardCount;
    QVector<uint32_t> m_stdpalOffsets;
    uint32_t m_fontOffset;           // Main font (FONT_NORMAL for FRLG, FONT_SHORT for Emerald)
    uint32_t m_glyphWidthsOffset;    // Main font glyph widths

    // Additional fonts for Emerald (stores offset and glyph width offset by font index)
    QMap<int, uint32_t> m_fontOffsets;
    QMap<int, uint32_t> m_glyphWidthOffsets;

    // Name tables for script disassembly
    bool m_hasNameTables;
    struct NameTableInfo {
        uint32_t offset;
        int entrySize;
        int nameLength;  // For items only (name is at start of entry)
        int count;
    };
    NameTableInfo m_itemTable;
    NameTableInfo m_pokemonTable;
    NameTableInfo m_moveTable;
    mutable QMap<uint16_t, QString> m_itemNameCache;
    mutable QMap<uint16_t, QString> m_pokemonNameCache;
    mutable QMap<uint16_t, QString> m_moveNameCache;

    // Gen3 string decoder
    QString decodeGen3String(const QByteArray &data) const;

    // Helper functions
    bool validateROM();
    QImage tile4bppToImage(const uint8_t *tileData, const QVector<QRgb> &palette);
    QImage tile2bppToImage(const uint8_t *tileData, int width = 8, int height = 8);

    // 2bpp decoding helper
    QVector<QVector<uint8_t>> decode2bppTiles(uint32_t offset, int tileColumns, int tileRows);

    // Tilemap loading
    struct TilemapEntry {
        uint16_t tileIndex;
        uint8_t paletteIndex;
        bool hFlip;
        bool vFlip;
    };
    QVector<TilemapEntry> loadTilemap(const QByteArray &data);
};

#endif // GBAROMREADER_H

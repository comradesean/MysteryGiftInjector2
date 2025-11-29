/**
 * @file romdatabase.h
 * @brief ROM version identification and offset database for Pokemon Gen3 ROMs.
 *
 * This file provides the RomDatabase class which manages ROM identification
 * and stores all ROM-specific offsets needed for graphics extraction.
 *
 * ## Purpose
 * Different ROM versions (FireRed 1.0, 1.1, LeafGreen, Emerald) have data at
 * different offsets. Rather than hardcoding these, we load them from a YAML
 * configuration file (gen3_rom_data.yaml) at runtime.
 *
 * ## ROM Identification
 * ROMs are identified by their MD5 checksum, which provides reliable version
 * detection regardless of filename or modifications.
 *
 * ## Offset System
 * The database uses a delta-based offset system:
 * - Base offsets are defined for FireRed 1.0
 * - Other versions specify an "offsetDelta" that adjusts relative offsets
 * - Some offsets (palettes, sprites) are explicit per-version
 *
 * ## Supported ROMs
 * - Pokemon FireRed (USA) 1.0 - MD5: e26ee0d44e809351c8ce2f093c5f22a4
 * - Pokemon FireRed (USA) 1.1 - MD5: 51901a6e40661b3914aa333c802e24e8
 * - Pokemon LeafGreen (USA) 1.0 - MD5: 612ca9473451fa42b51d1711e1c47e9
 * - Pokemon LeafGreen (USA) 1.1 - MD5: 9d33a02159e018d09073e700e1fd10fd
 * - Pokemon Emerald (USA) - MD5: 605b89b67018abcea91e693a4dd25be3
 *
 * @see gen3_rom_data.yaml for the full offset database
 * @see GBAROReader for ROM access implementation
 *
 * @author ComradeSean
 * @version 1.0
 */

#ifndef ROMDATABASE_H
#define ROMDATABASE_H

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QString>
#include <QVector>
#include <QMap>
#include <cstdint>

/**
 * @class RomDatabase
 * @brief Manages ROM version identification and offset data.
 *
 * This class loads ROM offset data from a YAML configuration file and provides
 * lookup methods for:
 * - ROM version identification by MD5 hash
 * - Graphics offsets (sprites, palettes, tiles)
 * - Font offsets and glyph width tables
 * - Name tables for script disassembly
 */
class RomDatabase
{
public:
    struct GlyphInfo {
        uint32_t offset;
        uint32_t size;
        int dimensions[2];      // [width, height] in tiles
        int charSize[2];        // [width, height] in pixels
        int sourceTileColumns;
        QString widthTableName; // Empty if fixed width
        int fixedWidth;         // 0 if using width table
    };

    struct GlyphWidthTable {
        uint32_t offset;
        uint32_t size;
    };

    // Name table info for script disassembly
    struct NameTableInfo {
        uint32_t offset;
        int entrySize;
        int nameLength;  // For items only
        int count;
    };

    struct RomVersion {
        QString name;           // "FireRed_1.0"
        QString gameFamily;     // "FRLG" or "Emerald"
        QString code;           // "BPRE"
        QString md5;
        int offsetDelta;

        // Explicit offsets - palettes
        QVector<uint32_t> stdpalOffsets;
        QVector<uint32_t> wondercardPaletteOffsets;
        QVector<uint32_t> stampShadowOffsets;

        // Pokemon sprite offsets
        uint32_t frontSprites;
        uint32_t backSprites;
        uint32_t frontPalettes;
        uint32_t backPalettes;
        uint32_t shinyPalettes;
        uint32_t iconSprites;
        uint32_t iconPalettes;
        uint32_t iconPaletteIndices;

        // Wonder Card
        uint32_t wondercardTable;
        int wondercardCount;

        // Name tables for script disassembly
        bool hasNameTables;
        NameTableInfo itemTable;
        NameTableInfo pokemonTable;
        NameTableInfo moveTable;
    };

    struct GameFamily {
        QString name;           // "FRLG" or "Emerald"
        int bpp;
        int pokemonCount;
        QMap<int, GlyphInfo> glyphsLatin;
        QMap<int, GlyphInfo> glyphsJapanese;
        QMap<QString, GlyphWidthTable> glyphWidths;
        QVector<RomVersion> versions;
    };

    RomDatabase();
    ~RomDatabase();

    bool loadFromYaml(const QString &path, QString &error);
    bool isLoaded() const { return m_loaded; }

    // Find ROM version by MD5 hash
    const RomVersion* identifyRom(const QString &md5Hash) const;

    // Get ROM version by name
    const RomVersion* getVersion(const QString &name) const;

    // Get game family for a version
    const GameFamily* getGameFamily(const QString &familyName) const;

    // Get glyph offset with delta applied
    uint32_t getGlyphOffset(const RomVersion *version, int fontIndex, bool japanese = false) const;
    uint32_t getGlyphWidthOffset(const RomVersion *version, const QString &widthTableName) const;

    // Get all supported MD5 hashes
    QStringList getSupportedMD5Hashes() const;

private:
    bool m_loaded;
    QMap<QString, GameFamily> m_gameFamilies;
    QMap<QString, RomVersion*> m_versionsByMd5;
    QMap<QString, RomVersion*> m_versionsByName;

    // YAML parsing helpers
    bool parseYamlFile(const QString &content, QString &error);
    bool parseGameFamily(const QString &familyName, const QStringList &lines, int &lineIndex, QString &error);
    bool parseVersion(const QString &versionName, const QString &familyName, const QStringList &lines, int &lineIndex, QString &error);
    bool parseGlyphs(GameFamily &family, const QStringList &lines, int &lineIndex, QString &error);
    bool parseGlyphWidths(GameFamily &family, const QStringList &lines, int &lineIndex, QString &error);

    int getIndentLevel(const QString &line) const;
    QString trimLine(const QString &line) const;
    QVector<uint32_t> parseHexArray(const QString &arrayStr) const;
    uint32_t parseHex(const QString &hexStr) const;
};

#endif // ROMDATABASE_H

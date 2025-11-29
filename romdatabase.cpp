/**
 * @file romdatabase.cpp
 * @brief Implementation of ROM database loading and lookup.
 *
 * This file implements the RomDatabase class which parses the gen3_rom_data.yaml
 * configuration file and provides ROM identification and offset lookup services.
 *
 * ## YAML Parsing
 * The implementation includes a custom YAML parser (not using a full YAML library)
 * that handles the specific structure of gen3_rom_data.yaml. This includes:
 * - Game family definitions (FRLG, Emerald)
 * - ROM version data with MD5 hashes
 * - Glyph/font offset tables
 * - Name table offsets for script disassembly
 *
 * ## Offset Resolution
 * When looking up offsets, the database applies the version-specific offsetDelta
 * to base offsets, allowing shared glyph tables across versions while maintaining
 * per-version palette and sprite locations.
 *
 * @see romdatabase.h for class interface
 * @see gen3_rom_data.yaml for configuration format
 *
 * @author ComradeSean
 * @version 1.0
 */

// =============================================================================
// Project Includes
// =============================================================================
#include "romdatabase.h"

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

// =============================================================================
// CONSTRUCTOR & DESTRUCTOR
// =============================================================================

RomDatabase::RomDatabase()
    : m_loaded(false)
{
}

RomDatabase::~RomDatabase()
{
}

bool RomDatabase::loadFromYaml(const QString &path, QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QString("Failed to open YAML file: %1").arg(path);
        return false;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    if (!parseYamlFile(content, error)) {
        return false;
    }

    m_loaded = true;

    // Debug: Print all loaded ROM versions and their MD5s
    qDebug() << "ROM Database loaded. Versions found:";
    for (const QString &md5 : m_versionsByMd5.keys()) {
        qDebug() << "  MD5:" << md5 << "-> Version:" << m_versionsByMd5[md5]->name;
    }

    return true;
}

const RomDatabase::RomVersion* RomDatabase::identifyRom(const QString &md5Hash) const
{
    QString lowerMd5 = md5Hash.toLower();
    if (m_versionsByMd5.contains(lowerMd5)) {
        return m_versionsByMd5[lowerMd5];
    }
    return nullptr;
}

const RomDatabase::RomVersion* RomDatabase::getVersion(const QString &name) const
{
    if (m_versionsByName.contains(name)) {
        return m_versionsByName[name];
    }
    return nullptr;
}

const RomDatabase::GameFamily* RomDatabase::getGameFamily(const QString &familyName) const
{
    auto it = m_gameFamilies.find(familyName);
    if (it != m_gameFamilies.end()) {
        return &it.value();
    }
    return nullptr;
}

uint32_t RomDatabase::getGlyphOffset(const RomVersion *version, int fontIndex, bool japanese) const
{
    if (!version) return 0;

    const GameFamily *family = getGameFamily(version->gameFamily);
    if (!family) return 0;

    const QMap<int, GlyphInfo> &glyphs = japanese ? family->glyphsJapanese : family->glyphsLatin;
    if (!glyphs.contains(fontIndex)) return 0;

    // Apply offset delta to base glyph offset
    return glyphs[fontIndex].offset + version->offsetDelta;
}

uint32_t RomDatabase::getGlyphWidthOffset(const RomVersion *version, const QString &widthTableName) const
{
    if (!version) return 0;

    const GameFamily *family = getGameFamily(version->gameFamily);
    if (!family) return 0;

    if (!family->glyphWidths.contains(widthTableName)) return 0;

    // Apply offset delta to base width table offset
    return family->glyphWidths[widthTableName].offset + version->offsetDelta;
}

QStringList RomDatabase::getSupportedMD5Hashes() const
{
    return m_versionsByMd5.keys();
}

int RomDatabase::getIndentLevel(const QString &line) const
{
    int spaces = 0;
    for (QChar c : line) {
        if (c == ' ') spaces++;
        else break;
    }
    return spaces / 2; // 2 spaces per indent level
}

QString RomDatabase::trimLine(const QString &line) const
{
    return line.trimmed();
}

QVector<uint32_t> RomDatabase::parseHexArray(const QString &arrayStr) const
{
    QVector<uint32_t> result;

    // Parse format: [0x123, 0x456, ...]
    QString cleaned = arrayStr;
    cleaned.remove('[').remove(']').remove(' ');

    QStringList parts = cleaned.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        result.append(parseHex(part.trimmed()));
    }

    return result;
}

uint32_t RomDatabase::parseHex(const QString &hexStr) const
{
    QString cleaned = hexStr.trimmed();

    // Strip YAML comments (everything after #)
    int commentPos = cleaned.indexOf('#');
    if (commentPos >= 0) {
        cleaned = cleaned.left(commentPos).trimmed();
    }

    if (cleaned.startsWith("-0x") || cleaned.startsWith("-0X")) {
        // Handle negative hex (for offset_delta)
        cleaned = cleaned.mid(3);
        bool ok;
        return static_cast<uint32_t>(-static_cast<int32_t>(cleaned.toUInt(&ok, 16)));
    }
    if (cleaned.startsWith("0x") || cleaned.startsWith("0X")) {
        cleaned = cleaned.mid(2);
    }
    bool ok;
    return cleaned.toUInt(&ok, 16);
}

bool RomDatabase::parseYamlFile(const QString &content, QString &error)
{
    QStringList lines = content.split('\n');
    int lineIndex = 0;

    // Skip to 'games:' section
    while (lineIndex < lines.size()) {
        QString line = trimLine(lines[lineIndex]);
        if (line == "games:") {
            lineIndex++;
            break;
        }
        lineIndex++;
    }

    if (lineIndex >= lines.size()) {
        error = "Could not find 'games:' section in YAML";
        return false;
    }

    // Parse game families (FRLG, Emerald)
    while (lineIndex < lines.size()) {
        QString line = lines[lineIndex];
        if (line.trimmed().isEmpty() || line.trimmed().startsWith('#')) {
            lineIndex++;
            continue;
        }

        int indent = getIndentLevel(line);
        if (indent == 1) {
            // Game family name (FRLG: or Emerald:)
            QString familyName = trimLine(line).remove(':');
            if (!parseGameFamily(familyName, lines, lineIndex, error)) {
                return false;
            }
        } else if (indent == 0 && !line.trimmed().isEmpty()) {
            // End of games section
            break;
        } else {
            lineIndex++;
        }
    }

    return true;
}

bool RomDatabase::parseGameFamily(const QString &familyName, const QStringList &lines, int &lineIndex, QString &error)
{
    GameFamily family;
    family.name = familyName;
    family.bpp = 2;
    family.pokemonCount = 440;

    lineIndex++; // Move past family name line

    while (lineIndex < lines.size()) {
        QString line = lines[lineIndex];
        if (line.trimmed().isEmpty() || line.trimmed().startsWith('#')) {
            lineIndex++;
            continue;
        }

        int indent = getIndentLevel(line);
        if (indent <= 1) {
            // Back to game family level or higher
            break;
        }

        QString trimmed = trimLine(line);

        if (trimmed.startsWith("bpp:")) {
            family.bpp = trimmed.mid(4).trimmed().toInt();
            lineIndex++;
        } else if (trimmed.startsWith("pokemon_count:")) {
            family.pokemonCount = trimmed.mid(14).trimmed().toInt();
            lineIndex++;
        } else if (trimmed == "versions:") {
            lineIndex++;
            // Parse versions
            while (lineIndex < lines.size()) {
                QString vLine = lines[lineIndex];
                if (vLine.trimmed().isEmpty() || vLine.trimmed().startsWith('#')) {
                    lineIndex++;
                    continue;
                }

                int vIndent = getIndentLevel(vLine);
                if (vIndent <= 2) break;

                if (vIndent == 3 && vLine.trimmed().endsWith(':')) {
                    QString versionName = trimLine(vLine).remove(':');
                    if (!parseVersion(versionName, familyName, lines, lineIndex, error)) {
                        return false;
                    }
                    family.versions.append(*m_versionsByName[versionName]);
                } else {
                    lineIndex++;
                }
            }
        } else if (trimmed == "glyphs:") {
            if (!parseGlyphs(family, lines, lineIndex, error)) {
                return false;
            }
        } else if (trimmed == "glyph_widths:") {
            if (!parseGlyphWidths(family, lines, lineIndex, error)) {
                return false;
            }
        } else {
            lineIndex++;
        }
    }

    m_gameFamilies[familyName] = family;
    return true;
}

bool RomDatabase::parseVersion(const QString &versionName, const QString &familyName, const QStringList &lines, int &lineIndex, QString &error)
{
    RomVersion version;
    version.name = versionName;
    version.gameFamily = familyName;
    version.offsetDelta = 0;
    version.wondercardCount = 8;
    version.hasNameTables = false;
    version.itemTable = {0, 0, 0, 0};
    version.pokemonTable = {0, 0, 0, 0};
    version.moveTable = {0, 0, 0, 0};

    lineIndex++; // Move past version name line

    while (lineIndex < lines.size()) {
        QString line = lines[lineIndex];
        if (line.trimmed().isEmpty() || line.trimmed().startsWith('#')) {
            lineIndex++;
            continue;
        }

        int indent = getIndentLevel(line);
        if (indent <= 3) break;

        QString trimmed = trimLine(line);

        if (trimmed.startsWith("code:")) {
            version.code = trimmed.mid(5).trimmed();
        } else if (trimmed.startsWith("md5:")) {
            version.md5 = trimmed.mid(4).trimmed().toLower();
        } else if (trimmed.startsWith("offset_delta:")) {
            QString deltaStr = trimmed.mid(13).trimmed();
            if (deltaStr.startsWith("-")) {
                version.offsetDelta = -static_cast<int>(parseHex(deltaStr.mid(1)));
            } else {
                version.offsetDelta = static_cast<int>(parseHex(deltaStr));
            }
        } else if (trimmed == "palettes:") {
            lineIndex++;
            // Parse palette arrays
            while (lineIndex < lines.size()) {
                QString pLine = lines[lineIndex];
                int pIndent = getIndentLevel(pLine);
                if (pIndent <= 4) break;

                QString pTrimmed = trimLine(pLine);
                if (pTrimmed.startsWith("stdpal:")) {
                    version.stdpalOffsets = parseHexArray(pTrimmed.mid(7));
                } else if (pTrimmed.startsWith("wondercard:")) {
                    version.wondercardPaletteOffsets = parseHexArray(pTrimmed.mid(11));
                } else if (pTrimmed.startsWith("stamp_shadow:")) {
                    version.stampShadowOffsets = parseHexArray(pTrimmed.mid(13));
                }
                lineIndex++;
            }
            continue;
        } else if (trimmed == "pokemon_sprites:") {
            lineIndex++;
            // Parse sprite offsets
            while (lineIndex < lines.size()) {
                QString sLine = lines[lineIndex];
                int sIndent = getIndentLevel(sLine);
                if (sIndent <= 4) break;

                QString sTrimmed = trimLine(sLine);
                if (sTrimmed.startsWith("front_sprites:")) {
                    version.frontSprites = parseHex(sTrimmed.mid(14));
                } else if (sTrimmed.startsWith("back_sprites:")) {
                    version.backSprites = parseHex(sTrimmed.mid(13));
                } else if (sTrimmed.startsWith("front_palettes:")) {
                    version.frontPalettes = parseHex(sTrimmed.mid(15));
                } else if (sTrimmed.startsWith("back_palettes:")) {
                    version.backPalettes = parseHex(sTrimmed.mid(14));
                } else if (sTrimmed.startsWith("shiny_palettes:")) {
                    version.shinyPalettes = parseHex(sTrimmed.mid(15));
                } else if (sTrimmed.startsWith("icon_sprites:")) {
                    version.iconSprites = parseHex(sTrimmed.mid(13));
                } else if (sTrimmed.startsWith("icon_palettes:")) {
                    version.iconPalettes = parseHex(sTrimmed.mid(14));
                } else if (sTrimmed.startsWith("icon_palette_indices:")) {
                    version.iconPaletteIndices = parseHex(sTrimmed.mid(21));
                }
                lineIndex++;
            }
            continue;
        } else if (trimmed.startsWith("wondercard_table:")) {
            version.wondercardTable = parseHex(trimmed.mid(17));
        } else if (trimmed.startsWith("wondercard_count:")) {
            version.wondercardCount = trimmed.mid(17).trimmed().toInt();
        } else if (trimmed == "name_tables:") {
            lineIndex++;
            version.hasNameTables = true;
            // Parse name table sections (items, pokemon, moves)
            while (lineIndex < lines.size()) {
                QString ntLine = lines[lineIndex];
                int ntIndent = getIndentLevel(ntLine);
                if (ntIndent <= 4) break;

                QString ntTrimmed = trimLine(ntLine);
                if (ntTrimmed == "items:" || ntTrimmed == "pokemon:" || ntTrimmed == "moves:") {
                    bool isItems = ntTrimmed == "items:";
                    bool isPokemon = ntTrimmed == "pokemon:";
                    NameTableInfo *table = isItems ? &version.itemTable :
                                          (isPokemon ? &version.pokemonTable : &version.moveTable);
                    lineIndex++;
                    // Parse table properties
                    while (lineIndex < lines.size()) {
                        QString propLine = lines[lineIndex];
                        int propIndent = getIndentLevel(propLine);
                        if (propIndent <= 5) break;

                        QString propTrimmed = trimLine(propLine);
                        // Strip inline YAML comments for integer parsing
                        auto parseIntValue = [](const QString &str) -> int {
                            QString cleaned = str.trimmed();
                            int commentPos = cleaned.indexOf('#');
                            if (commentPos >= 0) {
                                cleaned = cleaned.left(commentPos).trimmed();
                            }
                            return cleaned.toInt();
                        };

                        if (propTrimmed.startsWith("offset:")) {
                            table->offset = parseHex(propTrimmed.mid(7));
                        } else if (propTrimmed.startsWith("entry_size:")) {
                            table->entrySize = parseIntValue(propTrimmed.mid(11));
                        } else if (propTrimmed.startsWith("name_length:")) {
                            table->nameLength = parseIntValue(propTrimmed.mid(12));
                        } else if (propTrimmed.startsWith("count:")) {
                            table->count = parseIntValue(propTrimmed.mid(6));
                        }
                        lineIndex++;
                    }
                    continue;
                }
                lineIndex++;
            }
            continue;
        }

        lineIndex++;
    }

    // Store version
    m_versionsByName[versionName] = new RomVersion(version);
    if (!version.md5.isEmpty()) {
        m_versionsByMd5[version.md5] = m_versionsByName[versionName];
    }

    return true;
}

bool RomDatabase::parseGlyphs(GameFamily &family, const QStringList &lines, int &lineIndex, QString &error)
{
    lineIndex++; // Move past 'glyphs:'

    while (lineIndex < lines.size()) {
        QString line = lines[lineIndex];
        if (line.trimmed().isEmpty() || line.trimmed().startsWith('#')) {
            lineIndex++;
            continue;
        }

        int indent = getIndentLevel(line);
        if (indent <= 2) break;

        QString trimmed = trimLine(line);

        // Font index (0:, 1:, etc.)
        if (indent == 3 && trimmed.endsWith(':')) {
            bool ok;
            int fontIndex = trimmed.remove(':').toInt(&ok);
            if (!ok) {
                lineIndex++;
                continue;
            }

            lineIndex++;

            GlyphInfo latinInfo = {};
            GlyphInfo japaneseInfo = {};
            bool hasLatin = false;
            bool hasJapanese = false;

            // Parse font entry
            while (lineIndex < lines.size()) {
                QString fLine = lines[lineIndex];
                int fIndent = getIndentLevel(fLine);
                if (fIndent <= 3) break;

                QString fTrimmed = trimLine(fLine);

                if (fTrimmed.startsWith("name:")) {
                    // Skip name
                } else if (fTrimmed == "latin:") {
                    lineIndex++;
                    // Check for null
                    if (lineIndex < lines.size() && trimLine(lines[lineIndex]) == "null") {
                        lineIndex++;
                        continue;
                    }
                    // Parse latin glyph info
                    while (lineIndex < lines.size()) {
                        QString lLine = lines[lineIndex];
                        int lIndent = getIndentLevel(lLine);
                        if (lIndent <= 4) break;

                        QString lTrimmed = trimLine(lLine);
                        if (lTrimmed.startsWith("offset:")) {
                            latinInfo.offset = parseHex(lTrimmed.mid(7));
                        } else if (lTrimmed.startsWith("size:")) {
                            latinInfo.size = parseHex(lTrimmed.mid(5));
                        } else if (lTrimmed.startsWith("dimensions:")) {
                            QVector<uint32_t> dims = parseHexArray(lTrimmed.mid(11));
                            if (dims.size() >= 2) {
                                latinInfo.dimensions[0] = dims[0];
                                latinInfo.dimensions[1] = dims[1];
                            }
                        } else if (lTrimmed.startsWith("char_size:")) {
                            QVector<uint32_t> cs = parseHexArray(lTrimmed.mid(10));
                            if (cs.size() >= 2) {
                                latinInfo.charSize[0] = cs[0];
                                latinInfo.charSize[1] = cs[1];
                            }
                        } else if (lTrimmed.startsWith("source_tile_columns:")) {
                            latinInfo.sourceTileColumns = lTrimmed.mid(20).trimmed().toInt();
                        } else if (lTrimmed.startsWith("width:")) {
                            latinInfo.widthTableName = lTrimmed.mid(6).trimmed();
                        } else if (lTrimmed.startsWith("fixed_width:")) {
                            latinInfo.fixedWidth = lTrimmed.mid(12).trimmed().toInt();
                        }
                        lineIndex++;
                    }
                    hasLatin = true;
                    continue;
                } else if (fTrimmed == "japanese:") {
                    lineIndex++;
                    // Check for null
                    if (lineIndex < lines.size() && trimLine(lines[lineIndex]) == "null") {
                        lineIndex++;
                        continue;
                    }
                    // Parse japanese glyph info (similar to latin)
                    while (lineIndex < lines.size()) {
                        QString jLine = lines[lineIndex];
                        int jIndent = getIndentLevel(jLine);
                        if (jIndent <= 4) break;

                        QString jTrimmed = trimLine(jLine);
                        if (jTrimmed.startsWith("offset:")) {
                            japaneseInfo.offset = parseHex(jTrimmed.mid(7));
                        } else if (jTrimmed.startsWith("size:")) {
                            japaneseInfo.size = parseHex(jTrimmed.mid(5));
                        } else if (jTrimmed.startsWith("dimensions:")) {
                            QVector<uint32_t> dims = parseHexArray(jTrimmed.mid(11));
                            if (dims.size() >= 2) {
                                japaneseInfo.dimensions[0] = dims[0];
                                japaneseInfo.dimensions[1] = dims[1];
                            }
                        } else if (jTrimmed.startsWith("char_size:")) {
                            QVector<uint32_t> cs = parseHexArray(jTrimmed.mid(10));
                            if (cs.size() >= 2) {
                                japaneseInfo.charSize[0] = cs[0];
                                japaneseInfo.charSize[1] = cs[1];
                            }
                        } else if (jTrimmed.startsWith("source_tile_columns:")) {
                            japaneseInfo.sourceTileColumns = jTrimmed.mid(20).trimmed().toInt();
                        } else if (jTrimmed.startsWith("width:")) {
                            japaneseInfo.widthTableName = jTrimmed.mid(6).trimmed();
                        } else if (jTrimmed.startsWith("fixed_width:")) {
                            japaneseInfo.fixedWidth = jTrimmed.mid(12).trimmed().toInt();
                        }
                        lineIndex++;
                    }
                    hasJapanese = true;
                    continue;
                }

                lineIndex++;
            }

            if (hasLatin) {
                family.glyphsLatin[fontIndex] = latinInfo;
            }
            if (hasJapanese) {
                family.glyphsJapanese[fontIndex] = japaneseInfo;
            }
        } else {
            lineIndex++;
        }
    }

    return true;
}

bool RomDatabase::parseGlyphWidths(GameFamily &family, const QStringList &lines, int &lineIndex, QString &error)
{
    lineIndex++; // Move past 'glyph_widths:'

    while (lineIndex < lines.size()) {
        QString line = lines[lineIndex];
        if (line.trimmed().isEmpty() || line.trimmed().startsWith('#')) {
            lineIndex++;
            continue;
        }

        int indent = getIndentLevel(line);
        if (indent <= 2) break;

        QString trimmed = trimLine(line);

        // Width table name (ends with :)
        if (indent == 3 && trimmed.endsWith(':')) {
            QString tableName = trimmed.remove(':');
            lineIndex++;

            GlyphWidthTable table = {};

            while (lineIndex < lines.size()) {
                QString wLine = lines[lineIndex];
                int wIndent = getIndentLevel(wLine);
                if (wIndent <= 3) break;

                QString wTrimmed = trimLine(wLine);
                if (wTrimmed.startsWith("offset:")) {
                    table.offset = parseHex(wTrimmed.mid(7));
                } else if (wTrimmed.startsWith("size:")) {
                    table.size = parseHex(wTrimmed.mid(5));
                }
                lineIndex++;
            }

            family.glyphWidths[tableName] = table;
        } else {
            lineIndex++;
        }
    }

    return true;
}

/**
 * @file gbaromreader.cpp
 * @brief Implementation of GBA ROM reading and graphics extraction.
 *
 * This file implements the GBAROReader class which provides all ROM access
 * functionality for the Mystery Gift Injector. Key operations include:
 *
 * ## ROM Loading & Identification
 * - File loading with validation (header checks)
 * - ROM version identification via MD5 checksum
 * - Dynamic offset loading from RomDatabase
 *
 * ## Graphics Extraction
 * - 4bpp tile extraction (32 bytes/tile for sprites and backgrounds)
 * - 2bpp tile extraction (16 bytes/tile for fonts)
 * - LZ77 decompression for compressed sprite data
 * - Wonder Card background rendering from tilemap + tileset
 * - Pokemon icon extraction with proper palette mapping
 *
 * ## Palette Handling
 * - BGR555 to ARGB32 conversion
 * - Palette extraction from ROM offsets
 * - Standard palette lookup for UI elements
 *
 * ## Font System
 * - Font glyph extraction as 2bpp tiles
 * - Glyph width table loading for text layout
 * - Multiple font support (Emerald uses different fonts than FRLG)
 *
 * ## Name Tables (for Script Disassembly)
 * - Item name extraction with caching
 * - Pokemon name extraction with caching
 * - Move name extraction with caching
 * - Gen3 string encoding to Unicode conversion
 *
 * @see gbaromreader.h for class interface
 * @see romdatabase.h for ROM offset data
 *
 * @author ComradeSean
 * @version 1.0
 */

// =============================================================================
// Project Includes
// =============================================================================
#include "gbaromreader.h"
#include "romdatabase.h"
#include "romloader.h"

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QFile>
#include <QDebug>

// =============================================================================
// CONSTRUCTOR & DESTRUCTOR
// =============================================================================

GBAROReader::GBAROReader()
    : m_versionIdentified(false)
    , m_iconSprites(0)
    , m_iconPalettes(0)
    , m_iconPaletteIndices(0)
    , m_wondercardTable(0)
    , m_wondercardCount(8)
    , m_fontOffset(0)
    , m_glyphWidthsOffset(0)
    , m_hasNameTables(false)
{
    m_itemTable = {0, 0, 0, 0};
    m_pokemonTable = {0, 0, 0, 0};
    m_moveTable = {0, 0, 0, 0};
}

GBAROReader::~GBAROReader()
{
}

bool GBAROReader::loadROM(const QString &path, QString &errorMessage)
{
    // Load without database - uses embedded resource for identification
    RomDatabase db;
    QString dbError;
    if (db.loadFromYaml(":/Resources/gen3_rom_data.yaml", dbError)) {
        return loadROM(path, &db, errorMessage);
    } else {
        errorMessage = "Failed to load ROM database: " + dbError;
        return false;
    }
}

bool GBAROReader::loadROM(const QString &path, RomDatabase *database, QString &errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = QString("Could not open ROM file: %1").arg(file.errorString());
        return false;
    }

    m_romData = file.readAll();
    file.close();

    m_filePath = path;

    if (!validateROM()) {
        errorMessage = "Invalid GBA ROM file";
        m_romData.clear();
        return false;
    }

    // Identify ROM version using MD5
    if (database && database->isLoaded()) {
        QString md5 = RomLoader::computeMD5(path);
        const RomDatabase::RomVersion *version = database->identifyRom(md5);

        if (version) {
            m_versionIdentified = true;
            m_versionName = version->name;
            m_gameFamily = version->gameFamily;

            // Load offsets from database
            m_iconSprites = version->iconSprites;
            m_iconPalettes = version->iconPalettes;
            m_iconPaletteIndices = version->iconPaletteIndices;
            m_wondercardTable = version->wondercardTable;
            m_wondercardCount = version->wondercardCount;
            m_stdpalOffsets = version->stdpalOffsets;

            // Get font and glyph width offsets from game family
            const RomDatabase::GameFamily *family = database->getGameFamily(version->gameFamily);
            if (family) {
                if (version->gameFamily == "Emerald") {
                    // Emerald uses two fonts:
                    // - FONT_SHORT_COPY_1 (index 3) for main text (title, subtitle, body, footer)
                    //   (identical to FONT_SHORT index 2, uses gFontShortLatinGlyphWidths)
                    // - FONT_NORMAL (index 1) for ID number only
                    int mainFontIndex = 3;  // FONT_SHORT_COPY_1
                    int idFontIndex = 1;    // FONT_NORMAL

                    // Load main font (FONT_SHORT_COPY_1)
                    if (family->glyphsLatin.contains(mainFontIndex)) {
                        const RomDatabase::GlyphInfo &glyph = family->glyphsLatin[mainFontIndex];
                        m_fontOffset = glyph.offset + version->offsetDelta;
                        m_fontOffsets[mainFontIndex] = m_fontOffset;

                        if (!glyph.widthTableName.isEmpty() && family->glyphWidths.contains(glyph.widthTableName)) {
                            m_glyphWidthsOffset = family->glyphWidths[glyph.widthTableName].offset + version->offsetDelta;
                            m_glyphWidthOffsets[mainFontIndex] = m_glyphWidthsOffset;
                        }
                    }

                    // Load ID font (FONT_NORMAL)
                    if (family->glyphsLatin.contains(idFontIndex)) {
                        const RomDatabase::GlyphInfo &glyph = family->glyphsLatin[idFontIndex];
                        m_fontOffsets[idFontIndex] = glyph.offset + version->offsetDelta;

                        if (!glyph.widthTableName.isEmpty() && family->glyphWidths.contains(glyph.widthTableName)) {
                            m_glyphWidthOffsets[idFontIndex] = family->glyphWidths[glyph.widthTableName].offset + version->offsetDelta;
                        }
                    }

                    qDebug() << "  Emerald fonts loaded:";
                    qDebug() << "    FONT_SHORT_COPY_1 (main):" << QString("0x%1").arg(m_fontOffsets.value(mainFontIndex, 0), 0, 16);
                    qDebug() << "    FONT_NORMAL (ID):" << QString("0x%1").arg(m_fontOffsets.value(idFontIndex, 0), 0, 16);
                } else {
                    // FRLG uses FONT_NORMAL_COPY_2 (index 3) for everything
                    // (identical to FONT_NORMAL index 2, uses sFontNormalLatinGlyphWidths)
                    int fontIndex = 3;
                    if (family->glyphsLatin.contains(fontIndex)) {
                        const RomDatabase::GlyphInfo &glyph = family->glyphsLatin[fontIndex];
                        m_fontOffset = glyph.offset + version->offsetDelta;
                        m_fontOffsets[fontIndex] = m_fontOffset;

                        if (!glyph.widthTableName.isEmpty() && family->glyphWidths.contains(glyph.widthTableName)) {
                            m_glyphWidthsOffset = family->glyphWidths[glyph.widthTableName].offset + version->offsetDelta;
                            m_glyphWidthOffsets[fontIndex] = m_glyphWidthsOffset;
                        }
                    }
                }
            }

            // Load name tables for script disassembly
            if (version->hasNameTables) {
                m_hasNameTables = true;
                m_itemTable.offset = version->itemTable.offset;
                m_itemTable.entrySize = version->itemTable.entrySize;
                m_itemTable.nameLength = version->itemTable.nameLength;
                m_itemTable.count = version->itemTable.count;

                m_pokemonTable.offset = version->pokemonTable.offset;
                m_pokemonTable.entrySize = version->pokemonTable.entrySize;
                m_pokemonTable.nameLength = 0;  // Not used for pokemon
                m_pokemonTable.count = version->pokemonTable.count;

                m_moveTable.offset = version->moveTable.offset;
                m_moveTable.entrySize = version->moveTable.entrySize;
                m_moveTable.nameLength = 0;  // Not used for moves
                m_moveTable.count = version->moveTable.count;
            }

            qDebug() << "ROM identified:" << m_versionName;
            qDebug() << "  Icon sprites:" << QString("0x%1").arg(m_iconSprites, 0, 16);
            qDebug() << "  Wonder Card table:" << QString("0x%1").arg(m_wondercardTable, 0, 16);
            qDebug() << "  Font offset:" << QString("0x%1").arg(m_fontOffset, 0, 16);
            qDebug() << "  Glyph widths:" << QString("0x%1").arg(m_glyphWidthsOffset, 0, 16);
            qDebug() << "  version->hasNameTables:" << version->hasNameTables;
            qDebug() << "  m_hasNameTables:" << m_hasNameTables;
            if (m_hasNameTables) {
                qDebug() << "  Name tables: items @" << QString("0x%1").arg(m_itemTable.offset, 0, 16)
                         << ", pokemon @" << QString("0x%1").arg(m_pokemonTable.offset, 0, 16)
                         << ", moves @" << QString("0x%1").arg(m_moveTable.offset, 0, 16);
                qDebug() << "    item table count:" << m_itemTable.count << "entry size:" << m_itemTable.entrySize;
            } else {
                qDebug() << "  Name tables NOT loaded";
            }
        } else {
            errorMessage = QString("Unknown ROM (MD5: %1). Supported ROMs: FireRed, LeafGreen, Emerald").arg(md5);
            m_romData.clear();
            return false;
        }
    } else {
        errorMessage = "ROM database not available for version identification";
        m_romData.clear();
        return false;
    }

    return true;
}

uint32_t GBAROReader::getStdPalOffset(int index) const
{
    if (index >= 0 && index < m_stdpalOffsets.size()) {
        return m_stdpalOffsets[index];
    }
    return 0;
}

bool GBAROReader::validateROM()
{
    // GBA ROMs should be at least 1MB and have proper header
    if (m_romData.size() < 1024 * 1024) {
        return false;
    }

    // Check for Nintendo logo (optional - some ROMs may have it stripped)
    // At minimum, check that we can read the header
    return true;
}

QString GBAROReader::gameTitle() const
{
    if (m_romData.size() < 0xAC + 12) {
        return QString();
    }

    // Game title is at 0xA0, 12 bytes
    QByteArray title = m_romData.mid(0xA0, 12);
    return QString::fromLatin1(title).trimmed();
}

QString GBAROReader::gameCode() const
{
    if (m_romData.size() < 0xAC + 4) {
        return QString();
    }

    // Game code is at 0xAC, 4 bytes
    QByteArray code = m_romData.mid(0xAC, 4);
    return QString::fromLatin1(code);
}

uint8_t GBAROReader::readByte(uint32_t offset) const
{
    if (offset >= static_cast<uint32_t>(m_romData.size())) {
        return 0;
    }
    return static_cast<uint8_t>(m_romData[offset]);
}

uint16_t GBAROReader::readHalfWord(uint32_t offset) const
{
    if (offset + 1 >= static_cast<uint32_t>(m_romData.size())) {
        return 0;
    }
    // Must cast to uint8_t first to avoid sign-extension when char >= 0x80
    uint8_t lo = static_cast<uint8_t>(m_romData[offset]);
    uint8_t hi = static_cast<uint8_t>(m_romData[offset + 1]);
    return static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
}

uint32_t GBAROReader::readWord(uint32_t offset) const
{
    if (offset + 3 >= static_cast<uint32_t>(m_romData.size())) {
        return 0;
    }
    // Must cast to uint8_t first to avoid sign-extension when char >= 0x80
    uint8_t b0 = static_cast<uint8_t>(m_romData[offset]);
    uint8_t b1 = static_cast<uint8_t>(m_romData[offset + 1]);
    uint8_t b2 = static_cast<uint8_t>(m_romData[offset + 2]);
    uint8_t b3 = static_cast<uint8_t>(m_romData[offset + 3]);
    return static_cast<uint32_t>(b0) |
           (static_cast<uint32_t>(b1) << 8) |
           (static_cast<uint32_t>(b2) << 16) |
           (static_cast<uint32_t>(b3) << 24);
}

QByteArray GBAROReader::readBytes(uint32_t offset, int length) const
{
    if (offset + length > static_cast<uint32_t>(m_romData.size())) {
        return QByteArray();
    }
    return m_romData.mid(offset, length);
}

QRgb GBAROReader::gbaColorToQRgb(uint16_t gbaColor)
{
    // GBA uses 15-bit RGB555 format (little-endian): 0bbbbbgggggrrrrr
    // Bits 0-4: Red, Bits 5-9: Green, Bits 10-14: Blue
    int r = (gbaColor & 0x001F) << 3;  // 5 bits -> 8 bits
    int g = ((gbaColor & 0x03E0) >> 5) << 3;
    int b = ((gbaColor & 0x7C00) >> 10) << 3;

    // Expand 5-bit to 8-bit by duplicating upper bits
    r |= (r >> 5);
    g |= (g >> 5);
    b |= (b >> 5);

    return qRgb(r, g, b);
}

QVector<QRgb> GBAROReader::extractPalette(uint32_t offset, int colorCount)
{
    QVector<QRgb> palette;
    palette.reserve(colorCount);

    for (int i = 0; i < colorCount; ++i) {
        uint16_t gbaColor = readHalfWord(offset + i * 2);
        // Extract all colors including color 0 (don't make it transparent)
        // For sprites, color 0 is typically transparent, but for backgrounds it's a real color
        palette.append(gbaColorToQRgb(gbaColor));
    }

    return palette;
}

QImage GBAROReader::tile4bppToImage(const uint8_t *tileData, const QVector<QRgb> &palette)
{
    QImage tile(8, 8, QImage::Format_Indexed8);
    tile.setColorTable(palette);

    // 4bpp: each byte contains 2 pixels (4 bits each)
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; x += 2) {
            int byteIndex = y * 4 + x / 2;
            uint8_t byte = tileData[byteIndex];

            // Low nibble = left pixel, high nibble = right pixel
            uint8_t pixel1 = byte & 0x0F;
            uint8_t pixel2 = (byte >> 4) & 0x0F;

            tile.setPixel(x, y, pixel1);
            tile.setPixel(x + 1, y, pixel2);
        }
    }

    return tile;
}

QImage GBAROReader::extractTile4bpp(uint32_t offset, const QVector<QRgb> &palette, int width, int height)
{
    if (width % 8 != 0 || height % 8 != 0) {
        qWarning() << "Tile dimensions must be multiples of 8";
        return QImage();
    }

    int tilesX = width / 8;
    int tilesY = height / 8;
    QImage result(width, height, QImage::Format_Indexed8);
    result.setColorTable(palette);

    const uint8_t *romData = reinterpret_cast<const uint8_t*>(m_romData.constData());

    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            int tileIndex = ty * tilesX + tx;
            uint32_t tileOffset = offset + tileIndex * TILE_SIZE_4BPP;

            if (tileOffset + TILE_SIZE_4BPP > static_cast<uint32_t>(m_romData.size())) {
                continue;
            }

            QImage tile = tile4bppToImage(romData + tileOffset, palette);

            // Copy tile to result image
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    result.setPixel(tx * 8 + x, ty * 8 + y, tile.pixelIndex(x, y));
                }
            }
        }
    }

    return result;
}

QImage GBAROReader::extractTileset4bpp(uint32_t offset, int tileCount, const QVector<QRgb> &palette, int tilesPerRow)
{
    int rows = (tileCount + tilesPerRow - 1) / tilesPerRow;
    int width = tilesPerRow * 8;
    int height = rows * 8;

    return extractTile4bpp(offset, palette, width, height);
}

QImage GBAROReader::extractPokemonIcon(uint16_t iconIndex)
{
    if (!isLoaded()) {
        return QImage();
    }

    qDebug() << "extractPokemonIcon: iconIndex =" << iconIndex;
    qDebug() << "  m_iconSprites =" << QString("0x%1").arg(m_iconSprites, 0, 16);
    qDebug() << "  m_iconPalettes =" << QString("0x%1").arg(m_iconPalettes, 0, 16);
    qDebug() << "  m_iconPaletteIndices =" << QString("0x%1").arg(m_iconPaletteIndices, 0, 16);

    // Icon sprites table contains 4-byte pointers to icon data
    uint32_t iconPtrOffset = m_iconSprites + iconIndex * 4;
    uint32_t iconPtr = readWord(iconPtrOffset);

    qDebug() << "  iconPtrOffset =" << QString("0x%1").arg(iconPtrOffset, 0, 16);
    qDebug() << "  iconPtr (raw) =" << QString("0x%1").arg(iconPtr, 0, 16);

    // Convert GBA address to ROM offset
    if (iconPtr < 0x08000000 || iconPtr > 0x09FFFFFF) {
        qWarning() << "Invalid icon pointer for index" << iconIndex << "ptr:" << QString("0x%1").arg(iconPtr, 0, 16);
        return QImage();
    }
    uint32_t iconOffset = iconPtr - 0x08000000;
    qDebug() << "  iconOffset (ROM) =" << QString("0x%1").arg(iconOffset, 0, 16);

    // Read palette index for this Pokemon (0-2, there are only 3 icon palettes)
    uint8_t paletteIndex = readByte(m_iconPaletteIndices + iconIndex);
    qDebug() << "  paletteIndex =" << paletteIndex;
    if (paletteIndex > 2) {
        paletteIndex = 0;  // Fallback to first palette
    }

    // Calculate palette offset (3 palettes, 32 bytes each)
    uint32_t paletteOffset = m_iconPalettes + paletteIndex * 32;
    qDebug() << "  paletteOffset =" << QString("0x%1").arg(paletteOffset, 0, 16);
    QVector<QRgb> palette = extractPalette(paletteOffset, 16);

    // Icons are 32x64 (2 frames stacked), extract first frame (32x32)
    return extractTile4bpp(iconOffset, palette, ICON_SIZE, ICON_SIZE);
}

QImage GBAROReader::extractFont()
{
    // Extract main font - used for Wonder Cards
    // For FRLG: FONT_NORMAL_COPY_2 (index 3)
    // For Emerald: FONT_SHORT_COPY_1 (index 3)
    // 8x16 character cells, 2bpp, source layout is 2 tile columns
    // Need 512 characters to cover all positions (positions go up to ~492 for 'ü')
    if (m_fontOffset == 0) {
        qWarning() << "Font offset not set";
        return QImage();
    }
    return extractFont2bpp(m_fontOffset, 512, 8, 16, 2);
}

QImage GBAROReader::extractFontByIndex(int fontIndex)
{
    // Extract a specific font by index
    // Emerald: index 1 = FONT_NORMAL (for ID), index 3 = FONT_SHORT_COPY_1 (main)
    // FRLG: index 3 = FONT_NORMAL_COPY_2
    if (!m_fontOffsets.contains(fontIndex)) {
        qWarning() << "Font index" << fontIndex << "not loaded";
        return QImage();
    }

    uint32_t offset = m_fontOffsets[fontIndex];
    if (offset == 0) {
        qWarning() << "Font offset for index" << fontIndex << "is zero";
        return QImage();
    }

    qDebug() << "Extracting font index" << fontIndex << "at offset" << QString("0x%1").arg(offset, 0, 16);
    return extractFont2bpp(offset, 512, 8, 16, 2);
}

QVector<QVector<uint8_t>> GBAROReader::decode2bppTiles(uint32_t offset, int tileColumns, int tileRows)
{
    // Decode 2bpp graphics data from ROM (GBA linear format)
    // Each 8x8 tile is 16 bytes
    // Each byte contains 4 pixels (2 bits each):
    //   bits 6-7 = pixel 0, bits 4-5 = pixel 1, bits 2-3 = pixel 2, bits 0-1 = pixel 3
    // Returns a 2D array of pixel values (0-3)

    int sourceWidth = tileColumns * 8;
    int sourceHeight = tileRows * 8;

    QVector<QVector<uint8_t>> result(sourceWidth, QVector<uint8_t>(sourceHeight, 0));

    for (int yTile = 0; yTile < tileRows; ++yTile) {
        for (int xTile = 0; xTile < tileColumns; ++xTile) {
            int xOffset = xTile * 8;
            int yOffset = yTile * 8;
            uint32_t tileStart = offset + (yTile * tileColumns + xTile) * 16;

            for (int i = 0; i < 16; ++i) {
                int xx = i % 2;
                int yy = i / 2;
                xx = 1 - xx;  // Flip horizontally within tile pair

                uint8_t raw = readByte(tileStart + i);

                // Extract 4 pixels from each byte (2 bits each)
                result[xOffset + xx * 4 + 0][yOffset + yy] = (raw >> 6) & 3;
                result[xOffset + xx * 4 + 1][yOffset + yy] = (raw >> 4) & 3;
                result[xOffset + xx * 4 + 2][yOffset + yy] = (raw >> 2) & 3;
                result[xOffset + xx * 4 + 3][yOffset + yy] = (raw >> 0) & 3;
            }
        }
    }

    return result;
}

QImage GBAROReader::extractFont2bpp(uint32_t offset, int numChars, int charWidth, int charHeight, int sourceTileColumns)
{
    if (!isLoaded()) {
        qWarning() << "ROM not loaded";
        return QImage();
    }

    // Calculate source layout
    int tilesWide = charWidth / 8;
    int tilesTall = charHeight / 8;
    int bytesPerChar = tilesWide * tilesTall * 16;  // 16 bytes per 8x8 tile at 2bpp
    int totalSize = numChars * bytesPerChar;

    // Calculate tile dimensions
    int totalTiles = totalSize / 16;
    int tileHeight = totalTiles / sourceTileColumns;

    // Decode 2bpp pixels
    QVector<QVector<uint8_t>> pixels = decode2bppTiles(offset, sourceTileColumns, tileHeight);

    int sourceWidth = pixels.size();
    int sourceHeight = pixels.isEmpty() ? 0 : pixels[0].size();

    // Calculate output layout (32 characters per row for latin fonts)
    int outputCharsPerRow = 32;
    int outputRows = (numChars + outputCharsPerRow - 1) / outputCharsPerRow;
    int outputWidth = outputCharsPerRow * charWidth;
    int outputHeight = outputRows * charHeight;

    // Source characters per row
    int sourceCharsPerRow = sourceWidth / charWidth;

    // Create indexed image with default 2bpp palette
    QImage result(outputWidth, outputHeight, QImage::Format_Indexed8);

    // Set up default palette (can be recolored later)
    QVector<QRgb> palette;
    palette.append(qRgba(144, 200, 255, 0));   // Color 0: Transparent background
    palette.append(qRgb(56, 56, 56));          // Color 1: Dark gray (foreground)
    palette.append(qRgb(216, 216, 216));       // Color 2: Light gray (shadow)
    palette.append(qRgba(255, 255, 255, 0));   // Color 3: Transparent (glyph bg)
    // Pad to 256 colors
    for (int i = 4; i < 256; ++i) {
        palette.append(qRgb(0, 0, 0));
    }
    result.setColorTable(palette);
    result.fill(0);  // Fill with transparent

    // Reorganize characters from source layout to output layout
    for (int charPos = 0; charPos < numChars; ++charPos) {
        // Position in source
        int sourceCharX = (charPos % sourceCharsPerRow) * charWidth;
        int sourceCharY = (charPos / sourceCharsPerRow) * charHeight;

        // Position in output
        int outputGridX = charPos % outputCharsPerRow;
        int outputGridY = charPos / outputCharsPerRow;

        // Copy character pixels
        for (int py = 0; py < charHeight; ++py) {
            for (int px = 0; px < charWidth; ++px) {
                int srcX = sourceCharX + px;
                int srcY = sourceCharY + py;

                if (srcX < sourceWidth && srcY < sourceHeight) {
                    uint8_t pixelVal = pixels[srcX][srcY];

                    // Remap color 3 to color 0 (both are background/transparent)
                    if (pixelVal == 3) {
                        pixelVal = 0;
                    }

                    int dstX = outputGridX * charWidth + px;
                    int dstY = outputGridY * charHeight + py;

                    if (dstX < outputWidth && dstY < outputHeight) {
                        result.setPixel(dstX, dstY, pixelVal);
                    }
                }
            }
        }
    }

    qDebug() << "Extracted 2bpp font:" << numChars << "characters,"
             << outputWidth << "x" << outputHeight << "px";

    return result;
}

QVector<uint8_t> GBAROReader::extractGlyphWidths(uint32_t offset, int count)
{
    QVector<uint8_t> widths;
    widths.reserve(count);

    for (int i = 0; i < count; ++i) {
        widths.append(readByte(offset + i));
    }

    return widths;
}

QVector<uint8_t> GBAROReader::getDefaultGlyphWidths()
{
    if (!isLoaded() || m_glyphWidthsOffset == 0) {
        // Return default width of 6 for all characters
        QVector<uint8_t> defaults(512, 6);
        return defaults;
    }

    return extractGlyphWidths(m_glyphWidthsOffset, GLYPH_WIDTHS_SIZE);
}

QVector<uint8_t> GBAROReader::getGlyphWidthsByIndex(int fontIndex)
{
    // Get glyph widths for a specific font index
    if (!isLoaded() || !m_glyphWidthOffsets.contains(fontIndex)) {
        qWarning() << "Glyph widths not available for font index" << fontIndex;
        QVector<uint8_t> defaults(512, 6);
        return defaults;
    }

    uint32_t offset = m_glyphWidthOffsets[fontIndex];
    if (offset == 0) {
        QVector<uint8_t> defaults(512, 6);
        return defaults;
    }

    qDebug() << "Extracting glyph widths for font index" << fontIndex << "at offset" << QString("0x%1").arg(offset, 0, 16);
    return extractGlyphWidths(offset, GLYPH_WIDTHS_SIZE);
}

QByteArray GBAROReader::decompressLZ77(uint32_t offset, QString &errorMessage) const
{
    if (offset + 4 > static_cast<uint32_t>(m_romData.size())) {
        errorMessage = "Offset out of bounds";
        return QByteArray();
    }

    // Read header
    uint8_t compressionType = readByte(offset);
    if (compressionType != 0x10) {
        errorMessage = QString("Not LZ77 compressed data (expected 0x10, got 0x%1)")
            .arg(compressionType, 2, 16, QChar('0'));
        return QByteArray();
    }

    // Read decompressed size (3 bytes, little endian)
    uint32_t decompressedSize = readByte(offset + 1) |
                                (readByte(offset + 2) << 8) |
                                (readByte(offset + 3) << 16);

    if (decompressedSize == 0 || decompressedSize > 0x100000) {  // Max 1MB sanity check
        errorMessage = QString("Invalid decompressed size: %1").arg(decompressedSize);
        return QByteArray();
    }

    QByteArray decompressed;
    decompressed.reserve(decompressedSize);

    uint32_t srcPos = offset + 4;  // Start after header
    const uint8_t *src = reinterpret_cast<const uint8_t*>(m_romData.constData());

    while (decompressed.size() < static_cast<int>(decompressedSize)) {
        if (srcPos >= static_cast<uint32_t>(m_romData.size())) {
            errorMessage = "Unexpected end of compressed data";
            return QByteArray();
        }

        // Read flag byte
        uint8_t flags = src[srcPos++];

        // Process 8 blocks
        for (int i = 0; i < 8; ++i) {
            if (decompressed.size() >= static_cast<int>(decompressedSize)) {
                break;  // Done
            }

            if (srcPos >= static_cast<uint32_t>(m_romData.size())) {
                errorMessage = "Unexpected end of compressed data";
                return QByteArray();
            }

            if (flags & 0x80) {
                // Compressed block - read 2 bytes
                if (srcPos + 1 >= static_cast<uint32_t>(m_romData.size())) {
                    errorMessage = "Unexpected end of compressed data";
                    return QByteArray();
                }

                uint8_t byte1 = src[srcPos++];
                uint8_t byte2 = src[srcPos++];

                // Extract length and displacement
                int length = ((byte1 >> 4) & 0x0F) + 3;  // 3-18 bytes
                int displacement = (((byte1 & 0x0F) << 8) | byte2) + 1;  // 1-4096

                // Copy from earlier in the decompressed buffer
                int copyPos = decompressed.size() - displacement;
                if (copyPos < 0) {
                    errorMessage = "Invalid LZ77 displacement";
                    return QByteArray();
                }

                for (int j = 0; j < length; ++j) {
                    if (decompressed.size() >= static_cast<int>(decompressedSize)) {
                        break;
                    }
                    decompressed.append(decompressed[copyPos + j]);
                }
            } else {
                // Uncompressed byte
                decompressed.append(src[srcPos++]);
            }

            flags <<= 1;  // Next flag bit
        }
    }

    return decompressed;
}

QImage GBAROReader::extractWonderCardFrame()
{
    // Note: Frame extraction requires version-specific offsets not yet in YAML
    // This function is currently unused by the main application
    qWarning() << "extractWonderCardFrame: Not implemented for dynamic ROM support";
    return QImage();
}

uint32_t readPointer(const QByteArray &data, uint32_t offset)
{
    if (offset + 4 > static_cast<uint32_t>(data.size())) {
        return 0;
    }

    // Cast to uint8_t first, then to uint32_t for safe bit operations
    uint8_t b0 = static_cast<uint8_t>(data[offset]);
    uint8_t b1 = static_cast<uint8_t>(data[offset + 1]);
    uint8_t b2 = static_cast<uint8_t>(data[offset + 2]);
    uint8_t b3 = static_cast<uint8_t>(data[offset + 3]);
    uint32_t ptr = static_cast<uint32_t>(b0) |
                   (static_cast<uint32_t>(b1) << 8) |
                   (static_cast<uint32_t>(b2) << 16) |
                   (static_cast<uint32_t>(b3) << 24);

    // Convert GBA address to ROM offset
    if (ptr >= 0x08000000 && ptr <= 0x09FFFFFF) {
        return ptr - 0x08000000;
    }

    return 0;
}

GBAROReader::WonderCardGraphicsEntry GBAROReader::loadWonderCardEntry(int index)
{
    WonderCardGraphicsEntry entry = {0, 0, 0, 0};

    qDebug() << "loadWonderCardEntry: index =" << index;
    qDebug() << "  m_wondercardTable =" << QString("0x%1").arg(m_wondercardTable, 0, 16);
    qDebug() << "  m_wondercardCount =" << m_wondercardCount;

    if (!isLoaded() || index < 0 || index >= m_wondercardCount) {
        qWarning() << "  Invalid index or not loaded";
        return entry;
    }

    uint32_t entryOffset = m_wondercardTable + (index * 16);
    qDebug() << "  entryOffset =" << QString("0x%1").arg(entryOffset, 0, 16);

    entry.tilesetPtr = readPointer(m_romData, entryOffset);
    entry.tilemapPtr = readPointer(m_romData, entryOffset + 4);
    entry.palettePtr = readPointer(m_romData, entryOffset + 8);

    qDebug() << "  tilesetPtr =" << QString("0x%1").arg(entry.tilesetPtr, 0, 16);
    qDebug() << "  tilemapPtr =" << QString("0x%1").arg(entry.tilemapPtr, 0, 16);
    qDebug() << "  palettePtr =" << QString("0x%1").arg(entry.palettePtr, 0, 16);

    return entry;
}

QVector<GBAROReader::TilemapEntry> GBAROReader::loadTilemap(const QByteArray &data)
{
    QVector<TilemapEntry> tilemap;

    for (int i = 0; i + 1 < data.size(); i += 2) {
        // Cast to uint8_t first, then to uint16_t for safe bit operations
        uint8_t lo = static_cast<uint8_t>(data[i]);
        uint8_t hi = static_cast<uint8_t>(data[i + 1]);
        uint16_t value = static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);

        TilemapEntry entry;
        entry.tileIndex = value & 0x3FF;
        entry.hFlip = (value & 0x400) != 0;
        entry.vFlip = (value & 0x800) != 0;
        entry.paletteIndex = (value >> 12) & 0xF;

        tilemap.append(entry);
    }

    return tilemap;
}

QImage GBAROReader::renderWonderCard(const WonderCardGraphicsEntry &entry)
{
    if (!isLoaded() || entry.tilesetPtr == 0 || entry.tilemapPtr == 0 || entry.palettePtr == 0) {
        qWarning() << "Invalid Wonder Card entry";
        return QImage();
    }

    QString error;

    // Decompress tileset
    QByteArray tilesetData = decompressLZ77(entry.tilesetPtr, error);
    if (tilesetData.isEmpty()) {
        qWarning() << "Failed to decompress tileset:" << error;
        return QImage();
    }

    // Decompress tilemap
    QByteArray tilemapData = decompressLZ77(entry.tilemapPtr, error);
    if (tilemapData.isEmpty()) {
        qWarning() << "Failed to decompress tilemap:" << error;
        return QImage();
    }

    // Load palette
    QVector<QRgb> palette = extractPalette(entry.palettePtr, 16);

    // Parse tilemap
    QVector<TilemapEntry> tilemap = loadTilemap(tilemapData);

    // Create tilesheet image
    int numTiles = tilesetData.size() / TILE_SIZE_4BPP;
    int tileSheetWidth = 16;
    int tileSheetHeight = (numTiles + tileSheetWidth - 1) / tileSheetWidth;

    // Wonder Card is 30x20 tiles (240x160 pixels)
    const int CARD_TILES_WIDE = 30;
    const int CARD_TILES_TALL = 20;

    QImage result(CARD_TILES_WIDE * 8, CARD_TILES_TALL * 8, QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    const uint8_t *tileData = reinterpret_cast<const uint8_t*>(tilesetData.constData());

    for (int ty = 0; ty < CARD_TILES_TALL; ++ty) {
        for (int tx = 0; tx < CARD_TILES_WIDE; ++tx) {
            int mapIndex = ty * CARD_TILES_WIDE + tx;
            if (mapIndex >= tilemap.size()) continue;

            const TilemapEntry &mapEntry = tilemap[mapIndex];
            int tileNum = mapEntry.tileIndex;

            if (tileNum >= numTiles) continue;

            // Decode this tile
            const uint8_t *currentTile = tileData + tileNum * TILE_SIZE_4BPP;
            QImage tileImg = tile4bppToImage(currentTile, palette);

            // Apply flips (using Qt6 flipped() instead of deprecated mirrored())
            if (mapEntry.hFlip) {
                tileImg = tileImg.flipped(Qt::Horizontal);
            }
            if (mapEntry.vFlip) {
                tileImg = tileImg.flipped(Qt::Vertical);
            }

            // Convert to ARGB32 for compositing
            QImage tileArgb = tileImg.convertToFormat(QImage::Format_ARGB32);

            // Copy tile to result
            for (int py = 0; py < 8; ++py) {
                for (int px = 0; px < 8; ++px) {
                    int destX = tx * 8 + px;
                    int destY = ty * 8 + py;
                    result.setPixel(destX, destY, tileArgb.pixel(px, py));
                }
            }
        }
    }

    return result;
}

QImage GBAROReader::extractWonderCardBackground(int index)
{
    if (!isLoaded()) {
        qWarning() << "ROM not loaded";
        return QImage();
    }

    if (index < 0 || index >= m_wondercardCount) {
        qWarning() << "Invalid Wonder Card index:" << index;
        return QImage();
    }

    WonderCardGraphicsEntry entry = loadWonderCardEntry(index);
    return renderWonderCard(entry);
}

// Gen3 character encoding table for name decoding
static const QHash<uint8_t, QString>& getGen3Charset()
{
    static QHash<uint8_t, QString> charset = {
        {0x00, " "},
        // Numbers
        {0xA1, "0"}, {0xA2, "1"}, {0xA3, "2"}, {0xA4, "3"}, {0xA5, "4"},
        {0xA6, "5"}, {0xA7, "6"}, {0xA8, "7"}, {0xA9, "8"}, {0xAA, "9"},
        // Punctuation
        {0xAB, "!"}, {0xAC, "?"}, {0xAD, "."}, {0xAE, "-"},
        // Uppercase letters
        {0xBB, "A"}, {0xBC, "B"}, {0xBD, "C"}, {0xBE, "D"}, {0xBF, "E"},
        {0xC0, "F"}, {0xC1, "G"}, {0xC2, "H"}, {0xC3, "I"}, {0xC4, "J"},
        {0xC5, "K"}, {0xC6, "L"}, {0xC7, "M"}, {0xC8, "N"}, {0xC9, "O"},
        {0xCA, "P"}, {0xCB, "Q"}, {0xCC, "R"}, {0xCD, "S"}, {0xCE, "T"},
        {0xCF, "U"}, {0xD0, "V"}, {0xD1, "W"}, {0xD2, "X"}, {0xD3, "Y"},
        {0xD4, "Z"},
        // Lowercase letters
        {0xD5, "a"}, {0xD6, "b"}, {0xD7, "c"}, {0xD8, "d"}, {0xD9, "e"},
        {0xDA, "f"}, {0xDB, "g"}, {0xDC, "h"}, {0xDD, "i"}, {0xDE, "j"},
        {0xDF, "k"}, {0xE0, "l"}, {0xE1, "m"}, {0xE2, "n"}, {0xE3, "o"},
        {0xE4, "p"}, {0xE5, "q"}, {0xE6, "r"}, {0xE7, "s"}, {0xE8, "t"},
        {0xE9, "u"}, {0xEA, "v"}, {0xEB, "w"}, {0xEC, "x"}, {0xED, "y"},
        {0xEE, "z"},
        // Special
        {0xB4, "'"}, {0xB8, ","}, {0xBA, "/"}, {0xF0, ":"},
    };
    return charset;
}

QString GBAROReader::decodeGen3String(const QByteArray &data) const
{
    QString result;
    const QHash<uint8_t, QString> &charset = getGen3Charset();

    for (int i = 0; i < data.size(); i++) {
        uint8_t byte = static_cast<uint8_t>(data[i]);
        if (byte == 0xFF) break;  // String terminator

        QString ch = charset.value(byte, "");
        if (!ch.isEmpty()) {
            result += ch;
        }
    }

    return result.trimmed();
}

QString GBAROReader::getItemName(uint16_t id) const
{
    if (!m_hasNameTables || id >= static_cast<uint16_t>(m_itemTable.count)) {
        return QString();
    }

    // Check cache first
    if (m_itemNameCache.contains(id)) {
        return m_itemNameCache[id];
    }

    // Read item name from ROM
    uint32_t offset = m_itemTable.offset + (id * m_itemTable.entrySize);
    if (offset + m_itemTable.nameLength > static_cast<uint32_t>(m_romData.size())) {
        return QString();
    }

    QByteArray nameData = m_romData.mid(offset, m_itemTable.nameLength);
    QString name = decodeGen3String(nameData);

    // Convert to ITEM_UPPERCASE format
    name = name.toUpper().replace(" ", "").replace(".", "");

    m_itemNameCache[id] = name;
    return name;
}

QStringList GBAROReader::getAllItemNames() const
{
    QStringList items;
    if (!m_hasNameTables) {
        return items;
    }

    for (int i = 0; i < m_itemTable.count; ++i) {
        QString name = getItemName(static_cast<uint16_t>(i));
        if (!name.isEmpty()) {
            items << name;
        } else {
            items << QString("ITEM_%1").arg(i, 4, 16, QChar('0')).toUpper();
        }
    }
    return items;
}

QString GBAROReader::getPokemonName(uint16_t id) const
{
    if (!m_hasNameTables || id >= static_cast<uint16_t>(m_pokemonTable.count)) {
        return QString();
    }

    // Check cache first
    if (m_pokemonNameCache.contains(id)) {
        return m_pokemonNameCache[id];
    }

    // Read pokemon name from ROM
    uint32_t offset = m_pokemonTable.offset + (id * m_pokemonTable.entrySize);
    if (offset + m_pokemonTable.entrySize > static_cast<uint32_t>(m_romData.size())) {
        return QString();
    }

    QByteArray nameData = m_romData.mid(offset, m_pokemonTable.entrySize);
    QString name = decodeGen3String(nameData);

    m_pokemonNameCache[id] = name;
    return name;
}

QString GBAROReader::getMoveName(uint16_t id) const
{
    if (!m_hasNameTables || id >= static_cast<uint16_t>(m_moveTable.count)) {
        return QString();
    }

    // Check cache first
    if (m_moveNameCache.contains(id)) {
        return m_moveNameCache[id];
    }

    // Read move name from ROM
    uint32_t offset = m_moveTable.offset + (id * m_moveTable.entrySize);
    if (offset + m_moveTable.entrySize > static_cast<uint32_t>(m_romData.size())) {
        return QString();
    }

    QByteArray nameData = m_romData.mid(offset, m_moveTable.entrySize);
    QString name = decodeGen3String(nameData);

    m_moveNameCache[id] = name;
    return name;
}

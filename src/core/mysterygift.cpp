#include "mysterygift.h"
#include <cstring>

// Complete Gen 3 character encoding table (International/English)
// Maps Gen 3 byte values (0x00-0xFF) to Unicode characters
// Source: Data Crystal TBL file & https://github.com/zeon256/gen3-charset
// CORRECTED MAPPINGS - digits at 0xA1-0xAA, uppercase at 0xBB-0xD4, lowercase at 0xD5-0xEE
static const QChar GEN3_TO_UNICODE[256] = {
    // 0x00-0x0F: Space and accented letters
    QChar(' '),    QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x00C7), QChar(0x00C8), QChar(0x00C9), QChar(0x00CA),
    QChar(0x00CB), QChar(0x00CC), QChar(' '),    QChar(0x00CE), QChar(0x00CF), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4),
    // 0x10-0x1F: More accented letters
    QChar(0x0152), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00D1), QChar(0x00DF), QChar(0x00E0), QChar(0x00E1),
    QChar(0x0000), QChar(0x00E7), QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x0000),
    // 0x20-0x2F: More accented, special symbols
    QChar(0x00EE), QChar(0x00EF), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x0153), QChar(0x00F9), QChar(0x00FA),
    QChar(0x00FB), QChar(0x00F1), QChar(0x00BA), QChar(0x00AA), QChar(0x1D49), QChar(0x0026), QChar(0x002B), QChar(0x0000),
    // 0x30-0x3F: Spaces, Lv, =, ;
    QChar(0x0000), QChar('L'),     QChar('v'),     QChar(0x003D), QChar(0x003B), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    // 0x40-0x4F: Symbols, PK, MN
    QChar(0x0000), QChar(0x00BF), QChar(0x00A1), QChar('P'),     QChar('K'),     QChar('M'),     QChar('N'),     QChar(0x0000),
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x00CD), QChar(0x0025), QChar(0x0028), QChar(0x0029), QChar(0x0000),
    // 0x50-0x5F: Spaces, arrows, â, í
    QChar(0x0000), QChar(0x00E2), QChar(0x0000), QChar(0x00ED), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    QChar(0x0000), QChar(0x0000), QChar(0x2191), QChar(0x2193), QChar(0x2190), QChar(0x2192), QChar(0x0000), QChar(0x0000),
    // 0x60-0x6F: Asterisks, superscripts
    QChar(0x002A), QChar(0x002A), QChar(0x002A), QChar(0x002A), QChar(0x1D49), QChar(0x003C), QChar(0x003E), QChar(0x0000),
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    // 0x70-0x7F: Spaces, arrows, asterisks
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    QChar(0x0000), QChar(0x2191), QChar(0x2193), QChar(0x2190), QChar(0x2192), QChar('*'),     QChar('*'),     QChar('*'),
    // 0x80-0x8F: Asterisks, superscript, spaces
    QChar('*'),     QChar('*'),     QChar('*'),     QChar('*'),     QChar(0x1D49), QChar('<'),     QChar('>'),     QChar(0x0000),
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    // 0x90-0x9F: Spaces
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000),
    // 0xA0-0xAF: CORRECTED - null, digits 0-9, punctuation
    QChar(0x0000), QChar('0'),     QChar('1'),     QChar('2'),     QChar('3'),     QChar('4'),     QChar('5'),     QChar('6'),
    QChar('7'),     QChar('8'),     QChar('9'),     QChar('!'),     QChar('?'),     QChar('.'),     QChar('-'),     QChar(0x30FB),
    // 0xB0-0xBF: CORRECTED - Ellipsis, quotes, gender, punctuation, /, A-E
    QChar(0x2025), QChar(0x201C), QChar(0x201D), QChar(0x2018), QChar(0x2019), QChar(0x2642), QChar(0x2640), QChar(' '),
    QChar(','),     QChar(0x00D7), QChar('/'),     QChar('A'),     QChar('B'),     QChar('C'),     QChar('D'),     QChar('E'),
    // 0xC0-0xCF: CORRECTED - Uppercase F-U
    QChar('F'),     QChar('G'),     QChar('H'),     QChar('I'),     QChar('J'),     QChar('K'),     QChar('L'),     QChar('M'),
    QChar('N'),     QChar('O'),     QChar('P'),     QChar('Q'),     QChar('R'),     QChar('S'),     QChar('T'),     QChar('U'),
    // 0xD0-0xDF: CORRECTED - Uppercase V-Z, lowercase a-j
    QChar('V'),     QChar('W'),     QChar('X'),     QChar('Y'),     QChar('Z'),     QChar('a'),     QChar('b'),     QChar('c'),
    QChar('d'),     QChar('e'),     QChar('f'),     QChar('g'),     QChar('h'),     QChar('i'),     QChar('j'),     QChar('k'),
    // 0xE0-0xEF: CORRECTED - Lowercase l-z, ▶
    QChar('l'),     QChar('m'),     QChar('n'),     QChar('o'),     QChar('p'),     QChar('q'),     QChar('r'),     QChar('s'),
    QChar('t'),     QChar('u'),     QChar('v'),     QChar('w'),     QChar('x'),     QChar('y'),     QChar('z'),     QChar(0x25BA),
    // 0xF0-0xFF: CORRECTED - Colon, umlauts, control characters
    QChar(':'),     QChar(0x00C4), QChar(0x00D6), QChar(0x00DC), QChar(0x00E4), QChar(0x00F6), QChar(0x00FC), QChar(0x0000),
    QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000), QChar(0x0000)
};

WonderCardData MysteryGift::parseWonderCard(const QByteArray &payload)
{
    WonderCardData data;

    if (payload.size() < WONDERCARD_PAYLOAD_SIZE) {
        // Return empty data if payload is too small
        data.eventId = 0;
        data.icon = 0;
        data.count = 0;
        data.typeColorResend = 0;
        data.stampMax = 0;
        return data;
    }

    const uint8_t *bytes = reinterpret_cast<const uint8_t*>(payload.constData());

    // Skip 4-byte CRC header if present (336-byte format vs 332-byte payload)
    if (payload.size() == WONDERCARD_TOTAL_SIZE) {
        bytes += WONDERCARD_HEADER_SIZE;
    }

    // Read numeric fields (little-endian)
    data.eventId = bytes[WonderCardOffsets::EVENT_ID] |
                  (bytes[WonderCardOffsets::EVENT_ID + 1] << 8);

    // Keep raw icon value - display code handles clamping/visibility
    data.icon = bytes[WonderCardOffsets::ICON] |
               (bytes[WonderCardOffsets::ICON + 1] << 8);

    data.count = bytes[WonderCardOffsets::COUNT] |
                (bytes[WonderCardOffsets::COUNT + 1] << 8) |
                (bytes[WonderCardOffsets::COUNT + 2] << 16) |
                (bytes[WonderCardOffsets::COUNT + 3] << 24);

    data.typeColorResend = bytes[WonderCardOffsets::TYPE_COLOR_RESEND];
    data.stampMax = bytes[WonderCardOffsets::STAMP_MAX];

    // Read text fields
    data.title = decodeText(bytes + WonderCardOffsets::TITLE, TEXT_FIELD_SIZE);
    data.subtitle = decodeText(bytes + WonderCardOffsets::SUBTITLE, TEXT_FIELD_SIZE);
    data.contentLine1 = decodeText(bytes + WonderCardOffsets::CONTENT_LINE_1, TEXT_FIELD_SIZE);
    data.contentLine2 = decodeText(bytes + WonderCardOffsets::CONTENT_LINE_2, TEXT_FIELD_SIZE);
    data.contentLine3 = decodeText(bytes + WonderCardOffsets::CONTENT_LINE_3, TEXT_FIELD_SIZE);
    data.contentLine4 = decodeText(bytes + WonderCardOffsets::CONTENT_LINE_4, TEXT_FIELD_SIZE);
    data.warningLine1 = decodeText(bytes + WonderCardOffsets::WARNING_LINE_1, TEXT_FIELD_SIZE);
    data.warningLine2 = decodeText(bytes + WonderCardOffsets::WARNING_LINE_2, TEXT_FIELD_SIZE);

    return data;
}

QByteArray MysteryGift::encodeWonderCard(const WonderCardData &data)
{
    QByteArray payload(WONDERCARD_PAYLOAD_SIZE, 0x00);
    uint8_t *bytes = reinterpret_cast<uint8_t*>(payload.data());

    // Write numeric fields (little-endian)
    bytes[WonderCardOffsets::EVENT_ID] = data.eventId & 0xFF;
    bytes[WonderCardOffsets::EVENT_ID + 1] = (data.eventId >> 8) & 0xFF;

    bytes[WonderCardOffsets::ICON] = data.icon & 0xFF;
    bytes[WonderCardOffsets::ICON + 1] = (data.icon >> 8) & 0xFF;

    bytes[WonderCardOffsets::COUNT] = data.count & 0xFF;
    bytes[WonderCardOffsets::COUNT + 1] = (data.count >> 8) & 0xFF;
    bytes[WonderCardOffsets::COUNT + 2] = (data.count >> 16) & 0xFF;
    bytes[WonderCardOffsets::COUNT + 3] = (data.count >> 24) & 0xFF;

    bytes[WonderCardOffsets::TYPE_COLOR_RESEND] = data.typeColorResend;
    bytes[WonderCardOffsets::STAMP_MAX] = data.stampMax;

    // Write text fields
    encodeText(bytes + WonderCardOffsets::TITLE, data.title, TEXT_FIELD_SIZE);
    encodeText(bytes + WonderCardOffsets::SUBTITLE, data.subtitle, TEXT_FIELD_SIZE);
    encodeText(bytes + WonderCardOffsets::CONTENT_LINE_1, data.contentLine1, TEXT_FIELD_SIZE);
    encodeText(bytes + WonderCardOffsets::CONTENT_LINE_2, data.contentLine2, TEXT_FIELD_SIZE);
    encodeText(bytes + WonderCardOffsets::CONTENT_LINE_3, data.contentLine3, TEXT_FIELD_SIZE);
    encodeText(bytes + WonderCardOffsets::CONTENT_LINE_4, data.contentLine4, TEXT_FIELD_SIZE);
    encodeText(bytes + WonderCardOffsets::WARNING_LINE_1, data.warningLine1, TEXT_FIELD_SIZE);
    encodeText(bytes + WonderCardOffsets::WARNING_LINE_2, data.warningLine2, TEXT_FIELD_SIZE);

    return payload;
}

uint16_t MysteryGift::calculateCRC16(const QByteArray &data, const QByteArray &crcTable)
{
    if (crcTable.size() != 512) {
        return 0; // Invalid table
    }

    const uint8_t *bytes = reinterpret_cast<const uint8_t*>(data.constData());
    const uint8_t *table = reinterpret_cast<const uint8_t*>(crcTable.constData());

    // CRC-16/XMODEM with seed 0x1121
    uint16_t crc = 0x1121;

    for (int i = 0; i < data.size(); ++i) {
        uint8_t tableIndex = (crc ^ bytes[i]) & 0xFF;
        // Read 16-bit value from table (little-endian)
        uint16_t tableValue = table[tableIndex * 2] | (table[tableIndex * 2 + 1] << 8);
        crc = tableValue ^ (crc >> 8);
    }

    // Final XOR (bitwise NOT)
    return ~crc & 0xFFFF;
}

QString MysteryGift::decodeText(const uint8_t *data, int maxLength)
{
    QString result;
    for (int i = 0; i < maxLength; ++i) {
        uint8_t byte = data[i];

        if (byte == 0xFF) {
            // String terminator - stop reading
            break;
        }

        QChar ch = GEN3_TO_UNICODE[byte];
        if (ch.unicode() != 0x0000) {
            // Valid character - add to result
            result.append(ch);
        } else if (byte == 0x00 || byte == 0xA0) {
            // Explicit space/padding (0x00 and 0xA0 are both used as space/null in Gen3)
            result.append(' ');
        } else if (byte == 0xFE) {
            // Gen3 newline/line break - treat as space for single-line display
            result.append(' ');
        } else if (byte == 0xFA || byte == 0xFB || byte == 0xFC || byte == 0xFD) {
            // Gen3 control codes (scroll, wait, highlight, placeholder) - treat as space
            result.append(' ');
        }
        // Skip other unmapped control characters
    }

    return result;  // Don't trim - preserve spaces
}

// Helper function: Find Gen 3 byte value for a given Unicode character
static uint8_t unicodeToGen3(QChar ch)
{
    uint16_t unicode = ch.unicode();

    // IMPORTANT: Search the standard character ranges FIRST to avoid
    // finding duplicate characters at wrong positions.
    // Letters P, K, M, N appear at 0x43-0x46 (PKMN special) AND at
    // their proper positions in the alphabet (0xCA, 0xC5, 0xC7, 0xC8).
    // We must prefer the alphabet positions.

    // Priority 1: Digits 0-9 at 0xA1-0xAA
    if (unicode >= '0' && unicode <= '9') {
        return 0xA1 + (unicode - '0');
    }

    // Priority 2: Uppercase A-Z at 0xBB-0xD4
    if (unicode >= 'A' && unicode <= 'Z') {
        return 0xBB + (unicode - 'A');
    }

    // Priority 3: Lowercase a-z at 0xD5-0xEE
    if (unicode >= 'a' && unicode <= 'z') {
        return 0xD5 + (unicode - 'a');
    }

    // Priority 4: Common punctuation
    if (unicode == '!') return 0xAB;
    if (unicode == '?') return 0xAC;
    if (unicode == '.') return 0xAD;
    if (unicode == '-') return 0xAE;
    if (unicode == ',') return 0xB8;
    if (unicode == '/') return 0xBA;
    if (unicode == ':') return 0xF0;
    if (unicode == ' ') return 0x00;

    // Fallback: Search the full lookup table for other characters
    for (int i = 0; i < 256; ++i) {
        if (GEN3_TO_UNICODE[i].unicode() == unicode) {
            return static_cast<uint8_t>(i);
        }
    }

    // Character not found - return space/padding
    return 0x00;
}

void MysteryGift::encodeText(uint8_t *dest, const QString &text, int maxLength)
{
    // Initialize with zeros (padding)
    // Gen3 Wonder Cards use 0x00 padding after text, NOT 0xFF terminators
    std::memset(dest, 0x00, maxLength);

    int writePos = 0;
    for (int i = 0; i < text.length() && writePos < maxLength; ++i) {
        QChar ch = text[i];

        // Lookup character in encoding table
        uint8_t gen3Byte = unicodeToGen3(ch);

        // Write the byte (even if it's 0x00 for unmappable chars)
        dest[writePos++] = gen3Byte;
    }

    // Note: No 0xFF terminator added - the 0x00 padding serves as implicit terminator
    // Adding 0xFF would corrupt data since original cards use 0x00 padding
}

/**
 * @file mysterygift.h
 * @brief Mystery Gift / Wonder Card data structures and utilities.
 *
 * This file defines the data structures and utility functions for parsing
 * and encoding Pokemon Generation III Mystery Gift data (Wonder Cards).
 *
 * ## Wonder Card Structure (International/US version)
 *
 * Wonder Cards are 332-byte data blocks containing event information:
 * - **Header** (bytes 0-9): Event ID, icon, count, type/color flags
 * - **Text Fields** (bytes 10-329): Title, subtitle, content lines, warnings
 *   - Each text field is 40 bytes in Gen3 encoding
 *   - Terminated by 0xFF bytes
 *
 * Storage in save files:
 * - Located in Section 4 of the save file
 * - FRLG offset: 0x460 (with 4-byte CRC header)
 * - Emerald offset: 0x56C (with 4-byte CRC header)
 *
 * ## GREEN MAN Script Structure
 *
 * Scripts are 1000-byte blocks containing the event delivery logic:
 * - Bytecode executed by the game's script engine
 * - Controls item delivery, flag setting, NPC interactions
 * - Preceded by 4-byte CRC header
 *
 * ## CRC16 Algorithm
 *
 * Both Wonder Cards and Scripts use CCITT CRC-16:
 * - Polynomial: 0x1021
 * - Initial seed: 0x1121
 * - Uses 256-entry lookup table (loaded from tab.bin)
 *
 * ## Gen3 Text Encoding
 *
 * Pokemon Gen3 uses a proprietary character encoding:
 * - Single-byte characters with values 0x00-0xFF
 * - 0xFF = string terminator
 * - 0xFE = newline/text break
 * - Special characters: 0xFB = player name, 0xFA = special codes
 *
 * @see savefile.h for save file access
 * @see WonderCardRenderer for visual rendering
 *
 * @author ComradeSean
 * @version 1.0
 */

#ifndef MYSTERYGIFT_H
#define MYSTERYGIFT_H

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QString>
#include <QByteArray>
#include <cstdint>

// Wonder Card field offsets (within 332-byte payload)
namespace WonderCardOffsets {
    const int EVENT_ID = 0x00;              // u16 - Event identifier
    const int ICON = 0x02;                  // u16 - Pokemon icon display
    const int COUNT = 0x04;                 // u32 - Distribution counter
    const int TYPE_COLOR_RESEND = 0x08;     // u8 - Type/Color/Resend flags
    const int STAMP_MAX = 0x09;             // u8 - Maximum stamp value
    const int TITLE = 0x0A;                 // 40 bytes - Card title text
    const int SUBTITLE = 0x32;              // 40 bytes - Subtitle text
    const int CONTENT_LINE_1 = 0x5A;        // 40 bytes - Content line 1
    const int CONTENT_LINE_2 = 0x82;        // 40 bytes - Content line 2
    const int CONTENT_LINE_3 = 0xAA;        // 40 bytes - Content line 3
    const int CONTENT_LINE_4 = 0xD2;        // 40 bytes - Content line 4
    const int WARNING_LINE_1 = 0xFA;        // 40 bytes - Warning line 1
    const int WARNING_LINE_2 = 0x122;       // 40 bytes - Warning line 2
}

// Wonder Card icon values
namespace WonderCardIcon {
    const uint16_t BULBASAUR = 0x0001;
    const uint16_t DEOXYS = 0x00F9;
    const uint16_t QUESTION_MARK = 0xFFFF;
}

// Wonder Card type values (in TYPE_COLOR_RESEND byte)
enum class WonderCardType : uint8_t {
    Event = 0,
    Stamp = 1,
    Counter = 2
};

/// <summary>
/// Parsed Wonder Card data structure.
/// Provides convenient access to all Wonder Card fields.
/// </summary>
struct WonderCardData {
    uint16_t eventId;
    uint16_t icon;
    uint32_t count;
    uint8_t typeColorResend;
    uint8_t stampMax;

    QString title;
    QString subtitle;
    QString contentLine1;
    QString contentLine2;
    QString contentLine3;
    QString contentLine4;
    QString warningLine1;
    QString warningLine2;

    // Derived fields
    WonderCardType type() const { return static_cast<WonderCardType>(typeColorResend & 0x03); }
    uint8_t color() const { return (typeColorResend >> 2) & 0x07; }
    bool canResend() const { return (typeColorResend & 0x40) != 0; }

    // Check if Wonder Card appears empty/uninitialized
    bool isEmpty() const { return eventId == 0 && icon == 0; }
};

/// <summary>
/// Mystery Gift utilities for parsing and encoding Wonder Card data.
/// </summary>
class MysteryGift
{
public:
    // Constants
    static const qint64 WONDERCARD_PAYLOAD_SIZE = 332;
    static const qint64 WONDERCARD_HEADER_SIZE = 4;     // CRC16 + Padding
    static const qint64 WONDERCARD_TOTAL_SIZE = 336;    // Header + Payload
    static const qint64 GMSCRIPT_PAYLOAD_SIZE = 1000;
    static const qint64 GMSCRIPT_HEADER_SIZE = 4;
    static const qint64 GMSCRIPT_TOTAL_SIZE = 1004;

    // Text field sizes
    static const int TEXT_FIELD_SIZE = 40;  // International version

    // Parse Wonder Card from 332-byte payload
    static WonderCardData parseWonderCard(const QByteArray &payload);

    // Encode Wonder Card to 332-byte payload
    static QByteArray encodeWonderCard(const WonderCardData &data);

    // Calculate CRC16 checksum
    static uint16_t calculateCRC16(const QByteArray &data, const QByteArray &crcTable);

    // Decode text from Pokemon Gen 3 encoding to QString
    static QString decodeText(const uint8_t *data, int maxLength);

    // Encode text from QString to Pokemon Gen 3 encoding
    static void encodeText(uint8_t *dest, const QString &text, int maxLength);
};

#endif // MYSTERYGIFT_H

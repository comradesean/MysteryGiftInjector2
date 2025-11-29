/**
 * @file savefile.h
 * @brief Pokemon Generation III save file parsing and modification.
 *
 * This file provides the SaveFile class which handles all save file operations
 * for Pokemon FireRed, LeafGreen, Ruby, Sapphire, and Emerald.
 *
 * ## Save File Structure
 * Gen3 save files are 128KB (131072 bytes) with two save slots:
 * - Slot A: Offset 0x00000 - 0x0DFFF (14 sections x 4KB)
 * - Slot B: Offset 0x0E000 - 0x1BFFF (14 sections x 4KB)
 * - Hall of Fame: Offset 0x1C000 - 0x1FFFF
 *
 * Each section has a footer containing:
 * - Section ID (0xFF4): Which of the 14 sections this is
 * - Checksum (0xFF6): 16-bit sum of bytes 0-0xFF3
 * - Signature (0xFF8): Magic number 0x08012025
 * - Save Index (0xFFC): Counter incremented each save
 *
 * The active save slot is the one with the higher save index.
 *
 * ## Mystery Gift Data
 * Wonder Card and Script data are stored in Section 4:
 * - FRLG: Wonder Card at 0x460, Script at 0x3A0
 * - Emerald: Wonder Card at 0x56C, Script at 0x3B0
 *
 * ## Checksum Algorithm
 * Each section uses a simple 16-bit additive checksum:
 * - Sum all bytes 0x000 to 0xFF3 as 32-bit values
 * - Add upper and lower 16-bit halves
 * - Store result at 0xFF6
 *
 * @see mysterygift.h for Wonder Card data structures
 *
 * @author ComradeSean
 * @version 1.0
 */

#ifndef SAVEFILE_H
#define SAVEFILE_H

// =============================================================================
// Qt Framework Includes
// =============================================================================
#include <QString>
#include <QByteArray>
#include <cstdint>

// =============================================================================
// Project Includes
// =============================================================================
#include "mysterygift.h"

enum class GameType {
    Unknown,
    FireRedLeafGreen,
    RubySapphire,
    Emerald
};

struct SaveBlockInfo {
    size_t blockIndex;      // Which save slot (0 or 1)
    uint32_t saveIndex;     // Save counter (higher = more recent)
    bool valid;             // All checksums valid?
    uint32_t gameCode;      // Game identifier at offset 0xAC
    bool hasSecurityKey;    // Non-zero security key (Emerald feature)
};

// Options for Wonder Card injection (mirrors game's ClearSavedWonderCardAndRelated behavior)
struct InjectionOptions {
    bool clearMetadata = true;      // Clear WonderCardMetadata (always recommended)
    bool clearTrainerIds = false;   // Clear saved trainer IDs for battles/trades
    bool clearMysteryGiftFlags = false;  // Clear Mystery Gift event flags
    bool clearMysteryGiftVars = false;   // Clear Mystery Gift variables
};

class SaveFile
{
public:
    SaveFile();
    ~SaveFile();

    // File operations
    bool loadFromFile(const QString &path, QString &errorMessage);
    bool saveToFile(const QString &path, bool makeBackup, QString &errorMessage);

    // Game detection
    GameType detectGameType();
    QString gameTypeToString(GameType type) const;

    // Validation
    bool validateChecksums();
    bool isLoaded() const { return !m_bytes.isEmpty(); }

    // Accessors
    QString filePath() const { return m_filePath; }
    qint64 fileSize() const { return m_bytes.size(); }
    GameType detectedGame() const { return m_detectedGame; }
    bool checksumValid() const { return m_checksumValid; }
    const QByteArray& bytes() const { return m_bytes; }

    // Mystery Gift / Wonder Card operations
    bool hasWonderCard() const;
    WonderCardData extractWonderCard(QString &errorMessage) const;
    QByteArray extractWonderCardRaw(QString &errorMessage) const;
    QByteArray extractScript(QString &errorMessage) const;
    bool injectWonderCard(const WonderCardData &wonderCard, const QByteArray &scriptData,
                          const QByteArray &crcTable, QString &errorMessage,
                          const QByteArray &rawWonderCardData = QByteArray(),
                          const InjectionOptions &options = InjectionOptions());

    // Mystery Gift flag operations
    bool isMysteryGiftEnabled() const;  // Check if flag is already set
    bool enableMysteryGift(QString &errorMessage);  // Set the flag (updates checksum internally)

    // Constants
    static const qint64 EXPECTED_FILE_SIZE = 131072; // 128 KB
    static const size_t SECTION_SIZE = 0x1000;       // 4 KB
    static const size_t SECTIONS_PER_SAVE = 14;

    // Section footer offsets (relative to section start)
    static const size_t SECTION_ID_OFFSET = 0xFF4;   // 2 bytes
    static const size_t CHECKSUM_OFFSET = 0xFF6;     // 2 bytes
    static const size_t SIGNATURE_OFFSET = 0xFF8;    // 4 bytes (magic: 0x08012025)
    static const size_t SAVE_INDEX_OFFSET = 0xFFC;   // 4 bytes (save counter)

    // Section checksum data lengths (varies by game and section)
    // Most sections use 0xF80 bytes, but Section 4 varies:
    static const size_t CHECKSUM_DATA_LENGTH_DEFAULT = 0xF80;  // Default for most sections
    static const size_t CHECKSUM_DATA_LENGTH_SECTION4_FRLG = 0xEE8;     // 3816 bytes
    static const size_t CHECKSUM_DATA_LENGTH_SECTION4_EMERALD = 0xF08;  // 3848 bytes

    // Game code offset
    static const size_t GAME_CODE_OFFSET = 0xAC;

    // Wonder Card constants
    static const uint8_t WONDERCARD_BLOCK_MARKER = 0x04;
    static const size_t WONDERCARD_OFFSET_FRLG = 0x460;
    static const size_t WONDERCARD_OFFSET_EMERALD = 0x56C;
    static const size_t GMSCRIPT_OFFSET_FRLG = 0x79C;
    static const size_t GMSCRIPT_OFFSET_EMERALD = 0x8A8;

    // WonderCardMetadata structure (from decomp):
    // struct WonderCardMetadata {
    //     u16 battlesWon;      // offset 0x00
    //     u16 battlesLost;     // offset 0x02
    //     u16 numTrades;       // offset 0x04
    //     u16 iconSpecies;     // offset 0x06 - IMPORTANT: game reads icon from here for display
    //     u16 stampData[2][MAX_STAMP_CARD_STAMPS];  // stamp tracking
    // };
    // Note: When game saves WonderCard legitimately, it copies iconSpecies from
    // WonderCard to WonderCardMetadata. We must do the same during injection.
    static const size_t WCMETADATA_OFFSET_FRLG = 0x5B4;    // 0x460 + 336 + 4 (after WC + CRC)
    static const size_t WCMETADATA_OFFSET_EMERALD = 0x6C0;  // 0x56C + 336 + 4
    static const size_t WCMETADATA_ICON_OFFSET = 6;         // iconSpecies offset within metadata
    static const size_t WCMETADATA_SIZE = 32;               // Size to clear (includes stampData)

    // TrainerIds array (from decomp): u32 trainerIds[2][5] = 40 bytes
    // Stores IDs for 10 trainers - 5 for battles, 5 for trades
    // Located at end of MysteryGiftSave structure, before GMScript
    static const size_t TRAINERIDS_OFFSET_FRLG = 0x75C;     // 0x79C - 40
    static const size_t TRAINERIDS_OFFSET_EMERALD = 0x868;  // 0x8A8 - 40
    static const size_t TRAINERIDS_SIZE = 40;               // 10 * sizeof(u32)

    // RamScript structure (from decomp):
    // struct RamScript {
    //     u8 magic;       // offset 0x00 - Must be RAM_SCRIPT_MAGIC (51) for valid script
    //     u8 mapGroup;    // offset 0x01 - Map group where script triggers (0xFF = any)
    //     u8 mapNum;      // offset 0x02 - Map number where script triggers (0xFF = any)
    //     u8 objectId;    // offset 0x03 - Object ID that triggers script (0xFF = none)
    //     u8 script[996]; // offset 0x04 - Actual script bytecode (996 bytes)
    // };
    // Total: 1000 bytes (+ 4-byte CRC header = 1004 bytes in save)
    // Note: Game's InitRamScript() sets magic=51 after zeroing the structure.
    // ValidateRamScript() checks magic==51 before executing.
    static const uint8_t RAM_SCRIPT_MAGIC = 51;             // 0x33 - Required for valid script
    static const size_t RAMSCRIPT_MAGIC_OFFSET = 0;         // Offset within RamScript payload
    static const size_t RAMSCRIPT_SIZE = 1000;              // Size of RamScript structure
    static const size_t GMSCRIPT_SIZE_WITH_CRC = 1004;      // 4-byte CRC header + 1000-byte payload

    // Mystery Gift enable flag offsets (in Section 2)
    static const size_t MYSTERY_GIFT_OFFSET_FRLG = 0x067;      // Bit 1 (0x02)
    static const uint8_t MYSTERY_GIFT_BIT_FRLG = 0x02;
    static const size_t MYSTERY_GIFT_OFFSET_EMERALD = 0x40B;   // Bit 3 (0x08)
    static const uint8_t MYSTERY_GIFT_BIT_EMERALD = 0x08;

private:
    // Helper functions
    uint16_t computeSectionChecksum(const uint8_t* data, size_t length);
    SaveBlockInfo analyzeSaveSlot(size_t slotIndex);
    int findActiveSaveSlot();
    int findWonderCardBlock() const;
    int findSection(int sectionId) const;  // Find physical position of a section by ID
    size_t getSection4ChecksumLength() const;  // Get game-specific checksum length for Section 4
    size_t getSectionChecksumLength(int sectionId) const;  // Get checksum length for any section

    // Member variables
    QString m_filePath;
    QByteArray m_bytes;
    GameType m_detectedGame;
    bool m_checksumValid;
    int m_activeSaveSlot;
};

#endif // SAVEFILE_H

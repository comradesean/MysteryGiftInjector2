#include "savefile.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

SaveFile::SaveFile()
    : m_detectedGame(GameType::Unknown)
    , m_checksumValid(false)
    , m_activeSaveSlot(-1)
{
}

SaveFile::~SaveFile()
{
}

bool SaveFile::loadFromFile(const QString &path, QString &errorMessage)
{
    // Clear previous data
    m_bytes.clear();
    m_filePath.clear();
    m_detectedGame = GameType::Unknown;
    m_checksumValid = false;
    m_activeSaveSlot = -1;

    // Check if file exists
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        errorMessage = "File does not exist: " + path;
        return false;
    }

    // Open file
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = "Failed to open file: " + file.errorString();
        return false;
    }

    // Read all bytes
    m_bytes = file.readAll();
    file.close();

    // Validate file size
    if (m_bytes.size() != EXPECTED_FILE_SIZE) {
        errorMessage = QString("Invalid file size: %1 bytes (expected %2 bytes)")
                          .arg(m_bytes.size())
                          .arg(EXPECTED_FILE_SIZE);
        m_bytes.clear();
        return false;
    }

    // Store file path
    m_filePath = path;

    // Detect game type
    m_detectedGame = detectGameType();

    // Validate checksums
    m_checksumValid = validateChecksums();

    return true;
}

bool SaveFile::saveToFile(const QString &path, bool makeBackup, QString &errorMessage)
{
    if (!isLoaded()) {
        errorMessage = "No save file loaded";
        return false;
    }

    // Create backup if requested
    if (makeBackup) {
        QFileInfo fileInfo(path);
        QString backupPath = fileInfo.absolutePath() + "/" +
                           fileInfo.baseName() + ".bak";

        if (QFile::exists(path)) {
            // Remove old backup if it exists
            if (QFile::exists(backupPath)) {
                QFile::remove(backupPath);
            }
            // Copy current file to backup
            if (!QFile::copy(path, backupPath)) {
                errorMessage = "Failed to create backup file";
                return false;
            }
        }
    }

    // Write to temp file first (atomic write)
    QString tempPath = path + ".tmp";
    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly)) {
        errorMessage = "Failed to create temporary file: " + file.errorString();
        return false;
    }

    qint64 bytesWritten = file.write(m_bytes);
    file.close();

    if (bytesWritten != m_bytes.size()) {
        errorMessage = "Failed to write complete file";
        QFile::remove(tempPath);
        return false;
    }

    // Remove original and rename temp file
    if (QFile::exists(path)) {
        if (!QFile::remove(path)) {
            errorMessage = "Failed to remove original file";
            QFile::remove(tempPath);
            return false;
        }
    }

    if (!QFile::rename(tempPath, path)) {
        errorMessage = "Failed to rename temporary file";
        return false;
    }

    m_filePath = path;
    return true;
}

uint16_t SaveFile::computeSectionChecksum(const uint8_t* data, size_t length)
{
    // Algorithm from Project 1
    size_t words = length / 4;
    uint32_t sum = 0;

    for (size_t i = 0; i < words; ++i) {
        // Little-endian 32-bit word
        uint32_t word = data[i*4] |
                       (data[i*4+1] << 8) |
                       (data[i*4+2] << 16) |
                       (data[i*4+3] << 24);
        sum += word;
    }

    // Fold high 16 bits into low 16 bits
    sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(sum & 0xFFFF);
}

SaveBlockInfo SaveFile::analyzeSaveSlot(size_t slotIndex)
{
    SaveBlockInfo info;
    info.blockIndex = slotIndex;
    info.valid = true;
    info.saveIndex = 0;
    info.gameCode = 0;
    info.hasSecurityKey = false;

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());
    size_t baseOffset = slotIndex * (SECTION_SIZE * SECTIONS_PER_SAVE);

    // Validate all 14 sections
    for (size_t section = 0; section < SECTIONS_PER_SAVE; ++section) {
        size_t sectionOffset = baseOffset + (section * SECTION_SIZE);
        const uint8_t* sectionData = saveData + sectionOffset;

        // Compute checksum (use default length for validation before game type is known)
        // Note: Per-section lengths vary, but most use 0xF80 so this is reasonable for validation
        uint16_t computed = computeSectionChecksum(sectionData, CHECKSUM_DATA_LENGTH_DEFAULT);

        // Read stored checksum (little-endian)
        uint16_t stored = sectionData[CHECKSUM_OFFSET] |
                         (sectionData[CHECKSUM_OFFSET + 1] << 8);

        if (computed != stored) {
            info.valid = false;
        }

        // Read section ID
        uint16_t sectionId = sectionData[SECTION_ID_OFFSET] |
                            (sectionData[SECTION_ID_OFFSET + 1] << 8);

        // Read save index from each section (should be same for all)
        uint32_t saveIdx = sectionData[SAVE_INDEX_OFFSET] |
                          (sectionData[SAVE_INDEX_OFFSET + 1] << 8) |
                          (sectionData[SAVE_INDEX_OFFSET + 2] << 16) |
                          (sectionData[SAVE_INDEX_OFFSET + 3] << 24);
        if (section == 0) {
            info.saveIndex = saveIdx;
        }

        // Section 0 contains game code
        if (sectionId == 0) {
            info.gameCode = sectionData[GAME_CODE_OFFSET] |
                           (sectionData[GAME_CODE_OFFSET + 1] << 8) |
                           (sectionData[GAME_CODE_OFFSET + 2] << 16) |
                           (sectionData[GAME_CODE_OFFSET + 3] << 24);

            // Check security key (offset 0xAC + 4, non-zero for Emerald)
            uint32_t securityKey = sectionData[GAME_CODE_OFFSET + 4] |
                                  (sectionData[GAME_CODE_OFFSET + 5] << 8) |
                                  (sectionData[GAME_CODE_OFFSET + 6] << 16) |
                                  (sectionData[GAME_CODE_OFFSET + 7] << 24);
            info.hasSecurityKey = (securityKey != 0);
        }
    }

    return info;
}

int SaveFile::findActiveSaveSlot()
{
    if (!isLoaded()) {
        return -1;
    }

    SaveBlockInfo slot0 = analyzeSaveSlot(0);
    SaveBlockInfo slot1 = analyzeSaveSlot(1);

    // Prefer valid save over invalid
    if (slot0.valid && !slot1.valid) return 0;
    if (!slot0.valid && slot1.valid) return 1;

    // If both valid or both invalid, use higher save index
    return (slot0.saveIndex > slot1.saveIndex) ? 0 : 1;
}

GameType SaveFile::detectGameType()
{
    if (!isLoaded()) {
        return GameType::Unknown;
    }

    // Find active save slot
    m_activeSaveSlot = findActiveSaveSlot();
    if (m_activeSaveSlot < 0) {
        return GameType::Unknown;
    }

    SaveBlockInfo activeSlot = analyzeSaveSlot(m_activeSaveSlot);

    // Game detection logic from Project 1
    if (activeSlot.gameCode == 0x00000001) {
        // Can't distinguish FireRed from LeafGreen by game code alone
        return GameType::FireRedLeafGreen;
    }
    else if (activeSlot.hasSecurityKey && activeSlot.gameCode != 0x00000001) {
        return GameType::Emerald;
    }
    else if (activeSlot.gameCode == 0x00000000) {
        // Ruby or Sapphire - can't distinguish
        return GameType::RubySapphire;
    }

    return GameType::Unknown;
}

bool SaveFile::validateChecksums()
{
    if (!isLoaded()) {
        return false;
    }

    // Validate active save slot
    if (m_activeSaveSlot < 0) {
        m_activeSaveSlot = findActiveSaveSlot();
    }

    if (m_activeSaveSlot < 0) {
        return false;
    }

    SaveBlockInfo activeSlot = analyzeSaveSlot(m_activeSaveSlot);
    return activeSlot.valid;
}

QString SaveFile::gameTypeToString(GameType type) const
{
    switch (type) {
        case GameType::FireRedLeafGreen: return "Pokémon FireRed/LeafGreen";
        case GameType::RubySapphire:     return "Pokémon Ruby/Sapphire";
        case GameType::Emerald:          return "Pokémon Emerald";
        case GameType::Unknown:
        default:                         return "Unknown";
    }
}

int SaveFile::findWonderCardBlock() const
{
    if (!isLoaded() || m_activeSaveSlot < 0) {
        return -1;
    }

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);

    // Search through all 14 sections for Wonder Card block (marker 0x04)
    for (size_t section = 0; section < SECTIONS_PER_SAVE; ++section) {
        size_t sectionOffset = slotOffset + (section * SECTION_SIZE);
        uint8_t sectionId = saveData[sectionOffset + SECTION_ID_OFFSET];

        if (sectionId == WONDERCARD_BLOCK_MARKER) {
            return static_cast<int>(section);
        }
    }

    return -1;  // Wonder Card block not found
}

bool SaveFile::hasWonderCard() const
{
    if (!isLoaded()) {
        return false;
    }

    int wonderCardBlock = findWonderCardBlock();
    if (wonderCardBlock < 0) {
        return false;
    }

    // Get game-specific offset
    size_t wonderCardOffset = (m_detectedGame == GameType::Emerald) ?
                              WONDERCARD_OFFSET_EMERALD : WONDERCARD_OFFSET_FRLG;

    // Calculate absolute offset in file
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);
    size_t blockOffset = slotOffset + (wonderCardBlock * SECTION_SIZE);
    size_t dataOffset = blockOffset + wonderCardOffset;

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());

    // Check if Wonder Card has data (CRC16 header at offset + 0 should be non-zero if active)
    uint16_t crc = saveData[dataOffset] | (saveData[dataOffset + 1] << 8);

    return crc != 0;
}

WonderCardData SaveFile::extractWonderCard(QString &errorMessage) const
{
    WonderCardData emptyCard;

    if (!isLoaded()) {
        errorMessage = "No save file loaded";
        return emptyCard;
    }

    if (m_detectedGame == GameType::Unknown) {
        errorMessage = "Unknown game type - cannot extract Wonder Card";
        return emptyCard;
    }

    if (m_detectedGame == GameType::RubySapphire) {
        errorMessage = "Ruby/Sapphire do not support Wonder Cards";
        return emptyCard;
    }

    // Find Wonder Card block
    int wonderCardBlock = findWonderCardBlock();
    if (wonderCardBlock < 0) {
        errorMessage = "Wonder Card block not found in save file";
        return emptyCard;
    }

    // Get game-specific offset
    size_t wonderCardOffset = (m_detectedGame == GameType::Emerald) ?
                              WONDERCARD_OFFSET_EMERALD : WONDERCARD_OFFSET_FRLG;

    // Calculate absolute offset
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);
    size_t blockOffset = slotOffset + (wonderCardBlock * SECTION_SIZE);
    size_t dataOffset = blockOffset + wonderCardOffset + 4;  // Skip CRC16 + padding header

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());

    // Extract 332-byte Wonder Card payload
    QByteArray payload(reinterpret_cast<const char*>(saveData + dataOffset), 332);

    // Parse using MysteryGift class
    WonderCardData wonderCard = MysteryGift::parseWonderCard(payload);

    return wonderCard;
}

QByteArray SaveFile::extractWonderCardRaw(QString &errorMessage) const
{
    QByteArray emptyPayload;

    if (!isLoaded()) {
        errorMessage = "No save file loaded";
        return emptyPayload;
    }

    if (m_detectedGame == GameType::Unknown) {
        errorMessage = "Unknown game type - cannot extract Wonder Card";
        return emptyPayload;
    }

    if (m_detectedGame == GameType::RubySapphire) {
        errorMessage = "Ruby/Sapphire do not support Wonder Cards";
        return emptyPayload;
    }

    // Find Wonder Card block
    int wonderCardBlock = findWonderCardBlock();
    if (wonderCardBlock < 0) {
        errorMessage = "Wonder Card block not found in save file";
        return emptyPayload;
    }

    // Get game-specific offset
    size_t wonderCardOffset = (m_detectedGame == GameType::Emerald) ?
                              WONDERCARD_OFFSET_EMERALD : WONDERCARD_OFFSET_FRLG;

    // Calculate absolute offset
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);
    size_t blockOffset = slotOffset + (wonderCardBlock * SECTION_SIZE);
    size_t dataOffset = blockOffset + wonderCardOffset;  // Include CRC header

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());

    // Extract full 336-byte Wonder Card (4-byte CRC header + 332-byte payload)
    QByteArray rawData(reinterpret_cast<const char*>(saveData + dataOffset), 336);

    return rawData;
}

QByteArray SaveFile::extractScript(QString &errorMessage) const
{
    QByteArray emptyScript;

    if (!isLoaded()) {
        errorMessage = "No save file loaded";
        return emptyScript;
    }

    if (m_detectedGame == GameType::Unknown) {
        errorMessage = "Unknown game type - cannot extract script";
        return emptyScript;
    }

    if (m_detectedGame == GameType::RubySapphire) {
        errorMessage = "Ruby/Sapphire scripts are different format";
        return emptyScript;
    }

    // Find Wonder Card block (script is in same section)
    int wonderCardBlock = findWonderCardBlock();
    if (wonderCardBlock < 0) {
        errorMessage = "Wonder Card block not found in save file";
        return emptyScript;
    }

    // Get game-specific offset for script
    size_t scriptOffset = (m_detectedGame == GameType::Emerald) ?
                          GMSCRIPT_OFFSET_EMERALD : GMSCRIPT_OFFSET_FRLG;

    // Calculate absolute offset
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);
    size_t blockOffset = slotOffset + (wonderCardBlock * SECTION_SIZE);
    size_t dataOffset = blockOffset + scriptOffset + 4;  // Skip CRC16 + padding header

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());

    // Extract 1000-byte script payload
    QByteArray script(reinterpret_cast<const char*>(saveData + dataOffset), 1000);

    return script;
}

bool SaveFile::injectWonderCard(const WonderCardData &wonderCard, const QByteArray &scriptData,
                                const QByteArray &crcTable, QString &errorMessage,
                                const QByteArray &rawWonderCardData,
                                const InjectionOptions &options)
{
    if (!isLoaded()) {
        errorMessage = "No save file loaded";
        return false;
    }

    if (m_detectedGame == GameType::Unknown) {
        errorMessage = "Unknown game type - cannot inject Wonder Card";
        return false;
    }

    if (m_detectedGame == GameType::RubySapphire) {
        errorMessage = "Ruby/Sapphire do not support Wonder Cards";
        return false;
    }

    // Find Wonder Card block
    int wonderCardBlock = findWonderCardBlock();
    if (wonderCardBlock < 0) {
        errorMessage = "Wonder Card block not found in save file";
        return false;
    }

    // Get game-specific offsets
    size_t wonderCardOffset = (m_detectedGame == GameType::Emerald) ?
                              WONDERCARD_OFFSET_EMERALD : WONDERCARD_OFFSET_FRLG;
    size_t scriptOffset = (m_detectedGame == GameType::Emerald) ?
                          GMSCRIPT_OFFSET_EMERALD : GMSCRIPT_OFFSET_FRLG;
    size_t metadataOffset = (m_detectedGame == GameType::Emerald) ?
                            WCMETADATA_OFFSET_EMERALD : WCMETADATA_OFFSET_FRLG;
    size_t trainerIdsOffset = (m_detectedGame == GameType::Emerald) ?
                              TRAINERIDS_OFFSET_EMERALD : TRAINERIDS_OFFSET_FRLG;

    // Calculate absolute offsets
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);
    size_t blockOffset = slotOffset + (wonderCardBlock * SECTION_SIZE);

    uint8_t* saveData = reinterpret_cast<uint8_t*>(m_bytes.data());
    uint8_t* blockData = saveData + blockOffset;

    // Clear WonderCardMetadata first (mirrors game's ClearSavedWonderCardAndRelated behavior)
    // This zeros battlesWon, battlesLost, numTrades, iconSpecies, stampData
    if (options.clearMetadata) {
        memset(blockData + metadataOffset, 0x00, WCMETADATA_SIZE);
        // Also clear the metadata CRC (4 bytes before metadata)
        memset(blockData + metadataOffset - 4, 0x00, 4);
    }

    // Clear TrainerIds if requested (40 bytes = 10 trainer IDs)
    if (options.clearTrainerIds) {
        memset(blockData + trainerIdsOffset, 0x00, TRAINERIDS_SIZE);
    }

    // Get Wonder Card payload - prefer raw data to avoid encoding artifacts
    QByteArray wonderCardPayload;
    if (!rawWonderCardData.isEmpty()) {
        // Use raw data - strip 4-byte CRC header if present (336->332 bytes)
        if (rawWonderCardData.size() == 336) {
            wonderCardPayload = rawWonderCardData.mid(4, 332);
        } else if (rawWonderCardData.size() == 332) {
            wonderCardPayload = rawWonderCardData;
        } else {
            // Fallback to encoding from struct
            wonderCardPayload = MysteryGift::encodeWonderCard(wonderCard);
        }
    } else {
        // No raw data provided - encode from struct
        wonderCardPayload = MysteryGift::encodeWonderCard(wonderCard);
    }

    // Calculate Wonder Card CRC16
    uint16_t wonderCardCRC = MysteryGift::calculateCRC16(wonderCardPayload, crcTable);

    // Write Wonder Card: CRC16 + padding + payload
    // Note: Do NOT write FF FF terminator after payload - the game interprets
    // non-zero bytes at offset +336 as "no card present" indicator
    size_t wcOffset = wonderCardOffset;
    blockData[wcOffset] = wonderCardCRC & 0xFF;
    blockData[wcOffset + 1] = (wonderCardCRC >> 8) & 0xFF;
    blockData[wcOffset + 2] = 0x00;  // Padding
    blockData[wcOffset + 3] = 0x00;  // Padding
    memcpy(blockData + wcOffset + 4, wonderCardPayload.constData(), 332);

    // Write GMScript if provided
    // Script can be 1000 bytes (payload only) or 1004 bytes (with CRC header)
    // Following decomp's InitRamScript() logic:
    //   1. Zero the RamScript area
    //   2. Set magic = RAM_SCRIPT_MAGIC (51)
    //   3. Set mapGroup, mapNum, objectId
    //   4. Copy script bytecode
    QByteArray scriptPayload;
    if (scriptData.size() == 1004) {
        // Strip 4-byte CRC header to get 1000-byte payload
        scriptPayload = scriptData.mid(4, 1000);
    } else if (scriptData.size() == 1000) {
        scriptPayload = scriptData;
    }

    if (scriptPayload.size() == 1000) {
        // Make a mutable copy to ensure magic number is set (following decomp logic)
        QByteArray scriptToWrite = scriptPayload;

        // Verify/set magic number at offset 0 (decomp: gSaveBlock1Ptr->gmScript.magic = RAM_SCRIPT_MAGIC)
        // This ensures the script will be recognized as valid by ValidateRamScript()
        if (static_cast<uint8_t>(scriptToWrite[RAMSCRIPT_MAGIC_OFFSET]) != RAM_SCRIPT_MAGIC) {
            // Script doesn't have magic number - set it (shouldn't happen with valid presets)
            scriptToWrite[RAMSCRIPT_MAGIC_OFFSET] = static_cast<char>(RAM_SCRIPT_MAGIC);
        }

        // Calculate CRC16 on the (potentially modified) script payload
        uint16_t scriptCRC = MysteryGift::calculateCRC16(scriptToWrite, crcTable);

        // Write script with CRC header (4 bytes) + payload (1000 bytes)
        blockData[scriptOffset] = scriptCRC & 0xFF;
        blockData[scriptOffset + 1] = (scriptCRC >> 8) & 0xFF;
        blockData[scriptOffset + 2] = 0x00;  // Padding
        blockData[scriptOffset + 3] = 0x00;  // Padding
        memcpy(blockData + scriptOffset + 4, scriptToWrite.constData(), RAMSCRIPT_SIZE);
    }

    // Update WonderCardMetadata.iconSpecies (the game uses this for icon display)
    // The icon value is at payload offset 2-3 (little-endian u16)
    // Note: We write this AFTER clearing metadata so the icon is preserved
    uint16_t iconSpecies = static_cast<uint8_t>(wonderCardPayload[2]) |
                          (static_cast<uint8_t>(wonderCardPayload[3]) << 8);
    size_t iconOffset = metadataOffset + WCMETADATA_ICON_OFFSET;
    blockData[iconOffset] = iconSpecies & 0xFF;
    blockData[iconOffset + 1] = (iconSpecies >> 8) & 0xFF;

    // Get section-specific checksum data length (Section 4 varies by game)
    size_t checksumDataLength = getSection4ChecksumLength();

    // Recalculate block checksum
    uint16_t blockChecksum = computeSectionChecksum(blockData, checksumDataLength);

    // Write block checksum (little-endian - Gen3 uses little-endian throughout)
    blockData[CHECKSUM_OFFSET] = blockChecksum & 0xFF;
    blockData[CHECKSUM_OFFSET + 1] = (blockChecksum >> 8) & 0xFF;

    return true;
}

int SaveFile::findSection(int sectionId) const
{
    if (!isLoaded() || m_activeSaveSlot < 0) {
        return -1;
    }

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);

    // Search through all 14 physical positions for the requested section ID
    for (size_t pos = 0; pos < SECTIONS_PER_SAVE; ++pos) {
        size_t sectionOffset = slotOffset + (pos * SECTION_SIZE);
        uint16_t id = saveData[sectionOffset + SECTION_ID_OFFSET] |
                     (saveData[sectionOffset + SECTION_ID_OFFSET + 1] << 8);

        if (id == sectionId) {
            return static_cast<int>(pos);
        }
    }

    return -1;  // Section not found
}

size_t SaveFile::getSection4ChecksumLength() const
{
    // Section 4 checksum data length varies by game
    if (m_detectedGame == GameType::Emerald) {
        return CHECKSUM_DATA_LENGTH_SECTION4_EMERALD;  // 0xF08 = 3848 bytes
    } else {
        return CHECKSUM_DATA_LENGTH_SECTION4_FRLG;     // 0xEE8 = 3816 bytes
    }
}

size_t SaveFile::getSectionChecksumLength(int sectionId) const
{
    // Section 4 has game-specific length, all others use default
    if (sectionId == 4) {
        return getSection4ChecksumLength();
    }

    // Section-specific lengths from Gen3 documentation
    // Most sections use 0xF80, except Section 0 and Section 13
    switch (sectionId) {
        case 0:
            // Trainer Info - varies by game
            if (m_detectedGame == GameType::Emerald) {
                return 0xF2C;  // 3884 bytes
            } else if (m_detectedGame == GameType::FireRedLeafGreen) {
                return 0xF24;  // 3876 bytes
            } else {
                return 0x890;  // 2192 bytes (Ruby/Sapphire)
            }
        case 13:
            return 0x7D0;  // 2000 bytes (PC Buffer I)
        default:
            return CHECKSUM_DATA_LENGTH_DEFAULT;  // 0xF80 = 3968 bytes
    }
}

bool SaveFile::isMysteryGiftEnabled() const
{
    if (!isLoaded() || m_activeSaveSlot < 0) {
        return false;
    }

    if (m_detectedGame == GameType::Unknown || m_detectedGame == GameType::RubySapphire) {
        return false;
    }

    // Find Section 2 (where Mystery Gift flag is stored)
    int section2Pos = const_cast<SaveFile*>(this)->findSection(2);
    if (section2Pos < 0) {
        return false;
    }

    // Calculate absolute offset for Section 2
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);
    size_t section2Offset = slotOffset + (section2Pos * SECTION_SIZE);

    const uint8_t* saveData = reinterpret_cast<const uint8_t*>(m_bytes.constData());
    const uint8_t* section2Data = saveData + section2Offset;

    // Get game-specific flag offset and bit
    size_t flagOffset;
    uint8_t flagBit;

    if (m_detectedGame == GameType::Emerald) {
        flagOffset = MYSTERY_GIFT_OFFSET_EMERALD;
        flagBit = MYSTERY_GIFT_BIT_EMERALD;
    } else {
        flagOffset = MYSTERY_GIFT_OFFSET_FRLG;
        flagBit = MYSTERY_GIFT_BIT_FRLG;
    }

    // Check if flag is set
    uint8_t currentValue = section2Data[flagOffset];
    return (currentValue & flagBit) != 0;
}

bool SaveFile::enableMysteryGift(QString &errorMessage)
{
    if (!isLoaded()) {
        errorMessage = "No save file loaded";
        return false;
    }

    if (m_detectedGame == GameType::Unknown) {
        errorMessage = "Unknown game type - cannot enable Mystery Gift";
        return false;
    }

    if (m_detectedGame == GameType::RubySapphire) {
        errorMessage = "Ruby/Sapphire use Mystery Event, not Mystery Gift";
        return false;
    }

    // Find Section 2 (where Mystery Gift flag is stored)
    int section2Pos = findSection(2);
    if (section2Pos < 0) {
        errorMessage = "Section 2 not found in save file";
        return false;
    }

    // Calculate absolute offset for Section 2
    size_t slotOffset = m_activeSaveSlot * (SECTION_SIZE * SECTIONS_PER_SAVE);
    size_t section2Offset = slotOffset + (section2Pos * SECTION_SIZE);

    uint8_t* saveData = reinterpret_cast<uint8_t*>(m_bytes.data());
    uint8_t* section2Data = saveData + section2Offset;

    // Get game-specific flag offset and bit
    size_t flagOffset;
    uint8_t flagBit;

    if (m_detectedGame == GameType::Emerald) {
        flagOffset = MYSTERY_GIFT_OFFSET_EMERALD;
        flagBit = MYSTERY_GIFT_BIT_EMERALD;
    } else {
        flagOffset = MYSTERY_GIFT_OFFSET_FRLG;
        flagBit = MYSTERY_GIFT_BIT_FRLG;
    }

    // Check if flag is already set
    uint8_t currentValue = section2Data[flagOffset];
    if ((currentValue & flagBit) != 0) {
        // Already enabled, no need to modify
        return true;
    }

    // Set the Mystery Gift enable bit
    section2Data[flagOffset] = currentValue | flagBit;

    // Recalculate Section 2 checksum
    size_t checksumLength = getSectionChecksumLength(2);
    uint16_t newChecksum = computeSectionChecksum(section2Data, checksumLength);

    // Write checksum (little-endian)
    section2Data[CHECKSUM_OFFSET] = newChecksum & 0xFF;
    section2Data[CHECKSUM_OFFSET + 1] = (newChecksum >> 8) & 0xFF;

    return true;
}

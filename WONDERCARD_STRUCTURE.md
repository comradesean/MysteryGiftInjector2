# Pokémon Generation III Wonder Card Structure

Complete technical documentation for Wonder Card and Green Man Script binary formats.

---

## Table of Contents
1. [Overview](#overview)
2. [Wonder Card Binary Format](#wonder-card-binary-format)
3. [Green Man Script Binary Format](#green-man-script-binary-format)
4. [Character Encoding](#character-encoding)
5. [Checksum Calculation](#checksum-calculation)
6. [Game-Specific Offsets](#game-specific-offsets)
7. [Example Data](#example-data)

---

## Overview

Mystery Gifts in Generation III games (FireRed, LeafGreen, Emerald) consist of two binary data structures:

1. **Wonder Card** (332 bytes): The event item data visible in the player's Mystery Gift menu
2. **Green Man Script** (1000 bytes): The dialog/script shown when receiving the gift

Both are stored in **Save File Section 4** (marker `0x04`) at game-specific offsets.

---

## Wonder Card Binary Format

### File Structure

```
Total Size: 336 bytes (0x150) on disk/network
├─ CRC-16 Checksum    [2 bytes]   Offset 0x000-0x001
├─ Padding            [2 bytes]   Offset 0x002-0x003
└─ Payload Data       [332 bytes] Offset 0x004-0x14F (0x14C bytes)
    ├─ Header         [10 bytes]  Offset 0x004-0x00D
    ├─ Text Fields    [320 bytes] Offset 0x00E-0x14D
    └─ Struct Align   [2 bytes]   Offset 0x14E-0x14F (ARM7 4-byte alignment)
```

### Payload Structure (332 bytes)

| Offset | Size | Type | Field Name | Description |
|--------|------|------|------------|-------------|
| 0x00 | 2 | u16 | Event ID | Unique event identifier |
| 0x02 | 2 | u16 | Icon | Pokémon icon display code |
| 0x04 | 4 | u32 | Distribution Count | Number of times distributed |
| 0x08 | 1 | u8 | Type/Color/Resend | Packed flags byte |
| 0x09 | 1 | u8 | Stamp Maximum | Max stamp value |
| 0x0A | 40 | text | Title | Card title (English: 40 bytes) |
| 0x32 | 40 | text | Subtitle | Card subtitle |
| 0x5A | 40 | text | Content Line 1 | First content line |
| 0x82 | 40 | text | Content Line 2 | Second content line |
| 0xAA | 40 | text | Content Line 3 | Third content line |
| 0xD2 | 40 | text | Content Line 4 | Fourth content line |
| 0xFA | 40 | text | Warning Line 1 | First warning message |
| 0x122 | 40 | text | Warning Line 2 | Second warning message |

**Note**: Japanese versions use 18-20 byte text fields instead of 40 bytes.

### Type/Color/Resend Byte (Offset 0x08)

```
Bit Layout: RRCCCTTT
├─ Bits 0-1 (TT): Type
│  ├─ 00 = Event
│  ├─ 01 = Stamp
│  └─ 02 = Counter
├─ Bits 2-4 (CCC): Color (1-8)
└─ Bits 6-7 (RR): Resend flags
   ├─ Bit 6 = 0x40: Can resend
   └─ Bit 7 = 0x80: Additional resend flag
```

### Icon Values

| Value | Icon |
|-------|------|
| 0x0001 | Bulbasaur |
| 0x00F9 | Deoxys |
| 0xFFFF | Question Mark (default) |

See Pokémon sprite index for full icon list.

---

## Green Man Script Binary Format

### File Structure

```
Total Size: 1004 bytes
├─ CRC-16 Checksum    [2 bytes]  Offset 0x000-0x001
├─ Padding            [2 bytes]  Offset 0x002-0x003
└─ Script Bytecode    [1000 bytes] Offset 0x004-0x3EB
```

### Script Data (1000 bytes)

The Green Man Script contains **event script bytecode** - executable commands that run when the player receives the Mystery Gift.

**Format**: Binary bytecode in Gen III script language
**Content**:
- Dialog text (with Gen III character encoding)
- Script commands (display messages, give items, etc.)
- Control flow (conditionals, jumps)
- Special effects

**Note**: This is NOT plain text - it's compiled script instructions.

### Game-Specific Behavior

- **Emerald**: Different script structure than FR/LG
- **FireRed/LeafGreen**: Share common script format

---

## Character Encoding

All text fields use **Pokémon Generation III character encoding** (Western/English).

### Encoding Rules

1. **String Terminator**: `0xFF` marks end of string
2. **Padding**: Unused space filled with `0x00`
3. **Character Set**: 256 characters mapped to Unicode
4. **Max Length**: 40 bytes per text field (international version)

### Key Character Mappings

| Hex Range | Characters |
|-----------|------------|
| 0x71-0x7A | Digits 0-9 |
| 0x8C-0xA5 | Uppercase A-Z |
| 0xA6-0xBF | Lowercase a-z |
| 0x7B | ! (exclamation) |
| 0x7C | ? (question) |
| 0x7D | . (period) |
| 0x7E | - (hyphen) |
| 0x89 | , (comma) |
| 0x00 | Space/padding |
| 0xFF | String terminator |

### Special Characters

| Hex | Character | Unicode |
|-----|-----------|---------|
| 0x86 | ♂ | U+2642 (male symbol) |
| 0x87 | ♀ | U+2640 (female symbol) |
| 0x88 | $ | U+0024 (dollar sign) |
| 0x5A | ↑ | U+2191 (up arrow) |
| 0x5B | ↓ | U+2193 (down arrow) |
| 0x5C | ← | U+2190 (left arrow) |
| 0x5D | → | U+2192 (right arrow) |

### Accented Letters

Full support for European characters:
- **French**: À, Ç, É, è, ê, etc.
- **Spanish**: Ñ, ñ, á, í, etc.
- **German**: Ä, Ö, Ü, ä, ö, ü, ß

*See `mysterygift.cpp` GEN3_TO_UNICODE table for complete mapping.*

---

## Checksum Calculation

### CRC-16 Algorithm (Wonder Card & Script)

**Type**: CRC-16/CCITT with custom parameters

```
Polynomial:     0x1021 (standard CRC-16)
Initial Value:  0x1121 (custom seed)
Final XOR:      0xFFFF (bitwise NOT of result)
Input:          Little-endian bytes
Output:         16-bit checksum (little-endian)
```

### Algorithm Steps

1. Initialize CRC = `0x1121`
2. For each byte in payload:
   ```
   tableIndex = (CRC ^ byte) & 0xFF
   tableValue = crcTable[tableIndex * 2] | (crcTable[tableIndex * 2 + 1] << 8)
   CRC = tableValue ^ (CRC >> 8)
   ```
3. Apply final XOR: `CRC = ~CRC & 0xFFFF`
4. Store as little-endian (LSB first)

### CRC Lookup Table

- **File**: `tab.bin` (512 bytes)
- **Format**: 256 entries × 2 bytes (little-endian uint16)
- **Generation**: Polynomial 0x1021, standard CRC-16 table

### Section Checksum (Save Block)

After injecting Wonder Card/Script, the entire Section 4 block checksum must be recalculated:

**Algorithm**:
1. Sum all 4-byte integers in first 0xFF4 bytes (962 integers × 4 bytes = 3848 bytes)
2. Add carry: `sum = (sum & 0xFFFF) + (sum >> 16)`
3. Byte swap: `checksum = ((sum & 0xFF) << 8) | ((sum >> 8) & 0xFF)`
4. Write to offset 0xFF6 (big-endian)

---

## Game-Specific Offsets

### Wonder Card Location in Save File

**Save Structure**:
```
Save File: 128 KB (131,072 bytes)
├─ Save Slot 1: 57,344 bytes (14 sections × 4KB)
└─ Save Slot 2: 57,344 bytes (14 sections × 4KB)
```

**Active Slot**: Determined by save counter (higher = active)

**Wonder Card Block**: Section with ID `0x04` (varies by position)

| Game | Section 4 Base | Wonder Card Offset | Script Offset |
|------|----------------|-------------------|---------------|
| FireRed | Variable | +0x460 | +0x79C |
| LeafGreen | Variable | +0x460 | +0x79C |
| Emerald | Variable | +0x56C | +0x8A8 |
| Ruby* | Variable | N/A | +0x810 |
| Sapphire* | Variable | N/A | +0x810 |

*Ruby/Sapphire support Mystery Events but not Wonder Cards

### Injection Procedure

1. Load save file (128 KB)
2. Determine active save slot (compare save counters at 0x0FFC and 0xEFFC)
3. Find Section 4 (search for section ID `0x04` in active slot)
4. Calculate absolute offset: `slotBase + (sectionIndex × 0x1000) + gameOffset`
5. Write Wonder Card (336 bytes total):
   - CRC-16 (2 bytes, little-endian)
   - Padding (2 bytes, `0x00 0x00`)
   - Payload (332 bytes, includes 2-byte end padding)
6. Write Green Man Script (if applicable)
7. Recalculate Section 4 checksum
8. Save file

---

## Example Data

### Aurora Ticket (FireRed/LeafGreen)

**Title**: `"AURORA TICKET"`
**Subtitle**: `""`
**Content Line 1**: `"Obtain the AURORA TICKET at a"`
**Content Line 2**: `"Nintendo event!"`
**Event ID**: Varies by region
**Icon**: `0x00F9` (Deoxys)

### Mystic Ticket (FireRed/LeafGreen)

**Title**: `"MYSTIC TICKET"`
**Subtitle**: `""`
**Content Line 1**: `"Obtain the MYSTIC TICKET at a"`
**Content Line 2**: `"Nintendo event!"`
**Event ID**: Varies by region
**Icon**: `0xFFFF` (Question Mark)

### Sample Hex Dump (Wonder Card Header)

```
Offset  Hex Data                                         ASCII
------  -----------------------------------------------  ----------------
0x000:  21 A4 00 00                                      !...            CRC-16 + Padding
0x004:  XX XX XX XX YY YY ZZ ZZ                          ........        Event ID, Icon, Count
0x008:  TT SS BB BB BB BB BB BB BB BB BB BB BB BB BB BB  T.SSBBBBBBBBBBBB Type, Stamp, Title...
...
```

---

## Implementation Notes

### Memory Safety
- Always validate file sizes before reading
- Check for 0xFF terminator when parsing text
- Bounds-check all array accesses

### Endianness
- **Wonder Card fields**: Little-endian (LSB first)
- **Section checksum**: Big-endian (MSB first)
- **CRC-16**: Stored little-endian

### Error Handling
- Verify CRC-16 checksums before accepting data
- Check save file checksums before injection
- Always create backups before modifying saves

### Testing
- Use known-good event files (Aurora/Mystic Tickets)
- Verify in-game before distributing
- Test with all three games (Emerald, FireRed, LeafGreen)

---

## Wonder Card Flag Mapping (sReceivedGiftFlags)

The game uses `sReceivedGiftFlags` to map Wonder Card `flagId` values to the corresponding game flag that tracks receipt.

### FireRed/LeafGreen Flag Mapping

| flagId | Game Flag | Flag ID | Description |
|--------|-----------|---------|-------------|
| 0x0001 | FLAG_RECEIVED_AURORA_TICKET | 0x02A7 | Aurora Ticket (Birth Island → Deoxys) |
| 0x0002 | FLAG_RECEIVED_MYSTIC_TICKET | 0x02A8 | Mystic Ticket (Navel Rock → Ho-Oh/Lugia) |
| 0x0003 | FLAG_RECEIVED_OLD_SEA_MAP | 0x02A9 | Old Sea Map (Faraway Island → Mew) |

**Ship Enable Flags:**
- `FLAG_ENABLE_SHIP_BIRTH_ISLAND` (0x084B) - Enables ship route after Aurora Ticket
- `FLAG_ENABLE_SHIP_NAVEL_ROCK` (0x084A) - Enables ship route after Mystic Ticket

**Ticket Shown Flags:**
- `FLAG_SHOWN_AURORA_TICKET` (0x02F1) - Set when ticket is presented to NPC
- `FLAG_SHOWN_MYSTIC_TICKET` (0x02F0) - Set when ticket is presented to NPC

### Emerald Flag Mapping

| flagId | Game Flag | Flag ID | Description |
|--------|-----------|---------|-------------|
| 0x0001 | FLAG_RECEIVED_AURORA_TICKET | 0x013A | Aurora Ticket (Birth Island → Deoxys) |
| 0x0002 | FLAG_RECEIVED_MYSTIC_TICKET | 0x013B | Mystic Ticket (Navel Rock → Ho-Oh/Lugia) |
| 0x0003 | FLAG_RECEIVED_OLD_SEA_MAP | 0x013C | Old Sea Map (Faraway Island → Mew) |

**Unused Wonder Card Flags (0x013D-0x014D):**
Emerald reserves 17 additional flags for potential future Mystery Gift events that were never used.

**Ticket Shown Flags:**
- `FLAG_SHOWN_EON_TICKET` (0x01AE) - Eon Ticket (Southern Island → Latios/Latias)
- `FLAG_SHOWN_AURORA_TICKET` (0x01AF) - Set when Aurora Ticket is presented
- `FLAG_SHOWN_OLD_SEA_MAP` (0x01B0) - Set when Old Sea Map is presented
- `FLAG_SHOWN_MYSTIC_TICKET` (0x01DB) - Set when Mystic Ticket is presented

### How Flag Mapping Works

From `pokefirered/src/mystery_gift.c`:

```c
static const u16 sReceivedGiftFlags[] = {
    FLAG_RECEIVED_AURORA_TICKET,
    FLAG_RECEIVED_MYSTIC_TICKET,
    FLAG_RECEIVED_OLD_SEA_MAP
};
```

When processing a Wonder Card:
1. Game reads `flagId` from Wonder Card (offset 0x00-0x01)
2. Uses `flagId - 1` as index into `sReceivedGiftFlags[]`
3. Sets corresponding game flag to prevent re-receipt
4. Script can check this flag to conditionally give items

**Important**: `flagId` values are 1-indexed in Wonder Cards, but the array is 0-indexed, hence the `flagId - 1` lookup.

---

## Verified Structure Sizes

From pokefirered decompilation (`include/constants/global.h`):

```c
#define WONDER_CARD_TEXT_LENGTH 40
#define WONDER_CARD_BODY_TEXT_LINES 4
#define MAX_STAMP_CARD_STAMPS 7
```

**Calculated struct WonderCard size: 332 bytes**
- Header fields: 10 bytes (flagId:2 + iconSpecies:2 + idNumber:4 + type/bg/send:1 + maxStamps:1)
- Text fields: 320 bytes (8 × 40 bytes)
- Struct alignment: 2 bytes (ARM7 pads to 4-byte boundary: 330 → 332)

**File format: 336 bytes (0x150)**
- CRC-16: 2 bytes
- Start padding: 2 bytes
- Payload: 332 bytes (0x14C)

**Source**: [suloku/wc-tool](https://github.com/suloku/wc-tool/blob/master/wc3_tool/WC3/wc3.cs)
```c
SIZE_WC3 = 0x4 + 0x14C + ...  // 0x14C = 332 bytes payload
ICON_WC3 = 0x4 + 0x14C + 10   // WonderCard ends at offset 0x150
```

---

## References

- [Bulbapedia - Character Encoding (Generation III)](https://bulbapedia.bulbagarden.net/wiki/Character_encoding_in_Generation_III)
- [Project Pokémon - Gen 3 Mystery Event/Gift Research](https://projectpokemon.org/home/forums/topic/35903-gen-3-mystery-eventgift-research/)
- [Pokémon Save File Structure Documentation](https://bulbapedia.bulbagarden.net/wiki/Save_data_structure_in_Generation_III)
- [pokefirered decompilation](https://github.com/pret/pokefirered) - Source for struct definitions and flag mappings

---

**Document Version**: 1.1
**Last Updated**: 2025-11-28
**Author**: ComradeSean
**Project**: ComradeSean's Mystery Gift Injector

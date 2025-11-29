# Mystery Gift Tickets Folder

This folder contains all Mystery Gift ticket resources for Pokemon Generation III games.

## Folder Structure

```
Tickets/
  {NAME}_WonderCard.bin  - Wonder Card data (336 bytes)
  {NAME}_Script.bin      - Script data (1004 bytes)
```

## File Naming Convention

Files are automatically discovered based on naming pattern:

```
{NAME}_WonderCard.bin
{NAME}_Script.bin
```

### Name Components

The `{NAME}` portion should contain:

| Component | Description | Examples |
|-----------|-------------|----------|
| Event | Event type | AURORA_TICKET, MYSTIC_TICKET, OLD_SEA_MAP, EON_TICKET |
| Game | Game code | FRLG, EMERALD, E, RS |
| Language | Region/language (optional) | ENGUSA, ENGUK, ESP, FRE, GER, ITA, JAP |

### Examples

```
AURORA_TICKET_FRLG_ENGUSA_WonderCard.bin + AURORA_TICKET_FRLG_ENGUSA_Script.bin
MYSTIC_TICKET_EMERALD_ENGUK_WonderCard.bin + MYSTIC_TICKET_EMERALD_ENGUK_Script.bin
AURORA_TICKET_FRLG_ESP_WonderCard.bin + AURORA_TICKET_FRLG_ESP_Script.bin
```

## File Size Requirements

**CRITICAL**: Binary files must be EXACTLY the specified size:

| File Type | Size | Hex | Structure |
|-----------|------|-----|-----------|
| WonderCard | 336 bytes | 0x150 | 4-byte CRC header + 332-byte payload |
| Script | 1004 bytes | 0x3EC | 4-byte CRC header + 1000-byte payload |

Files with incorrect sizes will be skipped with a warning.

## Dynamic Discovery

The application automatically discovers tickets by:

1. Scanning for `*_WonderCard.bin` files
2. Matching each WonderCard to its Script by the shared prefix
3. Parsing game type from the filename (FRLG, EMERALD, E, RS)
4. Extracting language code for display name (ENGUSA, ENGUK, ESP, etc.)
5. Validating file sizes (336 bytes for WonderCard, 1004 bytes for Script)

**No manifest file is required** - just drop properly named files in the folder!

## Adding Custom Tickets

### Step 1: Obtain Ticket Files

You need two binary files with CRC headers:
1. **Wonder Card** - 336 bytes (4-byte header + 332-byte payload)
2. **Script** - 1004 bytes (4-byte header + 1000-byte payload)

### Step 2: Name Your Files

Follow the naming convention:
- `{EVENT}_{GAME}_{LANG}_WonderCard.bin`
- `{EVENT}_{GAME}_{LANG}_Script.bin`

Example:
- `CUSTOM_TICKET_FRLG_ENGUSA_WonderCard.bin`
- `CUSTOM_TICKET_FRLG_ENGUSA_Script.bin`

### Step 3: Copy Files

Place both files in the Tickets folder.

### Step 4: Restart Application

Restart the Mystery Gift Injector - your custom ticket will appear in the dropdown!

## CRC Header Format

Both WonderCard and Script files use the same 4-byte header:

```
Offset 0x00-0x01: CRC-16 checksum (little-endian)
Offset 0x02-0x03: Padding (0x00 0x00)
Offset 0x04+:     Payload data
```

For placeholder/ripped data, use zero-summed CRC (all zeros).

## Troubleshooting

### "No ticket files found"
- Ensure files match pattern: `*_WonderCard.bin`
- Check filename spelling and underscores

### "Missing script file"
- Ensure the Script file has the same prefix as the WonderCard
- Example: `AURORA_TICKET_FRLG_ENGUSA_WonderCard.bin` requires `AURORA_TICKET_FRLG_ENGUSA_Script.bin`

### "Invalid WonderCard size"
- Your WonderCard file must be exactly 336 bytes (with CRC header)
- Older 332-byte files need 4-byte header prepended

### "Invalid Script size"
- Your Script file must be exactly 1004 bytes (with CRC header)
- Older 1000-byte files need 4-byte header prepended

### "Could not determine game type"
- Ensure filename contains a recognized game code: FRLG, EMERALD, E, or RS
- The game code should be separated by underscores

### Ticket not appearing
- Check that both WonderCard AND Script files exist with matching prefixes
- Verify game code is valid (FRLG, EMERALD, E, RS)
- Check console output for warnings about skipped files

## Included Tickets

### FireRed/LeafGreen (FRLG)
| Ticket | Languages |
|--------|-----------|
| Aurora Ticket | USA, UK, Spanish, French, German, Italian |
| Mystic Ticket | USA |

### Emerald
| Ticket | Languages |
|--------|-----------|
| Aurora Ticket | UK, Spanish, French, German, Italian |
| Mystic Ticket | USA, TCGWC 2005 |

## Notes

- Ruby and Sapphire do not support Mystery Gifts/Wonder Cards
- The CRC table (tab.bin) is embedded in the application
- Always backup your save files before injecting custom tickets!

---

**For more information about Pokemon Gen III Mystery Gifts, see:**
- [Bulbapedia - Mystery Gift](https://bulbapedia.bulbagarden.net/wiki/Mystery_Gift)
- [Project Pokemon - Event Database](https://projectpokemon.org/home/events/)

# Mystery Gift Injector

A tool for injecting Mystery Gift Wonder Cards into Pokemon Generation III save files (FireRed, LeafGreen, Emerald).

## Features

- **Wonder Card Injection**: Inject authentic Mystery Gift event cards into save files
- **Visual Wonder Card Editor**: View and edit Wonder Card text with ROM-accurate graphics
- **Script Disassembler**: View GREEN MAN scripts with disassembly
- **ROM Graphics Extraction**: Extract fonts, sprites, and backgrounds from GBA ROMs
- **Save File Validation**: Automatic checksum recalculation and validation

## Supported Games

| Game | Versions |
|------|----------|
| Pokemon FireRed | USA 1.0, 1.1 |
| Pokemon LeafGreen | USA 1.0, 1.1 |
| Pokemon Emerald | USA, Europe |

## Planned Features

- **Japanese ROM Support**: Full support for Japanese versions of FireRed, LeafGreen, and Emerald
- **Ruby/Sapphire Support**: Extend compatibility to Pokemon Ruby and Sapphire
- **Wonder News**: Potential support for Wonder News injection

## Building

### Requirements

- Qt 6.x
- CMake 3.16+
- C++17 compatible compiler

### Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Dependencies

The application uses:
- Qt Widgets for the GUI
- Qt Core for data processing
- Built-in YAML parser for ROM offset data

## Usage

1. **Load a ROM**: The application auto-detects Pokemon GBA ROMs in the working directory. For authentic Wonder Card graphics, a supported ROM is required.

2. **Open a Save File**: Use File → Open to load a .sav file (128KB Pokemon Gen3 save).

3. **Select a Preset**: Choose from available Mystery Gift presets in the dropdown.

4. **Edit (Optional)**: Click "Edit" to modify Wonder Card text fields.

5. **Save**: Use File → Save to inject the Wonder Card into the save file.

## Project Structure

```
Mystery_Gift_Injector/
├── *.cpp, *.h          # C++ source files (Qt application)
├── CMakeLists.txt      # Build configuration
├── Resources/          # Configuration files
│   ├── gen3_rom_data.yaml      # ROM offsets database
│   ├── script_commands.yaml    # Script command definitions
│   └── *.json                  # Font/character mappings
├── Tickets/            # Pre-made Wonder Card presets
│   └── *_WonderCard.bin, *_Script.bin  # Dynamically discovered ticket pairs
└── WONDERCARD_STRUCTURE.md  # Technical documentation
```

## Technical Details

### Wonder Card Format

Wonder Cards are 332-byte data blocks containing:
- Event metadata (ID, icon, type)
- Text fields (title, subtitle, content, warnings)
- Uses Gen3 proprietary text encoding

See [WONDERCARD_STRUCTURE.md](WONDERCARD_STRUCTURE.md) for full documentation.

### Save File Handling

- Reads/writes 128KB Gen3 save files
- Handles dual save slots (A/B)
- Recalculates section checksums automatically
- Supports enabling Mystery Gift flag

### Graphics Extraction

The application extracts from ROM:
- 4bpp tiles (sprites, backgrounds)
- 2bpp fonts with glyph width tables
- BGR555 palettes
- LZ77 compressed data

## Credits

- **Author**: ComradeSean
- **Research**: Pokemon Gen3 ROM structure and Mystery Gift format

## License

This project is for educational and research purposes. Pokemon is a trademark of Nintendo/Game Freak/The Pokemon Company.

---

*For questions or issues, please open a GitHub issue.*

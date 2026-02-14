# Cinder *(repo: dirtverb)*

A gritty shimmer reverb VST3 plugin by **Substrate Audio**. Built with JUCE 8 and C++20. Combines lush infinite reverb with lo-fi degradation and wavefolder distortion.

**Visual identity:** Substrate Audio standard — `#0A0A0A` background, DM Sans + Space Mono typography, ember `#C4502A` accent, flat arc-based knobs, resizable UI, tooltips, output metering.

## Features

- **Shimmer Reverb**: 8-channel Feedback Delay Network with pitch-shifted feedback for ethereal, infinite tails
- **Lo-fi Degradation**: Sample rate reduction (44.1kHz → 4kHz) and bit crushing (16-bit → 4-bit)
- **Wavefolder Distortion**: Triangle wave folding for rich harmonic content
- **Parallel Architecture**: Blend between clean reverb and wavefolded reverb
- **Resizable UI**: Aspect-ratio locked, 80%–140% scaling
- **Output Metering**: RMS/peak meter with peak hold
- **Tooltips**: Parameter value popups on hover/drag

## Parameters

| Parameter | Description |
|-----------|-------------|
| **DECAY** | Reverb tail length (0.1s to ∞) |
| **SHIMMER** | Octave-up pitch shift in feedback |
| **SIZE** | Room size / diffusion density |
| **DEGRADE** | Lo-fi destruction amount |
| **FOLD** | Wavefolder intensity |
| **DIRT** | Clean vs wavefolded reverb blend |
| **MIX** | Dry/wet mix |

## Quick Start (Windows)

### Prerequisites

1. **Visual Studio 2026** (or 2022+) with "Desktop development with C++" workload
2. **CMake 3.22+**
3. **JUCE Framework** cloned alongside this repo

### Build Instructions

```powershell
cd c:\dev\JUCE\dirtverb

# Configure
cmake -B build -G "Visual Studio 18 2026" -A x64

# Build Release
cmake --build build --config Release

# Plugin output:
# build\Cinder_artefacts\Release\VST3\Cinder.vst3
```

Or simply run `build.bat`.

### Install Plugin

Run `install.bat` as administrator, or manually copy:
```powershell
Copy-Item -Recurse "build\Cinder_artefacts\Release\VST3\Cinder.vst3" "C:\Program Files\Common Files\VST3\"
```

Then rescan plugins in your DAW.

## Project Structure

```
dirtverb/
├── CMakeLists.txt              # Build configuration
├── Resources/
│   └── Fonts/                  # Embedded DM Sans + Space Mono
├── Source/
│   ├── PluginProcessor.h/cpp   # Audio processing core (CinderProcessor)
│   ├── PluginEditor.h/cpp      # UI implementation (CinderEditor)
│   ├── DSP/
│   │   ├── ShimmerReverb.h     # FDN reverb with pitch shift
│   │   ├── LofiDegrader.h      # Sample rate + bit reduction
│   │   └── Wavefolder.h        # Triangle wave folding
│   └── UI/
│       ├── CinderLookAndFeel.h # Substrate Audio visual theme
│       ├── OutputMeter.h       # RMS/peak output meter
│       └── WaveformVisualizer.h # Level visualization with glitch effects
├── build.bat                   # Windows build script
├── install.bat                 # VST3 installer
└── README.md
```

## Signal Flow

```
Input → Shimmer Reverb → Lo-fi Degradation → ┬→ Clean Path    ─┬→ DIRT blend → MIX → Output
                                              └→ Wavefolder Path─┘
```

## Tested DAWs

- Ableton Live 11/12

## License

MIT License - feel free to use and modify!

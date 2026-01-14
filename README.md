# Dirtverb 🔊💀

A gritty shimmer reverb VST3 plugin built with JUCE. Combines lush infinite reverb with lo-fi degradation and wavefolder distortion.

## ✨ Features

- **Shimmer Reverb**: 8-channel Feedback Delay Network with pitch-shifted feedback for ethereal, infinite tails
- **Lo-fi Degradation**: Sample rate reduction (44.1kHz → 4kHz) and bit crushing (16-bit → 4-bit)
- **Wavefolder Distortion**: Triangle wave folding for rich harmonic content
- **Parallel Architecture**: Blend between clean reverb and wavefolded reverb
- **Experimental UI**: "Corrupted Crystal" theme with glitch effects

## 🎛️ Parameters

| Parameter | Description |
|-----------|-------------|
| **DECAY** | Reverb tail length (0.1s to ∞) |
| **SHIMMER** | Octave-up pitch shift in feedback |
| **SIZE** | Room size / diffusion density |
| **DEGRADE** | Lo-fi destruction amount |
| **FOLD** | Wavefolder intensity |
| **DIRT** | Clean vs wavefolded reverb blend |
| **MIX** | Dry/wet mix |

## 🚀 Quick Start (Windows)

### Prerequisites

1. **Visual Studio 2022** with "Desktop development with C++" workload
   - Download: https://visualstudio.microsoft.com/downloads/
   - During install, select "Desktop development with C++"

2. **CMake 3.22+**
   - Download: https://cmake.org/download/
   - Or install via winget: `winget install Kitware.CMake`

### Build Instructions

```powershell
# Navigate to project
cd c:\dev\JUCE\dirtverb

# Configure with CMake
cmake -B build -G "Visual Studio 18 2026" -A x64

# Build Release
cmake --build build --config Release

# Plugin will be at:
# build\dirtverb_artefacts\Release\VST3\dirtverb.vst3
```

### Install Plugin

Copy the built `.vst3` folder to your system VST3 directory:
```powershell
Copy-Item -Recurse "build\dirtverb_artefacts\Release\VST3\dirtverb.vst3" "C:\Program Files\Common Files\VST3\"
```

Then rescan plugins in your DAW.

## 📁 Project Structure

```
dirtverb/
├── CMakeLists.txt              # Build configuration
├── Source/
│   ├── PluginProcessor.h/cpp   # Audio processing core
│   ├── PluginEditor.h/cpp      # UI implementation
│   ├── DSP/
│   │   ├── ShimmerReverb.h     # FDN reverb with pitch shift
│   │   ├── LofiDegrader.h      # Sample rate + bit reduction
│   │   └── Wavefolder.h        # Triangle wave folding
│   └── UI/
│       ├── DirtLookAndFeel.h   # Custom visual theme
│       └── WaveformVisualizer.h # Level visualization
└── README.md
```

## 🎨 Signal Flow

```
Input → Shimmer Reverb → Lo-fi Degradation → ┬→ Clean Path    ─┬→ DIRT blend → MIX → Output
                                              └→ Wavefolder Path─┘
```

## 🎹 Tested DAWs

- Ableton Live 11/12

## 📄 License

MIT License - feel free to use and modify!

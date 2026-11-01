# GRAVEYARD SHIFT BY RIIFTY

A darkwave effect plugin with 111 built-in spooky presets.
Put it on drums, bass, synths, vocals, guitars, or your master.

## The 9 knobs
| Knob | What it does |
|---|---|
| DRIVE | Adds grit and heat |
| DARK | Closes the sound into the shadows (muffles the highs) |
| CRUSH | Lo-fi, broken, crunchy |
| WOBBLE | Slow, creepy chorus movement |
| ECHO | Dark echoes that fade into the fog |
| ECHO TIME | How far apart the echoes are |
| SPACE | Big haunted-hall reverb |
| WIDTH | Narrow = centered, wide = surrounding you |
| MIX | Dry (original) vs wet (spooky) |

## How to get the plugin file (no coding needed)
GitHub is a free website with free computers that can "bake" the plugin for you.

1. Make a free account at github.com
2. Click the **+** (top right) > **New repository**. Name it `graveyard-shift`. Click **Create repository**.
3. Click **uploading an existing file**. Drag in EVERYTHING from this folder
   (the `Source` folder, `tools` folder, `.github` folder, `CMakeLists.txt`, `README.md`).
   Click **Commit changes**.
   - If the hidden `.github` folder didn't upload: click **Add file > Create new file**, type
     `.github/workflows/build.yml` as the name, paste in the contents of that file, and commit.
4. Click the **Actions** tab. Wait ~10-15 minutes for the green check mark.
   (The first time, click the big button to enable workflows if it asks.)
5. Click the finished run, scroll to **Artifacts**, and download
   `GRAVEYARD-SHIFT-BY-RIIFTY-Windows` (or `-Mac`).
6. Unzip it. Open `VST3`. Copy the folder `GRAVEYARD SHIFT BY RIIFTY.vst3` into:
   - Windows: `C:\Program Files\Common Files\VST3`
   - Mac: `/Library/Audio/Plug-Ins/VST3`
7. Open your DAW and rescan plugins. Look for **GRAVEYARD SHIFT BY RIIFTY**.

## Changing the presets
Edit `tools/make_presets.py` (names and knob values), run it, and rebuild.

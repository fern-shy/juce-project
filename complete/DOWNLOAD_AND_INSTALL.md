# Pandoras Box: download, install, and use

Pandoras Box is a chaos effect by FernShy. Click one of its three eyes and
it rolls new hidden settings for a chain of delays, reverb, pitch shifting,
ring modulation, comb filters, and wavefolding distortion. Every click gives a
new sound, from shimmering harmony to metallic, horrific noise.

Version 1.0.2

## What you need

| | Windows | Mac |
|---|---|---|
| System | Windows 10 or 11, 64-bit | macOS 11 (Big Sur) or newer, Apple Silicon or Intel |
| DAW | Any 64-bit VST3 host: FL Studio, Ableton Live, Reaper, Cubase, Studio One... | Any AU or VST3 host: Logic Pro, Ableton Live, FL Studio, Reaper... |
| Download | Installer (`...Setup.exe`) | Installer (`.pkg`) |

## Windows

### 1. Download and install

1. Close your DAW.
2. Click **Download for Windows** and open the downloaded file (it ends in
   `Setup.exe`).
3. Windows may show a blue box saying **"Windows protected your PC."** Click
   **More info**, then **Run anyway**. This appears because the installer is
   not yet signed with a paid Microsoft certificate. It does not mean anything
   is wrong with the file.
4. Click **Yes** when Windows asks whether the app may make changes. The
   installer needs this to write to the shared plugin folder.
5. Click **Install**, then **Finish**.

Pandoras Box is now in `C:\Program Files\Common Files\VST3`, where every
Windows DAW looks for VST3 plugins. If you installed an earlier test version,
the installer replaces it.

### 2. Load it in FL Studio

1. Open FL Studio and choose **Options > Manage plugins**.
2. Click **Find installed plugins** (tick **Verify plugins** first) and wait
   for the scan to finish.
3. Find **Pandoras Box** (by FernShy) in the list and click its **star** so it
   appears in your effect menus.
4. Open the Mixer (**F9**), select the mixer track you want to mangle, click an
   empty effect slot, and choose **Pandoras Box**.

If you tried an earlier version that didn't work, FL Studio may still remember
it as broken. Scan again with the options to verify and rescan plugins ticked.

### Using Ableton Live instead

1. Open **Options > Preferences > Plug-Ins**.
2. Turn on **Use VST3 Plug-in System Folders**, then click **Rescan**.
3. In the browser, open **Plug-Ins > FernShy** and drag **Pandoras Box** onto
   an audio track.

## Mac

### 1. Download and install

1. Click **Download for Mac** and open the downloaded `.pkg` file.
2. macOS may say it can't verify the installer and refuse to open it. Click
   **Done**. This appears because the installer is not yet notarized by Apple.
3. Open **System Settings > Privacy & Security** and scroll down to
   **Security**. Next to the message about the Pandoras Box installer, click
   **Open Anyway**, then confirm with your password or Touch ID.
   (On macOS 14 or older you can instead Control-click the file, choose
   **Open**, then **Open** again.)
4. In the installer, click **Continue**, then **Install**, enter your
   password, and click **Close**.

This installs both the Audio Unit (for Logic Pro and GarageBand) and the VST3
into `/Library/Audio/Plug-Ins`.

### 2. Load it in your DAW

- **Logic Pro:** on an audio track, click an Audio FX slot and choose
  **Audio Units > FernShy > Pandoras Box**.
- **Ableton Live:** open **Settings > Plug-Ins**, turn on Audio Units and/or
  **Use VST3 Plug-in System Folders**, click **Rescan**, then drag
  **Plug-Ins > FernShy > Pandoras Box** onto a track.
- **FL Studio:** follow the FL Studio steps in the Windows section above.

## Using Pandoras Box

**Turn your speakers down before the first click.** Pandoras Box has a safety
limiter so it never goes past full scale, but some rolls are loud, harsh, and
piercing by design.

### The three eyes

The plugin starts with gentle default settings. Click an eye to roll:

- **Left eye:** new hidden settings for every effect (delay times and
  feedback, reverb size, pitch amounts, filter, distortion character, and
  more). The effect order stays the same.
- **Right eye:** shuffles the order the effects run in. The settings stay the
  same, but the same effects in a different order can sound completely
  different.
- **Middle (big) eye:** both at once, a completely new sound.

The eyes pulse with the plugin's output. There is no undo, so when you find a
sound you love, save it (see below). **Bypass** (bottom right) compares against
your dry signal.

### The eight intensity controls

Pandoras Box also has eight controls that aren't on its face. They appear in
your DAW's parameter list, where you can adjust and automate them. Each goes
from 0% (that group is off) to 100%, and all start at 50%.

| Control | What it scales |
|---|---|
| Time | Three delays with rolled times and feedback |
| Breath | A long reverb. Only about 1 in 5 rolls includes it |
| Order | A compressor that glues the chaos together |
| Chaos | Pitch shifting, ring modulation, chorus, and a resonant filter |
| Space | Stereo widening and detuning |
| Reflection | Octave layers above and below your sound |
| Fracture | Metallic comb-filter resonances |
| Wrath | Wavefolding distortion with a nasal, "broken speaker" colour |

About 1 in 10 rolls also adds an extra distortion and bitcrusher at the end of
the chain. No control scales it; re-roll to get rid of it.

To reach the controls:

- **FL Studio:** click the small menu at the top left of the plugin window
  and choose **Browse parameters**. To automate one, right-click it and choose
  **Create automation clip**.
- **Ableton Live:** click the triangle on the device title bar to unfold its
  parameters.
- **Logic Pro:** switch the plugin window's view from **Editor** to
  **Controls**.

### Saving and exporting

- Your project remembers the exact roll. Close and reopen it and you get the
  same sound.
- To keep a sound for other projects, save a preset with your DAW's normal
  preset save button.
- Exports and bounces sound the same as playback. Delay and reverb tails
  build up over time, so start the export a bar or two before the part you
  want.

## Troubleshooting

**Windows says "Smart App Control blocked" and has no Run anyway button.**
Smart App Control on some Windows 11 PCs blocks all unsigned software,
including this plugin. A signed version is planned. Until then, Pandoras Box
can't run on that PC. Turning Smart App Control off isn't recommended, because
it can't be turned back on without reinstalling Windows.

**Pandoras Box doesn't show up in FL Studio.** In **Options > Manage plugins**,
make sure the plugin search paths include `C:\Program Files\Common Files\VST3`,
then scan again with **Verify plugins** ticked. Pandoras Box is an effect, so
look for it among effects, not instruments.

**It shows an old version, the name "Tremolo", or doesn't load.** An earlier
test copy is still around. Run the installer again (it removes earlier copies
from the VST3 folder), delete any other `Pandoras Box` or
`PandorasBox-...` folders you copied by hand, and rescan.

**It doesn't show up in Ableton or Logic on a Mac.** Make sure the installer
finished, then rescan: in Ableton, **Settings > Plug-Ins > Rescan**; in Logic,
**Logic Pro > Settings > Plug-in Manager**, select Pandoras Box, and click
**Reset & Rescan Selection**.

## Manual install (without the installer)

On Windows you can use the ZIP version instead: extract it, copy only the
**`Pandoras Box.vst3`** folder (not the folder around it) into
`C:\Program Files\Common Files\VST3`, then restart your DAW and rescan.

The Mac ZIP contains the same `.pkg` installer, so follow the Mac steps above.

## Uninstall

- **Windows:** **Settings > Apps > Installed apps**, find **Pandoras Box
  (VST3)**, and choose **Uninstall**.
- **Mac:** in Finder choose **Go > Go to Folder**, open
  `/Library/Audio/Plug-Ins/Components` and `/Library/Audio/Plug-Ins/VST3`, and
  move **Pandoras Box** from each to the Trash.

---

## For FernShy: publishing this page

Upload these files from `complete/dist/` and link the download buttons to them:

| Button | File |
|---|---|
| Download for Windows | `PandorasBox-1.0.2-Windows-x64-unsigned-Setup.exe` |
| Download for Mac | `PandorasBox-1.0.2-macOS-universal-unsigned.pkg` |
| Windows ZIP (optional) | `PandorasBox-1.0.2-Windows-x64-unsigned.zip` |

You can rename the files before uploading (for example to
`PandorasBox-1.0.2-Windows-Setup.exe`). The guide only assumes the Windows file
ends in `Setup.exe` and the Mac file ends in `.pkg`.

The "Windows protected your PC", Smart App Control, and macOS "Open Anyway"
steps go away once the builds are signed. That needs a Windows code-signing
certificate (for example Azure Trusted Signing) and an Apple Developer Program
membership. With those set up, the same release scripts produce signed
installers; see `RELEASING.md`. After signing, delete the warning steps and
the Smart App Control troubleshooting entry from this page.

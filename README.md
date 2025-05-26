# Stereo Tool LADSPA Plugin

This project provides a LADSPA wrapper for [Thimeo Stereo Tool](https://www.thimeo.com/stereo-tool/), enabling integration of its broadcast-grade audio processing into LADSPA-compatible hosts.

---

## 🎛 Features

* Wraps `libStereoTool_intel64.so` into a LADSPA plugin
* Accepts stereo input/output
* Loads a `settings.sts` preset at runtime from the same directory
* Written in C with a C++ wrapper for compatibility

> ⚠️ Note: Due to the limited parameter passing capabilities of the LADSPA interface, the path to the configuration file (`settings.sts`) is currently hardcoded to the same directory as the plugin `.so` file.

---

## 🔧 Installation

1. Download "x86/64 Linux libStereoTool plugin" from [Thimeo](https://www.thimeo.com/stereo-tool/download/). Put the included `libStereoTool_intel64.so` (this is for 64 bit hosts, 32 bit and ARM are also available) into the library search path and run `ldconfig`
2. Clone this repo and run:

```bash
make
make install
```

3. Ensure your LADSPA host (like `ecasound`, `liquidsoap`, or `Audacity`) can discover the `.so` file:

```bash
export LADSPA_PATH=~/.ladspa
```

---

## 🔄 Configuration

Place a `settings.sts` file (exported from Stereo Tool GUI) in the same directory as `stereotool_ladspa.so`. This will be auto-loaded at startup.

You can download or generate your own `.sts` config using [Stereo Tool GUI](https://www.thimeo.com/stereo-tool/).

---

## 📄 License

This LADSPA wrapper is released under the [GNU General Public License v3.0 (GPLv3)](https://www.gnu.org/licenses/gpl-3.0.html).

⚠️ **Important**: This wrapper depends on `libStereoTool_intel64.so` which is licensed separately by [Thimeo](https://www.thimeo.com/licensing/). Use of Stereo Tool's DSP code may require a valid license.

---

## 🙏 Credits

* [Thimeo](https://www.thimeo.com) for Stereo Tool DSP
* LADSPA SDK

---

## 📣 Feedback & Contributions

Pull requests and issues welcome.

To contribute, fork this repo and submit a PR with:

* Clear description of change
* Build/test instructions
* Consistent code style

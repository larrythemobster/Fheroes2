# fheroes2: Stat Editor & Modding Fork

This is a customized fork of the [fheroes2](https://github.com/ihhub/fheroes2) engine, originally developed as a private client project and now released open-source. 

While it retains the incredible engine upgrades of the base fheroes2 project (modernizing the classic Heroes of Might and Magic II experience), this repository introduces a **Visual Modding System** and **Data Externalization** that allows anyone to completely rebalance the game and campaigns without compiling code.

## 🛠 Custom Features

* **Externalized Game Stats (`stats.json`):** Core game mechanics—including Artifact bonuses, Spell mana costs/damage, and Hero starting armies—have been extracted from the hardcoded C++ source and moved into an easily readable JSON file. 
* **Externalized Campaigns (`campaigns.json`):** Entire campaign structures are no longer hardcoded! Scenario links, carry-over awards, starting resources, and starting bonuses are now fully customizable via `campaigns.json`.
* **Visual Mod Editor (`editor.html`):** Included in the root of this project is a lightweight, browser-based GUI editor. Simply open `editor.html` in any web browser to visually edit the game's balance, adjust troop counts, or rewrite artifact bonuses. When you're done, click save, and drop the new `stats.json` into your game folder.
* **Instant Rebalancing:** Want *The Ultimate Book of Knowledge* to grant +12 Knowledge? Done. Want *Mass Cure* to cost 15 mana? Done. The game reads your custom stats on launch.

## 🗺 Native Map Converter

This fork includes a powerful built-in map converter that translates legacy map and campaign formats (`.mp2`, `.mpx`, `.h2c`, `.mx2`, and `.hxc`) into the engine's modern, robust `.fh2m` format. 

Because it uses the engine's native loading logic, it preserves object metadata, rumors, daily events, and campaign overlays accurately.

**How to use the converter:**
1. Open your command line or terminal.
2. Run the executable with the `--convert` flag, pointing to the directory containing your legacy maps:
   ```bash
   fheroes2 --convert /path/to/your/maps/directory
   ```
3. The engine will batch process the files, load their states, and export them as modern `.fh2m` files into the same directory.

## 📥 Downloads & Releases

1. Download the latest `.zip` release from our [Releases page](../../releases). 
2. Extract the files and ensure your `stats.json` and `campaigns.json` are in the same directory as the executable.
3. Use the included `editor.html` to create your own custom balance mod.
4. Run the game!

*(Note: To play the game using this engine, you will still need the original game's data files. Place your `DATA` and `MAPS` folders from your original installation into the fheroes2 directory.)*

---
*The section below contains the original fheroes2 documentation.*


# fheroes2

**fheroes2** is a recreation of the Heroes of Might and Magic II game engine.

This open source multiplatform project, written from scratch, is designed to reproduce the original game with significant
improvements in gameplay, graphics and logic (including support for high-resolution graphics, improved AI, numerous fixes
and user interface improvements), breathing new life into one of the most addictive turn-based strategy games.

You can find a complete list of all of our changes and enhancements in [**its own wiki page**](https://github.com/ihhub/fheroes2/wiki/Features-and-enhancements-of-the-project).

<p align="center">
    <img src="docs/images/screenshots/screenshot_world_map.webp" width="820" alt="Screenshot of the world map">
</p>

<p align="center">
    <img src="docs/images/screenshots/screenshot_battle.webp" width="410" alt="Screenshot of the battle screen">
    <img src="docs/images/screenshots/screenshot_castle.webp" width="410" alt="Screenshot of the castle screen">
</p>

## Download and Install

Please follow the [**installation guide**](docs/INSTALL.md) to download and install fheroes2.

[![Github Downloads](https://img.shields.io/github/downloads/ihhub/fheroes2/total.svg)](https://github.com/ihhub/fheroes2/releases)

## Copyright

All rights for the original game and its resources belong to former The 3DO Company. These rights were transferred to Ubisoft.
We do not encourage and do not support any form of illegal usage of the original game. We strongly advise to purchase the original
game on [**GOG**](https://www.gog.com) or [**Ubisoft Store**](https://store.ubi.com) platforms. Alternatively, you can download a
free demo version of the game. Please refer to the [**installation guide**](docs/INSTALL.md) for more information.

## License

This project is licensed under the [**GNU General Public License v2.0**](https://github.com/ihhub/fheroes2/blob/master/LICENSE).

Initially, the project was developed on [**sourceforge**](https://sourceforge.net/projects/fheroes2/).

## Contribution and Development

This repository is a place for everyone. If you want to contribute, please read more to learn [**how you can contribute**](https://github.com/ihhub/fheroes2/wiki/F.A.Q.#q-how-can-i-contribute-to-the-project).

### Developing fheroes2 engine

To build the project from source, please follow [**this guide**](docs/DEVELOPMENT.md).

[![Build Status](https://github.com/ihhub/fheroes2/actions/workflows/push.yml/badge.svg)](https://github.com/ihhub/fheroes2/actions)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=bugs)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=code_smells)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2)
[![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2)

To assist with the graphical asset efforts of the project, please look at our [**graphical artist guide**](docs/GRAPHICAL_ASSETS.md).

If you would like to help translating the project, please read the [**translation guide**](docs/TRANSLATION.md).

### Developing fheroes2 documentation site

To build the [website](https://ihhub.github.io/fheroes2/) from source, please follow
[**this guide**](docs/WEBSITE_LOCAL_DEV.md).

## Donation

We accept donations via [**Patreon**](https://www.patreon.com/fheroes2), [**PayPal**](https://www.paypal.com/paypalme/fheroes2) or [**Boosty**](https://boosty.to/fheroes2).
All donations will be used only for the future project development as we do not consider this project as a source of income by any means.

[![Patreon Donate](https://img.shields.io/badge/Donate-Patreon-green.svg)](https://www.patreon.com/fheroes2)
[![Paypal Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/fheroes2)
[![Boosty Donate](https://img.shields.io/badge/Donate-Boosty-green.svg)](https://boosty.to/fheroes2)

## Contacts

Follow us on social networks: [**Facebook**](https://www.facebook.com/groups/fheroes2) or [**VK**](https://vk.com/fheroes2).
We also have a [**Discord**](https://discord.gg/xF85vbZ) server to discuss the development of the project.

[![Facebook](https://img.shields.io/badge/Facebook-blue.svg)](https://www.facebook.com/groups/fheroes2)
[![VK](https://img.shields.io/badge/VK-blue.svg)](https://vk.com/fheroes2)
[![Discord](https://img.shields.io/discord/733093692860137523.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/xF85vbZ)

## Frequently Asked Questions (FAQ)

You can find answers to the most commonly asked questions on our [**F.A.Q. page**](https://github.com/ihhub/fheroes2/wiki/F.A.Q.).
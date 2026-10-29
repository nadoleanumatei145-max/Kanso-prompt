# Kanso

[![license](https://img.shields.io/badge/license-GPL--3.0-e0a458)](#license)
[![platform](https://img.shields.io/badge/platform-linux-e0a458)](#requirements)

**簡素** — simplicity.

A prompt renderer for zsh. Reads a JSON config and an optional Lua script, prints a
single line of zsh-native prompt escapes to stdout. Four modules built in, as many
custom ones as you care to script.

**[Website](https://nadoleanumatei145-max.github.io/Kanso-prompt/)** ·
**[Dependencies](https://nadoleanumatei145-max.github.io/Kanso-prompt/dependencies.html)** ·
**[Issues](https://github.com/nadoleanumatei145-max/Kanso-prompt/issues)**

## Preview

```
 ~/dev/kanso   main  󰔛 0.02s
󰘍 ls
CMakeLists.txt  build  include  src
```

Multi-line by default. Set `"newline": false` to keep the arrow on the same line as
the modules.

## Features

- **Four built-in modules** — working directory, git branch, execution time,
  exit-code arrow. Each with an enable flag, an icon, and a 256-color code.
- **Custom modules in Lua** — `gui.add_module{ icon, color, value }`. `value` can be
  a string, or a function evaluated on every prompt.
- **Optional config** — every key in `kanso.json` falls back to a default. A missing
  file or a half-written one still renders a complete prompt.
- **Native output** — prints zsh's own `%F{…}` escapes. No theme loader, no
  shell-side substitution tricks, no subprocess per segment.
- **Small** — one binary, two vendored header-only libraries, links against Lua.

## Requirements

| | |
|---|---|
| C++17 compiler | g++ 9+ / clang++ 10+ |
| Lua 5.3+ | dev headers and shared library (5.4 recommended) |
| sol2 | header-only, vendored at build time |
| nlohmann/json | header-only, vendored at build time |
| zsh | any version with `prompt_subst` |

Per-distribution package commands:
[dependencies page](https://nadoleanumatei145-max.github.io/Kanso-prompt/dependencies.html).

## Install

### 1. System packages

```sh
# Arch
sudo pacman -S --needed base-devel git lua nlohmann-json

# Debian / Ubuntu
sudo apt install -y build-essential git liblua5.4-dev nlohmann-json3-dev

# Fedora
sudo dnf install -y gcc-c++ git lua-devel nlohmann-json-devel
```

### 2. Build

```sh
git clone https://github.com/nadoleanumatei145-max/Kanso-prompt.git
cd Kanso-prompt

mkdir -p vendor
git clone --depth 1 https://github.com/ThePhD/sol2.git vendor/sol2
git clone --depth 1 https://github.com/nlohmann/json.git vendor/json

g++ src/main.cpp -o prompt_renderer -std=c++17 -O2 \
  -I vendor/sol2/include \
  -I vendor/json/single_include \
  -llua

sudo mv prompt_renderer /usr/local/bin/
```

Skip the vendor step if sol2 and nlohmann-json are installed system-wide, and drop
the two `-I` flags. On Debian-based systems the library may be `-llua5.4` instead of
`-llua`.

### 3. Wire into zsh

```zsh
# ~/.zshrc
setopt prompt_subst
zmodload zsh/datetime

preexec() { CMD_START=$EPOCHREALTIME }

set_custom_prompt() {
  local last_status=$?
  local duration=0
  if [ -n "$CMD_START" ]; then
    duration=$(printf "%.3f" $(( EPOCHREALTIME - CMD_START )))
    unset CMD_START
  fi
  PROMPT_TEXT=$(/usr/local/bin/prompt_renderer $last_status $duration)
}

autoload -Uz add-zsh-hook
add-zsh-hook precmd set_custom_prompt

PROMPT='$PROMPT_TEXT'
```

Restart the shell, or `source ~/.zshrc`.

### 4. Verify

```sh
prompt_renderer 0 0.5
```

The two arguments are the last exit code and the duration in seconds. Leave the
duration out and the timing module stays hidden.

## Configuration

`~/.config/kanso/kanso.json` — every key is optional.

```json
{
  "location_module": true,
  "location_icon": " ",
  "location_color": "33",

  "git_status_module": true,
  "git_status_icon": " ",
  "git_status_color": "171",

  "duration_module": true,
  "duration_icon": "󰔛 ",
  "duration_color": "214",

  "arrow": true,
  "arrow_icon": "󰘍 ",
  "arrow_color_succes": "76",
  "arrow_color_fail": "196",

  "newline": true
}
```

| Key | Default | Effect |
|---|---|---|
| `location_module` | `true` | show the working directory |
| `location_icon` | `" "` | prefix for the directory |
| `location_color` | `"33"` | 256-color code |
| `git_status_module` | `true` | show the git branch (hidden outside a repo) |
| `git_status_icon` | `" "` | prefix for the branch |
| `git_status_color` | `"171"` | 256-color code |
| `duration_module` | `true` | show execution time |
| `duration_icon` | `"󰔛 "` | prefix for the time |
| `duration_color` | `"214"` | 256-color code |
| `arrow` | `true` | show the exit-code arrow |
| `arrow_icon` | `"󰘍 "` | the arrow glyph |
| `arrow_color_succes` | `"76"` | arrow color when the exit code is `0` |
| `arrow_color_fail` | `"196"` | arrow color otherwise |
| `newline` | `true` | line break before the arrow |

Colors are 256-color terminal codes, not hex — they're emitted as `%F{n}`. Icons
require a Nerd Font in your terminal.

Note the spelling of `arrow_color_succes` (one `s`). That's the key the binary reads.

## Scripting

`~/.config/kanso/kanso.lua` — optional. Runs after the JSON config is loaded, so it
can override anything in the `gui` table.

```lua
-- custom module, evaluated on every prompt
gui.add_module({
  icon  = " ",
  color = "141",
  value = function()
    return exec("node --version")
  end
})

-- static value, computed once at load
gui.add_module({
  icon  = " ",
  color = "76",
  value = exec("whoami")
})

-- keep the arrow on the same line as the modules
newline = false
```

### Globals

| Name | Type | |
|---|---|---|
| `exec(cmd)` | function | run a shell command, return its trimmed stdout |
| `exit_code` | number | exit status of the last command |
| `duration` | number | duration of the last command, in seconds |
| `current_dir` | string | working directory, `$HOME` shortened to `~` |
| `branch` | string | current git branch, empty outside a repo |
| `newline` | boolean | line break before the arrow; readable and writable |

### `gui.add_module`

Takes a table with three fields:

| Field | Type | Default |
|---|---|---|
| `icon` | string | `""` |
| `color` | string | `"76"` |
| `value` | string or function | — |

Modules render in the order they're added. A module whose value evaluates to an
empty string is skipped entirely. Functions run once per prompt; a string value is
captured at load time.

## How it works

On every prompt zsh calls the binary with two arguments — the last exit code and the
duration in seconds. The binary then:

1. loads `~/.config/kanso/kanso.json`, if present;
2. reads the git branch by shelling out to `git branch --show-current`;
3. spins up a Lua state, exposes the values above as globals, and runs
   `~/.config/kanso/kanso.lua`, if present;
4. prints the assembled prompt to stdout using `%F{…}` / `%f` escapes.

zsh picks up that output through `PROMPT='$PROMPT_TEXT'` with `prompt_subst` enabled,
which is what makes the escapes interpret as colors instead of printing literally.

## Repository layout

```
Kanso-prompt/
├── src/
│   └── main.cpp        the whole program
├── docs/               website, served by GitHub Pages
│   ├── index.html
│   ├── dependencies.html
│   ├── style.css
│   └── script.js
├── .gitignore
├── LICENSE
└── README.md
```

## License

Kanso is licensed under the **GNU General Public License v3.0**. The full text is in
[LICENSE](LICENSE) — the canonical version is at
<https://www.gnu.org/licenses/gpl-3.0.txt>.

Two third-party libraries are vendored at build time and remain under their own
(MIT) licenses:

- [sol2](https://github.com/ThePhD/sol2) — MIT
- [nlohmann/json](https://github.com/nlohmann/json) — MIT

Both are GPL-3.0 compatible.

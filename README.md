# A KDE Jukebox

<img src="https://invent.kde.org/kde/juk/raw/master/128-apps-juk.png" align="right"
     title="Juk logo" width="96" height="96">

[JuK](https://juk.kde.org) is an audio jukebox application, supporting collections of MP3, Ogg Vorbis,
and FLAC audio files. It allows you to edit the tags of your audio files, and
manage your collection and playlists. Its main focus, in fact, is on music
management.

Learn more about [Juk at KDE.org](https://www.kde.org/applications/multimedia/juk/).

## Features

<img src="https://juk.kde.org/img/screenshots/juk-3.10.1-tag.png" align="center"
     title="Screenshot of JuK" width="800">

- Collection list and multiple user defined playlists
- Ability to scan directories to automatically import playlists and music files
  on start up
- Dynamic Search Playlists that are automatically updated as fields in the
  collection change.
- A Tree View mode where playlists are automatically generated for sets of
  albums, artists and genres.
- Playlist history to indicate which files have been played and when.
- Inline search for filtering the list of visible items.
- The ability to guess tag information from the file name
  - Previous versions could also use MusicBrainz online lookup, but we need help to
    get that working again.
- File renamer that can rename files based on the tag content.
- ID3v1, ID3v2 and Ogg Vorbis tag reading and editing support (via TagLib).

## Installation

The methods listed below for each major OS are based on executing the
installation commands on a terminal window. Alternatively, you can use
your OS' package management app.

Unless using `sudo` to escalate privileges, the installation commands are
expected to be executed as the `root` user.

### Ubuntu

```
sudo apt install juk
```

### Debian

```
apt install juk
```

### CentOS, Fedora, RHEL

```sh
dnf install juk # On CentOs, use 'yum' instead of 'dnf'
```

### OpenSUSE
```
zypper install juk
```

### ArchLinux

1. Enable the `extra` repository on `/etc/pacman.conf`:
    ```
    [extra]
    Include = /etc/pacman.d/mirrorlist
    ```
1. Install the `juk` xz package:
    ```
    # pacman -Sy juk
    ```

### Other OSs

Find your OS and installation instructions on
[Packages Search](https://pkgs.org/download/juk).

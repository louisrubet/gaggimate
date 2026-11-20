# LAN OTA: flash the display over the local network

For development builds only. Release builds and the web UI update (GitHub releases) are unchanged.

## Password

The receiver is compiled in only when a password is available at build time, looked up in this order:

1. the `GM_OTA_PASSWORD` environment variable;
2. the first line of `~/.config/gaggimate/ota-password` (another file with `GM_OTA_PASSWORD_FILE`).

Create the file once, outside the repository, readable by you only:

```shell
mkdir -p ~/.config/gaggimate
(umask 077; openssl rand -base64 24 > ~/.config/gaggimate/ota-password)
```

The build warns when the file is readable by other users. Keep a copy in a password manager: losing it means
one more USB flash with a new password.

## Enable

The first build carrying the receiver must be flashed over USB:

```shell
scripts/build_webui.sh                # otherwise the embedded web UI is empty
pio run -e display -t upload          # USB, once
```

## Update over the network

```shell
pio run -e display -t upload --upload-port 192.168.1.42   # or <mdns name>.local
```

An IP address or a `.local` name as upload port switches PlatformIO to espota; `scripts/lan_ota_post.py`
passes the password. Every build you flash must be made with the password available, or the next one will need
USB again.

## Behaviour

- Listens on UDP 3232 once the display is connected to Wi-Fi as a station (not in access point mode).
- Answers only while the machine is in standby: an update never interrupts a shot or steam. Out of standby,
  espota reports "No response from the ESP".
- The screen freezes during the transfer, then the display restarts on the new firmware.
- The device connects back to the computer over TCP on a random port; the computer firewall must accept it
  (Fedora Workstation's default zone does).

## Security

The password is checked with ArduinoOTA's MD5 challenge; only its unsalted MD5 is stored in the image, so use a
long random password. The transfer itself is not encrypted: use it on a trusted network only.

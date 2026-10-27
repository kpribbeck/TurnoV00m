# TurnoV00m Multiplayer (LAN)

TurnoV00m keeps Crispy Doom's network code: co-op and deathmatch for up to
4 players, one PC per player. `TurnoV00m.exe` has networking built in, so
nothing extra needs to be built.

## Packaging

1. Build, from the **MSYS2 UCRT64** shell:

   ```bash
   cmake --build build -j
   ```

2. Assemble the distributable folder, from PowerShell in the repo root:

   ```powershell
   powershell -ExecutionPolicy Bypass -File .\make-dist.ps1
   ```

3. Zip it:

   ```powershell
   Compress-Archive -Path dist\TurnoV00m -DestinationPath dist\TurnoV00m.zip -Force
   ```

4. Copy the zip to every PC and unzip it.

Every PC must run the same zip. Don't launch the game from `dist\TurnoV00m`
before zipping: it writes `default.cfg` and `turnov00m.cfg` beside the exe,
including a player name that every PC would then share.

## Quick start

All commands run in PowerShell from inside the unzipped `TurnoV00m` folder.

**Host** (one PC):

```powershell
.\TurnoV00m.exe -server -privateserver -nodes 2
```

**Join** (every other PC):

```powershell
.\TurnoV00m.exe -autojoin
```

If `-autojoin` doesn't find the host, connect by IP instead (run `ipconfig`
on the host to get its address):

```powershell
.\TurnoV00m.exe -connect 192.168.1.50
```

The first time a PC hosts, Windows asks whether to let `TurnoV00m.exe`
through the firewall: allow it on **private** networks. Joining PCs don't
need this.

### Host flags

| Flag | Meaning |
|---|---|
| `-server` | Host the game and play in it |
| `-privateserver` | Don't advertise the game on the public server list |
| `-nodes N` | Start automatically once N players have joined, **host included**. Without it, the host presses **Space** in the lobby to start |

Joining PCs never need game flags: the host's settings apply to everyone.

## Game modes

Co-op is the default. For deathmatch, add one mode flag to the host command.

| Flag | Mode |
|---|---|
| *(none)* | Co-op |
| `-deathmatch` | Deathmatch 1.0: weapons stay after pickup, items never respawn |
| `-altdeath` | Deathmatch 2.0: weapons disappear on pickup, all items respawn after 30 s |
| `-dm3` | Deathmatch 3.0: weapons stay **and** items respawn |

Optional extras, also on the host only:

| Flag | Effect |
|---|---|
| `-warp E M` | Start on episode E, map M (`-warp 1 7` = E1M7) |
| `-skill 1`..`5` | Difficulty (1 and 5 also double ammo pickups) |
| `-nomonsters` | No monsters, players only |
| `-timer N` | End each level after N minutes. Without it, a level only ends at the exit |

Example: 3-player deathmatch on E1M7, no monsters, 10-minute rounds:

```powershell
.\TurnoV00m.exe -server -privateserver -nodes 3 -altdeath -warp 1 7 -nomonsters -timer 10
```

Every map in `TurnoV00m.WAD` (E1M1 to E4M9) has the 4 or more deathmatch
spawn points deathmatch requires.

## Saving and loading co-op

**Save:** during the game, any player uses **Save Game** from the menu. Every
PC writes its own copy to the same slot, as `turnov00msavN.dsg` in the folder
the game was launched from.

**Load:** **Load Game** is disabled in network games. Instead, the host starts
the next session with `-loadgame N`, where N is the number in the save file's
name (the first slot is `turnov00msav0.dsg`, so `-loadgame 0`). Every PC then
loads its own copy:

```powershell
.\TurnoV00m.exe -server -privateserver -nodes 2 -loadgame 0
```

For a load to work:

- Use the **same PCs** and the **same number of players**.
- Join in the **same order** as when you saved. Player numbers follow join
  order (host = player 1, green), and the save stores each player's health
  and weapons by number.
- Every PC must still have its save file: don't delete, move or replace the
  game folders between sessions.

Starting without `-loadgame` begins a new game with pistol starts.

## Testing on one PC

Open two PowerShell windows in the game folder and run one command in each:

```powershell
.\TurnoV00m.exe -window -server -privateserver -nodes 2
```

```powershell
.\TurnoV00m.exe -window -connect 127.0.0.1
```

## Notes

- **PowerShell:** the `.\` before the exe name is required. Type real IP
  addresses: `<` and `>` are reserved characters. In Command Prompt (`cmd`),
  drop the `.\`.
- **Shortcuts:** on a PC or totem without a keyboard, make a Windows shortcut
  to `TurnoV00m.exe` and add the flags after the path in its *Target* field.
  Always use `-nodes` there, since nobody can press Space in the lobby, and
  keep the shortcut's *Start in* field set to the game folder, because saves
  are written to the folder the game starts in.
- **Over the internet:** forward UDP port 2342 on the host's router to the
  host, or put all PCs on Tailscale or ZeroTier and use `-connect` with the
  host's address on that network.

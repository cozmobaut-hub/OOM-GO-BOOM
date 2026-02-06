# OOM-GO-BOOM
A C-based pseudo-virus that hogs system memory reservered for the Windows Desktop Compositor, causing Windows to OOM the processes contributing to the screen.

## Disclaimer

The writers of this code are not responsible for the way in which this code is used, implemented or distributed. ONLY RUN THIS ON VM'S, YOUR OWN PERSONAL COMPUTER FOR LEARNING PURPOSES, OR SOME OTHER SYSTEM THAT YOU OWN AND DON'T CARE ABOUT.

### What does it do?

## Observed Impact
Desktop: Display crash, 4x boot loops to recovery.

Server: Pagefile corruption, BSODs, IPMI reset needed.

No hardware damage—just brutal resource exhaustion.

## Countermeasures
Kill: taskkill /f /im hog.exe

Prevent: Job Objects, AppLocker, GPO process limits.

Test: VirtualBox/VMware (2-4 GB RAM cap).

Educational use only. Mimics malware tactics without infection/persistence.
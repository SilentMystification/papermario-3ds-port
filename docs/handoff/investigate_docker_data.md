# docker_data investigation

- Total size: docker_data.vhdx = 165,660,327,936 bytes (154.3 GiB). Path: C:\Users\Andrew\AppData\Local\Docker\wsl\disk\docker_data.vhdx. Not inside the repo.
- The repo has no docker_data directory. Grep of the repo (including ignored files) finds no match.
- Inside the VM, Docker data root /var/lib/docker holds volumes/ = 139.9G (148 volumes).
- Top volumes: pm3ds-work 31.5G, pm3ds-work-laneB 10.0G, pm3ds-work-v13c 6.3G, pm3ds-work-r13b 6.0G, streetpass-br-cache 5.7G, pm3ds-work-logoff 5.5G, pm3ds-work-logoff2 3.6G, pm3ds-work-tex4 1.8G, pm3ds-work-tex2 1.6G, pm3ds-work-box 1.6G.
- Other: buildkit 4.2M, containers 180K. overlay2 size not measured (timed out).
- Writer: Docker Desktop WSL2 backend stores the VM disk. Volumes are written by 3ds/build.sh (pm3ds-build*, pm3ds-ccache, pm3ds-src-*), 3ds/run_azahar.ps1 and 3ds/push_3ds.sh (pm3ds-work*, mounted at /work and /out).
- Images: about 15 GB total (pm3ds-build:1 2.1G, devkitpro/devkitarm 2.09G, jubeatmegamix/build 2.25G, montymintypie/llvm-mingw-xp 2.2G, output 1.95G, icam-builder 2.09G). Stopped container hopeful_hermann 1.51G virtual.
- Tracked or ignored: not tracked by git (not in repo). Not in .gitignore (not needed).
- Cause: the .vhdx grows with Docker volumes from repeated 3DS builds, and each per-lane or per-experiment volume (pm3ds-work-*) keeps its own build tree. The .vhdx does not shrink when volumes are deleted.
- Unknown: which volumes the user wants to keep. Overlay2 layer size. Whether streetpass-br-cache and ac3ds-work are still used. Whether the VHDX shrinks after compact (not tested).
- Next step for the user (not done): docker volume rm of unused pm3ds-work-* volumes, then compact the VHDX with Docker Desktop.
- Nothing was changed, deleted, or committed.

#!/bin/bash
set +e
cd /home/kongbai/海底捞平板
# 使用已经存在的 Xvfb :99，强制 Mesa llvmpipe，避免 WSL AMD/D3D12 路径
export DISPLAY=:99
unset WAYLAND_DISPLAY
export SDL_VIDEODRIVER=x11
export SDL_RENDER_DRIVER=software
export SDL_AUDIODRIVER=dummy
export LIBGL_ALWAYS_SOFTWARE=1
export MESA_LOADER_DRIVER_OVERRIDE=llvmpipe
export GALLIUM_DRIVER=llvmpipe
export LIBGL_ALWAYS_INDIRECT=1
timeout 8s ./haidilao_app >/tmp/lvgl_llvmpipe.log 2>&1
rc=$?
echo RUN_RC=$rc
cat /tmp/lvgl_llvmpipe.log

# 海底捞点餐平板

## 文档与工程事实
- 语言/框架：C + LVGL 9 开发版本；PC 使用 SDL2，开发板使用 Linux framebuffer/evdev。
- 实际构建目录：WSL 的 /home/kongbai/海底捞平板。
- E 盘目录是资料/交付副本；若缺少 lvgl/，不要在 E 盘直接编译。
- PC 构建：make -f Makefile_haidilao clean && make -f Makefile_haidilao -j4。
- ARM 构建：make -f Make_arm clean && make -f Make_arm -j4。
- ARM 输出：build_arm/haidilao_arm；PC 输出：haidilao_app。
- ARM 上传：make -f Make_arm send，正式运行还需要 simkai.ttf。

## 目录约定
- mycode/：显示、输入、字体、网络等平台/业务代码。
- ui/：LVGL 页面、菜单数据和图片资源。
- server/：C 服务器端（若存在）。
- build_arm/：ARM 编译输出。
- lvgl/、freetype_tmp/：第三方库/交叉编译依赖，初学阶段不要改。
- Markdown 文档：先读 开发与学习手册.md，再按其中路线读其他文档和源码。

## 修改和验证约定
- 修改 lv_conf.h 或其他头文件后，先执行对应 Makefile 的 clean。
- 一次只改一个小点，先编译，再运行/部署，再查看日志。
- 不要把 PC SDL 程序上传到 ARM 板，也不要把 ARM 程序当作 PC 程序运行。
- 文档应区分“需求设计”“当前实现”“已验证”“尚未实现”，不要把设计稿中的未来功能写成已完成事实。

# 在 Mac 上查看容器里的 `cv::imshow` 画面（X11 转发）

## 背景：为什么默认看不到

容器本身**不自带 X server**。`cv::imshow`、OpenCV 的 GTK 后端、GStreamer 的 `ximagesink` 都要通过 X11 协议把窗口画到一个 X server 上，容器里没有，所以默认会崩：

```
OpenCV(4.9.0) .../window_gtk.cpp:638: error: Can't initialize GTK backend in function 'cvInitSystem'
```

我们的开发链路是 **Mac → SSH(3322) → 容器（Cursor Remote-SSH）**。解决办法是 **SSH X11 转发**：让容器里的 GUI 程序通过 SSH 隧道，把窗口回显到 Mac 的 XQuartz 上。

> ⚠️ **关键限制**：Cursor/VS Code 的 Remote-SSH **不携带 X11 转发**，所以按 F5 由调试器启动的进程默认仍然没有 `DISPLAY`、imshow 会崩。下面有两条路：终端手动跑（最简单）、或「借用」隧道让 F5 也能显示。

---

## 前置条件（一次性，已就绪）

| 项 | 状态 | 说明 |
|---|---|---|
| Mac 上 XQuartz | 需自行安装 | https://www.xquartz.org ，装完**注销重登一次** Mac |
| 容器里 `xauth` | ✅ 已装 | sshd 转发 X11 必需 |
| 容器里 `xeyes` | ✅ 已装 | 用于快速验证 X 通道 |
| sshd 配置 | ✅ `X11Forwarding yes` | 端口 host:3322 → 容器:22，用户 root |

---

## 方法一：在 `ssh -Y` 终端里手动跑可执行（最简单、最可靠）

适合：单纯想看画面、不一定要 Cursor 断点。

```bash
# 1) Mac 终端：用 -Y（trusted）转发，比 -X 更稳，GTK/OpenCV GUI 推荐 -Y
ssh -Y root@ip -p 3322

# 2) 进容器后，确认拿到了转发的 DISPLAY（不能是空）
echo $DISPLAY        # 通常是 localhost:10.0

# 3) 先用 xeyes 验证 X 通道本身通不通
xeyes                # Mac 上应弹出「一双眼睛」窗口 → 成功

# 4) 跑 VideoPipe 样例（需先准备好 vp_data，见文末）
cd /data1/code/VideoPipe
./build/bin/1-1-1_sample     # 人脸检测窗口会回显到 Mac
```

> `-Y` 与 `-X` 区别：`-X` 是 untrusted，会限制部分 X 扩展，GTK 偶尔报错；`-Y` 是 trusted，放权更多，GUI 最稳。开发环境用 `-Y` 即可。

---

## 方法二：让 Cursor 的 F5 调试器也能显示 imshow（借用隧道）

Cursor 的 F5 不走 `ssh -Y`，但它启动的进程和 `ssh -Y` 终端在**同一个容器网络命名空间**里，可以共用那条隧道。

**步骤：**

1. **保持方法一的那个 `ssh -Y` 终端开着**（它持有容器内 `127.0.0.1:6010` 的 X 代理，关掉就没了）。
2. 在该终端里 `echo $DISPLAY`，记下输出（通常是 `localhost:10.0`）。
3. 把 `.vscode/launch.json` 里每条配置的 `DISPLAY` 改成这个值：
   ```json
   { "name": "DISPLAY", "value": "localhost:10.0" }
   ```
4. 只要那个 `ssh -Y` 终端不关，F5 启动的进程就连同一个 X 代理 → imshow 显示到 Mac。

> 鉴权：sshd 把转发用的 cookie 写进 `/root/.Xauthority`，F5 进程也是 root、默认读同一个文件，通常无需额外设置。
> 若 F5 时报 `authorization failed` / `cannot open display`：在 `ssh -Y` 终端里 `echo $XAUTHORITY`，若是临时文件路径，把 launch.json 里再加一项 `{"name":"XAUTHORITY","value":"<该路径>"}`，或直接 `cp "$XAUTHORITY" /root/.Xauthority` 后重试。

---

## 常见问题排查

| 现象 | 原因 / 处理 |
|---|---|
| `echo $DISPLAY` 为空 | 连接没用上转发；确认用的是 `ssh -Y`/`-X`，且容器 `xauth` 已装、sshd `X11Forwarding yes` |
| `xeyes` 报 `can't open display` | Mac 端 XQuartz 没起来；打开 XQuartz.app，或注销重登 Mac 后重试 |
| `Authorization required, but no authorization protocol specified` | cookie 没共享；见方法二的鉴权说明 |
| F5 调试时连不上、终端跑可以 | 那个 `ssh -Y` 终端被关了，或 `launch.json` 的 `DISPLAY` 写错；重新 `ssh -Y` 并核对 `echo $DISPLAY` |
| `cvInitSystem / GTK backend` 崩溃 | 进程没拿到 `DISPLAY`；检查环境变量是否注入（Cursor 用 launch.json 的 `environment`，终端用 `ssh -Y` 自动注入） |
| 画面延迟/卡顿 | X11 转发过网络本就有延迟，正常现象；调试可降低刷新率或用文件输出代替 |

---

## 备注

- **`vp_data` 仍需自行准备**：样例用 `./vp_data/...` 读模型和测试视频，X11 通了也还要把 `vp_data` 下好放到 `cwd`（`launch.json` 里目前是项目根目录）。下载地址见 `README.md`（谷歌/百度网盘）。
- **性能**：X11 转发适合调试/看效果，不适合高帧率实时预览；要稳定录像用写文件的 sink 节点。
- **不想折腾 X11 时**：调试推理/流水线逻辑可以走无屏幕（headless）——注释掉 `board.display(...)`、把 `vp_screen_des_node` 换成文件输出 sink，画面单独看输出文件。

# Snow Shot Community（Windows x64 测试版）

> **独立开源 Fork**，源自 [mg-chao 的 Snow Shot](https://github.com/mg-chao/snow-shot)。
> 本项目**不是原作者发布的官方版本**，也不代表原作者提供技术支持。
> 原始版权声明、GPL 授权与第三方许可依然保留。

[English](README.md) · [测试版更新记录](docs/community-changelog.md) ·
[Windows 构建说明](docs/community-windows.md) ·
[文档导航](docs/README.md) ·
[本 Fork 的 Issue](https://github.com/wxyu71/snow-shot/issues)

## 当前状态：v1.5.8-beta.1

**仍处于测试阶段，不是稳定版。** 必须先通过 Windows 正式构建、打包和
安装程序自动化检查，才能发布供个人安装的 Beta1 测试包。原版 1.5.7
覆盖升级、实际本地 OCR 模型及云 OCR 仍需在 Windows 电脑上手动实测。
请勿将上游 Release 当作本 Fork 的测试版。

## 核心功能

- 保留智能窗口/元素框选、截图、快捷键、批注、贴图及原有引擎。
- **快速离线 PP-OCRv6**：本地运行，无需联网；识别完成自动复制全部文字，
  并保留可局部选择的文字层。
- **唯一的“在线 OCR”按钮只使用 PaddleOCR**，且仅在用户主动操作时
  发送图片。用户自行配置 AI Studio HTTPS 地址、Access Token 和模型
  （PP-OCRv6、PP-StructureV3、PaddleOCR-VL-1.6），输出 Markdown，公式
  以 LaTeX 保留。不会使用自定义视觉模型服务。
- **截图历史和固定图片历史默认开启**，仅保存在用户电脑本地；
  可手动关闭或配置保留期限，不会因保存历史而自动上传云端。
- 不含 Snow Shot 登录注册、会员、积分支付、分析统计或遥测。
- 上游定时同步具备商业文件保护；未经 Windows 回归测试的源码合并必须人工审核。

只有用户**主动使用在线 OCR**，图片才会直接发送到其配置的 PaddleOCR
AI Studio 服务，不经过 Snow Shot 账户服务。Windows 上 PaddleOCR Access Token 使用当前
用户作用域的 DPAPI 保护；其它平台目前仍保存在用户 Qt 设置中，应按敏感信息
处理。详见[此 Fork 的隐私说明](docs/community-privacy.md)。

## 构建与测试

需要 VS 2026、CMake 4.2+、仓库指定的 vcpkg 工具链、
Qt 6.12.0 静态 Release 构建及 OCR 发布资源：

```powershell
./scripts/bootstrap.ps1 -Preset snow-shot-msvc-release
./scripts/build.ps1 -Preset snow-shot-msvc-release
ctest --test-dir build/snow-shot-msvc-release -C Release --output-on-failure
./scripts/package-snow-shot.ps1 -Architecture x64 -SkipBuild
```

详见[Windows 构建与发行说明](docs/community-windows.md)。
真实百度 API 测试：在本机安全配置 `PADDLEOCR_ACCESS_TOKEN` 后执行
`python scripts/paddleocr_live_e2e.py --live`；
离线协议测试：`python scripts/paddleocr_live_e2e.py --selftest`。

## 开源许可与署名

沿用原项目适用的 GPL-3.0-or-later 授权及相关依赖许可。
保留 [LICENSE](snow_shot/LICENSE)、[COPYRIGHT](snow_shot/COPYRIGHT)
和第三方许可声明。原作者保有其作品版权；本 Fork 不宣称与原作者存在隶属关系。

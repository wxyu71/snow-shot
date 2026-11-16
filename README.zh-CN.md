# Snow Shot Community（Windows x64 测试版）

> **独立开源 Fork**，源自 [mg-chao 的 Snow Shot](https://github.com/mg-chao/snow-shot)。
> 本项目**不是原作者发布的官方版本**，也不代表原作者提供技术支持。
> 原始版权声明、GPL 授权与第三方许可依然保留。

[English](README.md) · [测试版更新记录](docs/community-changelog.md) ·
[Windows 构建说明](docs/community-windows.md) ·
[本 Fork 的 Issue](https://github.com/wxyu71/snow-shot/issues)

## 当前状态：v1.5.8-beta.1

**尚处于测试阶段，未达到正式发行条件。** Windows x64 完整版、离线 OCR
资源、从原版 1.5.7 覆盖升级和原生回归测试均需实际通过后才能发布。
请不要把原作者 GitHub Release 中的安装包误当成此 Fork 的测试版。

## 核心功能

- 保留智能窗口/元素框选、截图、快捷键、批注、贴图及原有引擎。
- **快速离线 PP-OCRv6**：本地运行，无需联网；识别完成自动复制全部文字，
  并保留可局部选择的文字层。
- **云端智能 OCR**：直接调用[百度 AI Studio PaddleOCR 官方 API]
  (https://ai.baidu.com/ai-doc/AISTUDIO/fml7mozw5)，使用用户自己的 URL
  与 Access Token；支持 PP-OCRv6、PP-StructureV3、PaddleOCR-VL-1.6；
  输出完整 Markdown，自动复制，同时支持预览、编辑、局部复制。
- 不含 Snow Shot 登录注册、会员、积分支付入口。
- 上游定时同步具备商业文件保护；未经 Windows 回归测试的源码合并必须人工审核。

云 OCR 会将选中的图片**直接发送至用户配置的百度 API**。
目前 Access Token 由用户目录下的 Qt 设置文件明文保存，不要上传此文件。
详见[此 Fork 的隐私说明](docs/community-privacy.md)。

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

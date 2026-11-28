# Upstream 1.6.7 integration — REVIEW ONLY, NOT MERGEABLE

**This is an intentionally incomplete review tree, not a release candidate.** The marker is forbidden by the existing Community deletion policy so preflight must fail until every retained path is resolved and the marker is removed by a reviewed commit. Do not merge into #15 or main merely because upstream ancestry is present.

- Community parent: `899dcec3edbca1c3ca3662fd89573bbc702f5881`
- Previous real merge base: `761fb564df133648c9f8f90573060aa7e82b52c9`
- Upstream release tag: `v1.6.7_snow-shot` → `a3537149f34ac9a5fe8e01ff52ac296dc7adffb1`
- Upstream main parent (includes the tag and the two subsequent commits): `cf87c82639588f5b9b9c03cd16129b7d578406d6`
- Changed upstream paths: 546; imported exact non-overlapping, non-protected Git objects: 455; already identical: 2; intentionally excluded deleted paths: 10; pending manual resolution: 79.

## Conflict resolution contract

1. Keep independent Community update trust, GitHub origin, RSA-PSS feed, Inno 7 production path and beta version. Reconcile upstream updater fixes using the Community transaction contract; never restore upstream update feeds or disable signing gates.
2. Keep WebDAV, independent PaddleOCR, history background OCR, quota handling and token privacy. Port upstream OCR geometry and copy workflows onto the Community controllers; do not restore deleted cloud/account files or commercial endpoints.
3. Reconcile CMake/source moves, native tests, configuration schemas, translations and icon manifests together. Upstream refactors rename/remove headers and move ownership; current retained paths are **not** yet verified compatible. Merge translation keys by context/source, preserve Community keys and all supported locales.
4. Audit native OCR submodule and asset changes as one pinned runtime upgrade, including both architectures' descriptors and offline/online package inventory; do not blindly take them independently.
5. Verify capture, recording, clipboard, local OCR, UI, WebDAV, installer smoke and signed end-to-end updates before removing this marker. No release secrets or publish permission are used on this branch.

## Pending review ledger

Each row records exact base / Community / upstream blob (or absence). No conflict is claimed resolved by merely choosing the Community blob in this provisional tree.

| Path | Reason | Base | Community | Upstream |
|---|---|---|---|---|
| `CMakeLists.txt` | community-protected | `813653aba3f2669a6b7af871884f85569240ae82` | `11e175af013b38bb04218fbf5e38c1f7c4ce4bb7` | `8a5e8685a911212c0ca7990b878db3c7f7d0bfad` |
| `cmake/SnowShotMini.cmake` | community-protected | `457393578831bb04175be1f6a3e510390efc3eb0` | `e8d2984b91e73d7ba5606fc82025b4e267ae3814` | `3eface73ca11caaf053f8aa081a4032da6b241d7` |
| `scripts/package-snow-shot.ps1` | community-protected | `7c7390af7cf8d02d60a9405e5d8b3b340e52856b` | `dde0d28269e94f3cca7f22068947c3f0b0395644` | `e79920beec19c5c696df7fb31b8959f53279701b` |
| `snow-crates/crates/simd-paddle-ocr-rs` | community-protected | `cac552c0ed5a10978dded53faf5df1c1309eb057` | `cac552c0ed5a10978dded53faf5df1c1309eb057` | `b0d8bf38c6e83e3bfe8b58a3dad6ac24a743df7f` |
| `snow_shot/CMakeLists.txt` | community-protected | `77292fb4da89bbd841d302d7b6b47eef69ed52be` | `5356deb15c6ed7a99688a653b1a71dbb07bac40b` | `76832efadade520e3362996e448c0649dd04a48c` |
| `snow_shot/i18n/core/snow_shot_core_en_US.ts` | community-protected | `f1fff393ceaea8ca0f1bbdc568b62f7230de21eb` | `f97c1ce1d9ad547877760ec8c0be8509a55c2c95` | `f7beb382ad12cf17a08bd603293295cf8352c73c` |
| `snow_shot/i18n/core/snow_shot_core_ru.ts` | community-protected | `e4f2e29bb13f598f8e195df123744048221d70cc` | `4307925792e80644cd723fb69eb6965674c8851e` | `8d12eed252bbe4852ca366b445a8f6c1a344a8e3` |
| `snow_shot/i18n/core/snow_shot_core_zh_CN.ts` | community-protected | `f4670e7c98fbe2272c205859a12376503f448201` | `1c634789010b51e6540f65dc2d9c0985bde91675` | `c3168720faf33fbb37bf6b4372b5c52765d713d2` |
| `snow_shot/i18n/core/snow_shot_core_zh_TW.ts` | community-protected | `66b2371aeb03eaf8e6bb5545abee878079206d5b` | `206754db261d6fe45f7733ec93bc1ba211fe48f9` | `516d6c89b3d7fcf13437b1e1abc762dba57e54d6` |
| `snow_shot/i18n/history/snow_shot_history_en_US.ts` | overlapping changes | `89b4ca5bcc07c90ce83546e672a352a60b891bc2` | `b2aa25b5c1205ec1d86fd4bbc2dd6d772f76cbc8` | `f3e878184504b50833d4303f94973b39a7711fa3` |
| `snow_shot/i18n/history/snow_shot_history_ru.ts` | overlapping changes | `09130992bb69c372e68842cdac7b41630d302f73` | `4412f467ed87bb48e73035058a1d9e128b86b4a7` | `d9108dfd2a630b867ea488fe95f093c9c86e72e9` |
| `snow_shot/i18n/history/snow_shot_history_zh_CN.ts` | overlapping changes | `360dbe7cf3a0d9741df1f88d32b8f99efccc8881` | `5eb9740ed1a9f2ffaa625eaedaaf5cda94ff81d4` | `e57738be735e70c899c3cb6803ffecd2af82d9d8` |
| `snow_shot/i18n/history/snow_shot_history_zh_TW.ts` | overlapping changes | `ca16d4b901e14a4a0a69fbb1a8ba08d63c6a7030` | `f6c022a3c62002eb7321eb031ad7ecbb2ff9e577` | `4284c09e98375e589db557a78b7c1433c669f835` |
| `snow_shot/i18n/modules.json` | community-protected | `1691edf33e6eafe8f78f77d8bcaf4d474b43bc1f` | `e64c75189f8d454a866a4184880e02be5f36878c` | `adbefa238f960a5be2434e17e47bb2fb3bfdfe64` |
| `snow_shot/i18n/recognition/snow_shot_recognition_en_US.ts` | overlapping changes | `ab2dc74d03b10d8954925bdb64f8ddac6e5c93c6` | `130ac97c83710e4a7a715a736eef8266ed54ad84` | `be4b6ac8c9b610aebaf7eb706e3bb634896cb608` |
| `snow_shot/i18n/recognition/snow_shot_recognition_ru.ts` | overlapping changes | `e872f6670a701e32aeba870394ba5a291a153d54` | `8cd407f12eb46f512a282a5cf50c4c44eb8b5ffb` | `03fcd637d049297a74f6a4f3ecede4945bc7973f` |
| `snow_shot/i18n/recognition/snow_shot_recognition_zh_CN.ts` | overlapping changes | `2964e6755b04efd74d86c8e94ad09e605e122162` | `883fba425289bebc807c9c19513e343a8677e87b` | `62625b810ac7e4c98f5bdae2bfa94c5907331972` |
| `snow_shot/i18n/recognition/snow_shot_recognition_zh_TW.ts` | overlapping changes | `6495988f0d827f9171b74f4ebb5d2f5da23c04b0` | `90e5ef8fb971cf0bd57aae5276bfd1d52e330680` | `51ec96ecefb241b790eaabd001253285c4f6a0e0` |
| `snow_shot/i18n/settings/snow_shot_settings_en_US.ts` | community-protected | `685e9a505da2dcc90a09601c27684a4f67b37d10` | `cf83c0803d23a35d387d221bc789911252fee128` | `dc0e43877783e35359e6cec46abae0f13eab1d67` |
| `snow_shot/i18n/settings/snow_shot_settings_ru.ts` | community-protected | `f7ad30012edb2c863c7349087b344a62ae7c591f` | `56b2b0f187552f3b48b6bc11f1408f3a69ca7a2e` | `617891da60ef5ee8aa1759cdc1ce8b0212070255` |
| `snow_shot/i18n/settings/snow_shot_settings_zh_CN.ts` | community-protected | `a79d6685ea0c8ea7ec178ee0f42584ed7193376b` | `b27781008620f07f1b661bb76a14d0fe73c5e185` | `cde408a9ec63b03f1ab3317eab31ed0a5bcbc68a` |
| `snow_shot/i18n/settings/snow_shot_settings_zh_TW.ts` | community-protected | `6e9fa6cb3bfeaf07357b93b905cc98a6418b4e79` | `f896d7b9084d1dc5b01903ee3f7ec5d107156133` | `b03e73cda89d515ba262a161d1ccec39ec332698` |
| `snow_shot/include/snow_shot/presentation/screenshotcontroller.h` | overlapping changes | `b7a4b70a3cebd0ad8c2ecc105e3d7bc1369e4154` | `fedf90754c1d2f1127dc0d841ab90121b6379831` | `425717487cd1b04d67b1359d7c1bfcdb9402375b` |
| `snow_shot/include/snow_shot/presentation/screenshotimageconversionview.h` | community-protected | `82362d91c6f86177e6ab726ac645a530b4fc1e81` | `ffefa69c16966331cedac9669cf42295e8644c1b` | `6ad4e1f403e969159907cdaae46740e566df83a3` |
| `snow_shot/include/snow_shot/presentation/screenshotocrcontroller.h` | overlapping changes | `28791de3e67991d4d0224ed597b1967ac01587db` | `921854494e2b01cf4097ce0ce728b069abba4315` | `6ac65a2e11e014e212f6a9918a7a2d3b7358f546` |
| `snow_shot/include/snow_shot/presentation/screenshotrecognitionsessioncontroller.h` | overlapping changes | `7f28392bcf519c62faaccbe508d3ebc2966c22bf` | `dd8d87e27ce2877c448952e971cfeae63e264cae` | `35881a54bd54f8713623a870064a472d3f6a9e43` |
| `snow_shot/include/snow_shot/presentation/screenshotrecognitionwindow.h` | overlapping changes | `f20ec3de1c697676acaf62ad71cd0a1555084fc3` | `765c2713a20d7cdbeb2e9505903dc62b3bdd09a0` | `99ed00dde98d8bfd6dcf517f0a7b5e7feda996d7` |
| `snow_shot/include/snow_shot/presentation/settings/settingsbackend.h` | overlapping changes | `d0858de732601d4b51787b641868309d52f817c2` | `4ed4dc55e52bb69a392b3c3615259181663805cd` | `121e99861bb30b8a932fdfd3a46438b2f93ca9d4` |
| `snow_shot/include/snow_shot/presentation/settings/settingscatalog.h` | overlapping changes | `7eaf852d8709d29c5b0b722d37b1e8f799d8e191` | `7c5a3a038ca8a8781ffab277a8c3694f5d92e650` | `48e1ba24eb100d5c99c8f6539c6315ab7942b7ff` |
| `snow_shot/include/snow_shot/presentation/settings/settingsruntimesession.h` | overlapping changes | `a9bdd425229aa92be162cf86603a6b8001d5bc17` | `ebb9439085b4ab3c5a8390b630adec5092207547` | `fa5c0729b463421b9fad7a80fc459692d8f36d0e` |
| `snow_shot/include/snow_shot/storage/configurationarchive.h` | community-protected | `08383fd095ed3bded5dd20aefd1bc0d412341cca` | `3405e1edf6c0786a7529091b371d4e44b516b509` | `0e4bf745a7818bd7f19a690c6f4205867ea772e0` |
| `snow_shot/include/snow_shot/storage/configurationstore.h` | overlapping changes | `15aee33a2f81eeee550e46f002e4e2f454f055b0` | `34a466557cc5b75994cc38c696124aef1fb0afd8` | `09d11c3cee135704591f540fe1bf92124fce237c` |
| `snow_shot/include/snow_shot/storage/settingsadapters.h` | overlapping changes | `4883f82c897e5a57297fec214a82679eb72aa5e5` | `6ccb0033651025cb111b2b2d321c348f343b8cc6` | `2603f0a962dc4f249dd47a9f7c68943722b658b9` |
| `snow_shot/packaging/ocr-models.md` | community-protected | `fbb696e995b1c7906b3cc98e72c8ae84274d8c92` | `b52564ee829b7ec8ac70dd5d67ede1b4521f75ee` | `e9c96581c61dc12d20ab8ac52e1d65e71d8f7671` |
| `snow_shot/packaging/snow-shot-ocr-asset-manifest-arm64.json` | community-protected | `f26e4cc38e60bbb9eee2744e8d37b5dff455feda` | `f26e4cc38e60bbb9eee2744e8d37b5dff455feda` | `3bc405aa46abd73adddb9a3dcfe2c1fdaeae93ac` |
| `snow_shot/packaging/snow-shot-ocr-asset-manifest.json` | community-protected | `ae320cb6aba873c5279b69aa2f344133150d06be` | `ae320cb6aba873c5279b69aa2f344133150d06be` | `9d1beab34336318898d956973f9ade8200152ec3` |
| `snow_shot/resources/icons.manifest.json` | community-protected | `6191144d6f3a3e0a8eda790fdcfb7701f16c099a` | `89c14b02c1dd096be23b94705b684f4a39ac92df` | `db533276eae6aeb4ef2e894f51bf24bda4766513` |
| `snow_shot/rust/snow-shot-updater/src/platform/windows_impl.rs` | overlapping changes | `ddf747e15eb25177a7b6584e2bc516f82256b5cb` | `1f161987b191e1bc957efa74aaa4ce0501dbd5ec` | `957de48ff4e28aa41a557d178f09fa4211bbf0c6` |
| `snow_shot/rust/snow-shot-updater/src/transaction.rs` | overlapping changes | `11418541c58cfd4416a2640683ee1654e1542bf8` | `0f2017b4121c70233e8abf2ed2dc4f5159d16736` | `f442a37b94928f31a35dc25b549f86cb46df8c4a` |
| `snow_shot/src/app/applicationcontroller.cpp` | community-protected | `e7b01c56c38a46c8679ba8d873846b797dbc0a68` | `3ee88a44c65314bc3bb3f2a4fa4083200969ce2e` | `cab64094bdf2b54af2bfd256b628ff9f93893bab` |
| `snow_shot/src/app/main.cpp` | community-protected | `26a21249e26d0cf5315c5e55aed5b681d6b24613` | `a0016ba7660c686e021c40076c02523106a21c81` | `04cee4adb8e2638657b6326e3a125e1f82873d8d` |
| `snow_shot/src/app/mcp/mcpapplicationservice.cpp` | overlapping changes | `c1ea81d938b4fd78efdc950c1ae81236b55d9ce2` | `93dcf4fabd8743fc51881daf395519767bc9485d` | `4923690c132ada4cf4603696f70a329975c40480` |
| `snow_shot/src/network/snowshotapiclient.cpp` | community-protected | `37a1fa17e816df496b869396621673f1e7c3504f` | `3c50f846958f16302b2ebe4c93643d3e9e0f64cb` | `d696e2816557adcdab298e53c13676c957bfac84` |
| `snow_shot/src/presentation/components/aboutpagewidget.cpp` | community-protected | `8c2b33f622a478fa298ed0f6ceb70ae7a6e5e616` | `953cc9bfb77e66c1aef609b9c6e9b4e82703d0f1` | `8a338cd430918b07e6c93d8be6b7141e9187c2a0` |
| `snow_shot/src/presentation/components/settingspagewidget.cpp` | overlapping changes | `f66dbdb9571f7b2f049e25ab72f9c1b553a9f2b5` | `a92c316446c97f549d16d6a483dcc6ece451f0ab` | `87c90e819de40014c21544c8a8a99ed7da7b4939` |
| `snow_shot/src/presentation/ocr/screenshotimageconversionview.cpp` | community-protected | `0ce13c62f0594cd81c28897f95cf717cea6d817a` | `6f372de0ec7cdc76ad06b4a8de0c9b56674892a3` | `cd7a79b83cdb6fb3548b53399d161836bbcefa19` |
| `snow_shot/src/presentation/ocr/screenshotocrcontroller.cpp` | community-protected | `e8a460d86e1355adee12c30b58df173b66b36ad7` | `874c03609f15c256af66a845380e5af987612dc5` | `b35f50c242e0d02224fc6c1fec306d71d062ccfa` |
| `snow_shot/src/presentation/ocr/screenshotocrrecognitionservice.cpp` | community-protected | `ae5fbe73d97eb001a478a80d540bd6a9a0dbbbe2` | `dce3a998bd8981cc9d490ae01fd9ba1f0e9c4699` | `bd975150a8696864b66a4e9b691ffb1f2c633485` |
| `snow_shot/src/presentation/ocr/screenshotrecognitionsessioncontroller.cpp` | community-protected | `8b4f4b9550a59857dbd4954283511918db25370d` | `5fb6dca55571e888dc5713252643ed916e6111ae` | `1ec43901154f7fc69174ef5b81d9ab1a9a4d2c14` |
| `snow_shot/src/presentation/ocr/screenshotrecognitionwindow.cpp` | overlapping changes | `4f27bc57baaada190686a8aa99b5fa892fa52cae` | `28acb8998f3579504972ed57d6d4d45c226ed101` | `a744c2ec9dbff7537a72e5507af7bf4fd2ead5ce` |
| `snow_shot/src/presentation/recording/screenrecordingcontroller.cpp` | overlapping changes | `1b46f96069bdd70bbd712606b765156e2549b43e` | `7401ef7fd10279200b364e1a95a5a3872ee8fb19` | `eda22512fd7cc350df8567db4d654b44c470f3eb` |
| `snow_shot/src/presentation/services/globalmousemanager.cpp` | overlapping changes | `965b800a49840e08b2804833fb0ed3256e0fc64d` | `d3ec4e807035a13dfdff39150b1bd4b321236e93` | `31235d284d878cb9726ba061eff89e640c7b732a` |
| `snow_shot/src/presentation/services/globalshortcutmanager.cpp` | overlapping changes | `979eb9b1fe56887deb1b498a6c21cc4bf4513480` | `1ca47dbe4e7f29bfc07f84ede738b83f7cb10327` | `29a2f4878e4157dd532fa3732be36e0114d207d1` |
| `snow_shot/src/presentation/services/screenshotcontroller.cpp` | community-protected | `5c49380c01bbaeb129b4f9617d17194c28e1a0eb` | `e98243ffb391c591cde72ff572785e17de3a6585` | `32a644877c20f4086701963e6b6e48a35d53d5a3` |
| `snow_shot/src/presentation/settings/settingsbackend.cpp` | community-protected | `f01dd4353d49254edcbdba66c897214eb9de57b7` | `9615171dc7dd273fe1dc99d501115d75ca7869cc` | `7ad281b4de427be1875ec75dba0d2212463f0fd6` |
| `snow_shot/src/presentation/settings/settingscatalog.cpp` | community-protected | `4e6cb9ef46bcec460a8af18b5d5d386e9c3ee5f1` | `e77209b9af4955f019a02ff214461e2bcf43e927` | `159d12cb6895104651df9f54288500c7045854dd` |
| `snow_shot/src/presentation/settings/settingsruntimesession.cpp` | overlapping changes | `1f8f0ce751afe84cfba165a2941713f2a54c1d54` | `4fa5f23462b266840386ab3eac79a56b1484a69c` | `43a93e7a5cfbca3e4e8dfd99d8c05ab22807dba0` |
| `snow_shot/src/presentation/tools/screenshottoolpalette.cpp` | overlapping changes | `ab61407c7db0c12f9dffadf76686d0980b1732bc` | `d470ee8dd6bc7396321b82dab1191bd50d6d0231` | `c35f5c251694fbd66f7f9dd4a1d22724b7817dd5` |
| `snow_shot/src/storage/configurationarchive.cpp` | community-protected | `d989c0ac072e2ebae905c6e6e4ae2cc610cf4165` | `7860b8530d340a096ce11b7e4baaca7b14e8d566` | `201473f30f9e8b2430cbc588753e720153e3da24` |
| `snow_shot/src/storage/configurationschema.cpp` | community-protected | `e72b34b1029931eef0c8298b048ad76fb0fd7a54` | `b79123dccde695a27e1a7eb6729d3b03a0883594` | `abaf5b7b33f64ff9f81d53ac3b43687e9d5834c1` |
| `snow_shot/src/storage/configurationstore.cpp` | community-protected | `f6a294f7b0b8daa0d4c3be04c1d7e61632d0a3ae` | `11d50cea6384710eb9200e5cfca06a440943b488` | `e63c76fbd552849297e60b308eb066258b3de70a` |
| `snow_shot/src/storage/settingsadapters.cpp` | overlapping changes | `e8bbcacf8968ec8e7c7f1efdd4cc74bde2b749e0` | `99d6d99279d01c20dc3f6341d336d36f4560928f` | `256156497e0bc7beaedb9b194b6711cad953b7a7` |
| `snow_shot/src/update/startupupdate.cpp` | community-protected | `a445cc9bc6a7d079fb4bf1fff45f0e1a75361356` | `a445cc9bc6a7d079fb4bf1fff45f0e1a75361356` | `7b336dd3c6a346a7faf20e9ad267ce22b318f3c2` |
| `snow_shot/tests/about_page_tests.cpp` | community-protected | `e13c214ce40293be6fa189732ebc74cf7e8d0b64` | `4e8b36c59c4cb356b75b1c4660d6edd2fa469e00` | `6b5b089a054cdf685692d72029d24ff425acb2dd` |
| `snow_shot/tests/application_storage_tests.cpp` | community-protected | `f5c623920713ff50177e730853ce9c887cc9fe67` | `3534f66ba5abc9c675fb30a6cc010cc1dc83829a` | `5042c19e15a4a9922a467bbcaa306d0bc4af6931` |
| `snow_shot/tests/configuration_archive_tests.cpp` | overlapping changes | `cc4e78079eb688846faa47cbdf625f72dc2028c5` | `18ca0703e01d6788254d0e0524a00758bad9bf60` | `f7521f58e2b7e3700ce8fb19c97dfce6e54c7ba5` |
| `snow_shot/tests/configuration_schema_tests.cpp` | overlapping changes | `26cd232753a5dec05d3a5096b59d927c1ce6fcc1` | `e61834f1a33439317f0f3b6fca7ff745822ffe29` | `01fcc9412ba443ae65ad57597de69e6b6c751637` |
| `snow_shot/tests/global_shortcut_manager_tests.cpp` | overlapping changes | `c83801a8710e204687c5d0de97e4820dd719e35a` | `9c97e9096a2ffe08ef7d158f93262ef512bf6f2b` | `0c8cf56a114b323364ddb15029b52b312637ecdd` |
| `snow_shot/tests/image_conversion_tests.cpp` | community-protected | `a75f2d60d999041636a0d72af339cdb11780e1c0` | `bddde718a8b762f740248a7c2f19ca327f2cb1f4` | `6ea2f19642373b0565427e66c8e72cf6195ea856` |
| `snow_shot/tests/mcp_application_fixture_tests.py` | overlapping changes | `a0637e7250f3dbb4ad016443c1dd8ceb61792747` | `05e267e5d26a163e57dcef2c9a175093802d1ca4` | `9e28eb550d37b699bd3c96e6375a73a76321395c` |
| `snow_shot/tests/mcp_application_tests.cpp` | overlapping changes | `3d28f0239840d5927fc43ddcf9d78702bf45f922` | `f577b9dcd0e59fdea3cc76360c6261833d23f3f8` | `f75781c2ed2542130f9c2d8162782666b96fcead` |
| `snow_shot/tests/screen_recording_controller_tests.cpp` | overlapping changes | `3b06a7444af17e82b31f95f5681ba6d32df0a045` | `2624e371dfc1c273444c7d6aa140c566b3c66b6d` | `0410c04a66e8ea9902f5359186fba5afca318f4b` |
| `snow_shot/tests/screenshot_ocr_assets_tests.cpp` | community-protected | `0281115b0048a7b6927a6f7cfbbfacb29d8b0de8` | `182dbdf54c1dc937da076d1c60fdb3a49057fead` | `21dae11114faeea3bf16709f399268170faf0de7` |
| `snow_shot/tests/screenshot_ocr_recognition_service_tests.cpp` | community-protected | `8429380825d1fb7d0bfea8e7eba1d7de3b6d9347` | `db20f040c4d469473434d9a0193209ff00835176` | `03d2fee15d56018eadd5d9f2fa9472164eface8a` |
| `snow_shot/tests/screenshot_recognition_session_controller_tests.cpp` | overlapping changes | `bd8d109c9d9769df4620585eb11048e22d3b20da` | `6ecdef8a802debd1c5c536fe853a9f832f3aa57f` | `9a5ee5f0418dcc849dd7e4809db702487488d1f7` |
| `snow_shot/tests/settings_catalog_tests.cpp` | community-protected | `f7038278e49bdf7fade32c7f7e1a85dc7e1f76a2` | `7e67ee94fffa462106a04de0c179485071414cd2` | `104430d8f3cd009b11b3a8b4a29de766cab7dbb9` |
| `snow_shot/tests/settings_page_loading_tests.cpp` | overlapping changes | `0278ec39e3375c2b70e362af671e5e6e2f6e12e0` | `21e5ddde8a24c15f03149c0add4a89532e200502` | `c50f942da05812f87e94a7032b34a1519d3c142a` |
| `snow_shot/tests/snowshot_api_client_tests.cpp` | community-protected | `eecf0868cc92f79b8cd1c82935547c7d4fb9a205` | `499f28ae421f9d325114a9fcb8d77f85c1aaaf71` | `ebe2383c4d6d9ba7499b30cded4dcfecb1a1c4d9` |
| `snow_shot/tests/update_helper_fixture.cpp` | overlapping changes | `de1bbf99a5f61729b621e49bf179a50ba28e6c9d` | `9b2eaed6cb420e3139727320141ad8f4882cf307` | `5b1251bbd44c6655122bec83e61d053d9ebc2c03` |

## Intentional deletion policy exclusions

- `docs-macos-build.md`
- `snow_shot/include/snow_shot/app/cloudsettingssyncservice.h`
- `snow_shot/include/snow_shot/storage/cloudsyncconfiguration.h`
- `snow_shot/src/app/cloudsettingssyncservice.cpp`
- `snow_shot/src/presentation/components/accountpagewidget.cpp`
- `snow_shot/src/storage/cloudsyncconfiguration.cpp`
- `snow_shot/src/storage/cloudsyncconfigurationkeys_p.h`
- `snow_shot/tests/account_tests.cpp`
- `snow_shot/tests/cloud_settings_sync_tests.cpp`
- `snow_shot/tests/macos_screenshots.md`

## Imported exact upstream objects

- `ant_design_qt/packages/ant_design_qt/resources/icons.manifest.json`
- `ant_design_qt/packages/ant_design_qt/resources/icons/window-maximize.svg`
- `ant_design_qt/packages/ant_design_qt/resources/icons/window-restore.svg`
- `ant_design_qt/packages/ant_design_qt/src/icons/widget_icons.cpp`
- `ant_design_qt/packages/ant_design_qt/src/icons/widget_icons.h`
- `ant_design_qt/packages/ant_design_qt/src/theme/theme_manager.cpp`
- `ant_design_qt/packages/ant_design_qt/src/widgets/modal.cpp`
- `ant_design_qt/packages/ant_design_qt/src/widgets/window_size_icon.h`
- `ant_design_qt/packages/ant_design_qt/tests/theme_typography_tests.cpp`
- `ant_design_qt/packages/ant_design_qt/tests/tst_modal_window.cpp`
- `ant_design_qt/tests/icon_core_tests.cpp`
- `cmake/SnowShotRuntimeTests.cmake`
- `cmake/SnowShotUpdateUiTests.cmake`
- `scripts/package-snow-shot-mini.ps1`
- `scripts/snow-shot-msix.ps1`
- `scripts/test-snow-shot-mini-release.ps1`
- `scripts/test-snow-shot-msix.ps1`
- `scripts/test-snow-shot-ocr-detection-sizing.py`
- `scripts/test-snow-shot-package-dependencies.ps1`
- `snow-crates/Cargo.lock`
- `snow-crates/crates/snow-audio-recorder/Cargo.toml`
- `snow-crates/crates/snow-audio-recorder/src/packet.rs`
- `snow-crates/crates/snow-audio-recorder/src/platform/macos.rs`
- `snow-crates/crates/snow-audio-recorder/src/platform/windows/wasapi_source.rs`
- `snow-crates/crates/snow-audio-recorder/src/recording.rs`
- `snow-crates/crates/snow-audio-recorder/src/timeline.rs`
- `snow-crates/crates/snow-capture-c/include/snow_capture.h`
- `snow-crates/crates/snow-capture-c/src/desktop_layout.rs`
- `snow-crates/crates/snow-capture-c/src/lib.rs`
- `snow-crates/crates/snow-capture/src/capture_session.rs`
- `snow-crates/crates/snow-capture/src/convert/mod.rs`
- `snow-crates/crates/snow-capture/src/convert/parallel.rs`
- `snow-crates/crates/snow-capture/src/platform/macos.rs`
- `snow-crates/crates/snow-capture/src/platform/windows/gdi.rs`
- `snow-crates/crates/snow-capture/src/platform/windows/surface.rs`
- `snow-crates/crates/snow-macos/src/bench.rs`
- `snow-crates/crates/snow-macos/src/capture.rs`
- `snow-crates/crates/snow-macos/src/content.rs`
- `snow-crates/crates/snow-macos/src/desktop.rs`
- `snow-crates/crates/snow-ocr-process/Cargo.toml`
- `snow-crates/crates/snow-ocr-process/PROTOCOL.md`
- `snow-crates/crates/snow-ocr-process/src/engine.rs`
- `snow-crates/crates/snow-ocr-process/src/main.rs`
- `snow-crates/crates/snow-ocr-process/src/sizing_validation.rs`
- `snow-crates/crates/snow-recording-runtime/src/direct.rs`
- `snow-crates/crates/snow-stitch-images-c/Cargo.toml`
- `snow-crates/crates/snow-stitch-images-c/src/execution.rs`
- `snow-crates/crates/snow-stitch-images-c/src/lib.rs`
- `snow-crates/crates/snow-stitch-images/src/compositor.rs`
- `snow-crates/crates/snow-stitch-images/src/estimator.rs`
- `snow-crates/crates/snow-stitch-images/src/estimator/registration.rs`
- `snow-crates/crates/snow-stitch-images/src/estimator/registration/full_overlap.rs`
- `snow-crates/crates/snow-stitch-images/src/estimator/registration/search.rs`
- `snow-crates/crates/snow-stitch-images/src/estimator/table_tests.rs`
- `snow-crates/crates/snow-stitch-images/src/frame.rs`
- `snow-crates/crates/snow-stitch-images/src/orb.rs`
- `snow-crates/crates/snow-stitch-images/src/orb_avx2.rs`
- `snow-crates/crates/snow-stitch-images/src/orb_neon.rs`
- `snow-crates/crates/snow-stitch-images/src/perf.rs`
- `snow-crates/crates/snow-stitch-images/src/region.rs`
- `snow-crates/crates/snow-stitch-images/src/region_neon.rs`
- `snow-crates/crates/snow-stitch-images/src/sampling.rs`
- `snow-crates/crates/snow-stitch-images/src/sampling_neon.rs`
- `snow-crates/crates/snow-stitch-images/src/stitcher.rs`
- `snow-crates/crates/snow-stitch-images/src/tiled_canvas.rs`
- `snow-crates/crates/snow-stitch-images/src/types.rs`
- `snow-crates/crates/snow-visual-region-detector-c/src/lib.rs`
- `snow-crates/crates/visual-region-detector/Cargo.toml`
- `snow-crates/crates/visual-region-detector/src/backend/mod.rs`
- `snow-crates/crates/visual-region-detector/src/backend/opencv_impl.rs`
- `snow-crates/crates/visual-region-detector/src/backend/pure_rust/align.rs`
- `snow-crates/crates/visual-region-detector/src/backend/pure_rust/canny.rs`
- `snow-crates/crates/visual-region-detector/src/backend/pure_rust/mod.rs`
- `snow-crates/crates/visual-region-detector/src/backend/pure_rust/morph.rs`
- `snow-crates/crates/visual-region-detector/src/color.rs`
- `snow-crates/crates/visual-region-detector/src/grid.rs`
- `snow-crates/crates/visual-region-detector/src/mask.rs`
- `snow-crates/crates/visual-region-detector/src/media.rs`
- `snow-crates/crates/visual-region-detector/src/pipeline.rs`
- `snow-crates/crates/visual-region-detector/src/surfaces.rs`
- `snow-crates/crates/visual-region-detector/tests/contracts.rs`
- `snow_draw_engine_qt/CMakeLists.txt`
- `snow_draw_engine_qt/crates/snow-draw-engine-c/include/snow_draw_engine.h`
- `snow_draw_engine_qt/crates/snow-draw-engine-c/src/abi/speech_bubble_exports.rs`
- `snow_draw_engine_qt/crates/snow-draw-engine-c/src/lib.rs`
- `snow_draw_engine_qt/crates/snow-draw-engine-document/src/speech_bubble.rs`
- `snow_draw_engine_qt/include/snow_draw_engine_qt/snow_canvas_image.h`
- `snow_draw_engine_qt/include/snow_draw_engine_qt/snow_canvas_wheel_input.h`
- `snow_draw_engine_qt/include/snow_draw_engine_qt/snow_canvas_widget.h`
- `snow_draw_engine_qt/src/core/snow_canvas_runtime_session.cpp`
- `snow_draw_engine_qt/src/rendering/snow_canvas_export.cpp`
- `snow_draw_engine_qt/src/rendering/snow_canvas_filter_render.cpp`
- `snow_draw_engine_qt/src/rendering/snow_canvas_image.cpp`
- `snow_draw_engine_qt/src/widget/snow_canvas_widget.cpp`
- `snow_draw_engine_qt/tests/snow_canvas_filter_render_tests.cpp`
- `snow_draw_engine_qt/tests/snow_canvas_filter_safety_tests.cpp`
- `snow_draw_engine_qt/tests/snow_canvas_image_memory_benchmark.cpp`
- `snow_draw_engine_qt/tests/snow_canvas_image_tests.cpp`
- `snow_image/CMakeLists.txt`
- `snow_image/src/codecs/png_codec.cpp`
- `snow_image/src/codecs/png_parallel.cpp`
- `snow_image/src/codecs/png_parallel.h`
- `snow_image/src/codecs/webp_codec.cpp`
- `snow_image/tests/CMakeLists.txt`
- `snow_image/tests/raster_export_tests.cpp`
- `snow_rust_ffi/Cargo.lock`
- `snow_shot/docs/pinned-window-actions.md`
- `snow_shot/docs/recording-follow-mouse.md`
- `snow_shot/docs/recording-hevc.md`
- `snow_shot/docs/recording-trimming.md`
- `snow_shot/i18n/capture/snow_shot_capture_en_US.ts`
- `snow_shot/i18n/capture/snow_shot_capture_ru.ts`
- `snow_shot/i18n/capture/snow_shot_capture_zh_CN.ts`
- `snow_shot/i18n/capture/snow_shot_capture_zh_TW.ts`
- `snow_shot/i18n/export/snow_shot_export_en_US.ts`
- `snow_shot/i18n/export/snow_shot_export_ru.ts`
- `snow_shot/i18n/export/snow_shot_export_zh_CN.ts`
- `snow_shot/i18n/export/snow_shot_export_zh_TW.ts`
- `snow_shot/include/snow_shot/app/mcp/mcpstylepatch.h`
- `snow_shot/include/snow_shot/app/shellimagelaunch.h`
- `snow_shot/include/snow_shot/app/shelllaunchcoordinator.h`
- `snow_shot/include/snow_shot/mouseoperations.h`
- `snow_shot/include/snow_shot/platform/macos/screenshotsessionfocus.h`
- `snow_shot/include/snow_shot/platform/screenshotnative.h`
- `snow_shot/include/snow_shot/platform/windows/packagedcontextmenupreference.h`
- `snow_shot/include/snow_shot/platform/windows/shellbatchfile.h`
- `snow_shot/include/snow_shot/platform/windows/shellcontextmenumode.h`
- `snow_shot/include/snow_shot/platform/windows/shellcontextmenuregistration.h`
- `snow_shot/include/snow_shot/presentation/apppermissionservice.h`
- `snow_shot/include/snow_shot/presentation/capture/screenshotcapturepolicy.h`
- `snow_shot/include/snow_shot/presentation/collagedocument.h`
- `snow_shot/include/snow_shot/presentation/collagewindow.h`
- `snow_shot/include/snow_shot/presentation/collagewindowcoordinator.h`
- `snow_shot/include/snow_shot/presentation/components/actionrow.h`
- `snow_shot/include/snow_shot/presentation/components/globalmouserow.h`
- `snow_shot/include/snow_shot/presentation/components/icons/snowshoticons.h`
- `snow_shot/include/snow_shot/presentation/components/mouseoperationrow.h`
- `snow_shot/include/snow_shot/presentation/components/shortcutkeyrow.h`
- `snow_shot/include/snow_shot/presentation/directcapturecontroller.h`
- `snow_shot/include/snow_shot/presentation/directcaptureworkflow.h`
- `snow_shot/include/snow_shot/presentation/floatingtoolbarcontroller.h`
- `snow_shot/include/snow_shot/presentation/globalactiontraits.h`
- `snow_shot/include/snow_shot/presentation/globalmousegesture.h`
- `snow_shot/include/snow_shot/presentation/globalmousemanager.h`
- `snow_shot/include/snow_shot/presentation/globalmousetypes.h`
- `snow_shot/include/snow_shot/presentation/globalshortcuttypes.h`
- `snow_shot/include/snow_shot/presentation/imagecompressionwindow.h`
- `snow_shot/include/snow_shot/presentation/imageeditingwindow.h`
- `snow_shot/include/snow_shot/presentation/imagesplitdocument.h`
- `snow_shot/include/snow_shot/presentation/imagesplitwindow.h`
- `snow_shot/include/snow_shot/presentation/mouseoperationinput.h`
- `snow_shot/include/snow_shot/presentation/pinnedwindowselectioncontroller.h`
- `snow_shot/include/snow_shot/presentation/screenshotairdropservice.h`
- `snow_shot/include/snow_shot/presentation/screenshotcanvasrenderer.h`
- `snow_shot/include/snow_shot/presentation/screenshotcanvastoolmapping.h`
- `snow_shot/include/snow_shot/presentation/screenshotcanvastools.h`
- `snow_shot/include/snow_shot/presentation/screenshotcanvastools.inc`
- `snow_shot/include/snow_shot/presentation/screenshotcapturecommands.h`
- `snow_shot/include/snow_shot/presentation/screenshotcapturestate.h`
- `snow_shot/include/snow_shot/presentation/screenshotcaptureworkflow.h`
- `snow_shot/include/snow_shot/presentation/screenshotclipboardcontent.h`
- `snow_shot/include/snow_shot/presentation/screenshotclipboardservice.h`
- `snow_shot/include/snow_shot/presentation/screenshotdisplayconfigurationobserver.h`
- `snow_shot/include/snow_shot/presentation/screenshotexportcache.h`
- `snow_shot/include/snow_shot/presentation/screenshotexportcoordinator.h`
- `snow_shot/include/snow_shot/presentation/screenshotexportservice.h`
- `snow_shot/include/snow_shot/presentation/screenshotexporttasks.h`
- `snow_shot/include/snow_shot/presentation/screenshotglobalmousedrag.h`
- `snow_shot/include/snow_shot/presentation/screenshotguidelinerendering.h`
- `snow_shot/include/snow_shot/presentation/screenshotimagerowsource.h`
- `snow_shot/include/snow_shot/presentation/screenshotocrpresentation.h`
- `snow_shot/include/snow_shot/presentation/screenshotocrtextlayout.h`
- `snow_shot/include/snow_shot/presentation/screenshotoverlaycanvaspresenter.h`
- `snow_shot/include/snow_shot/presentation/screenshotoverlaycoordinator.h`
- `snow_shot/include/snow_shot/presentation/screenshotoverlayeventsink.h`
- `snow_shot/include/snow_shot/presentation/screenshotoverlayinputhandler.h`
- `snow_shot/include/snow_shot/presentation/screenshotoverlayinteractionadapter.h`
- `snow_shot/include/snow_shot/presentation/screenshotoverlayshortcutcontroller.h`
- `snow_shot/include/snow_shot/presentation/screenshotoverlaywindow.h`
- `snow_shot/include/snow_shot/presentation/screenshotpinnededitcontroller.h`
- `snow_shot/include/snow_shot/presentation/screenshotpinnedwindow.h`
- `snow_shot/include/snow_shot/presentation/screenshotpresentationservices.h`
- `snow_shot/include/snow_shot/presentation/screenshotprintservice.h`
- `snow_shot/include/snow_shot/presentation/screenshotqrcontroller.h`
- `snow_shot/include/snow_shot/presentation/screenshotqrrecognitionservice.h`
- `snow_shot/include/snow_shot/presentation/screenshotrecognitioncopy.h`
- `snow_shot/include/snow_shot/presentation/screenshotrecognitionfileexport.h`
- `snow_shot/include/snow_shot/presentation/screenshotrecognitionimage.h`
- `snow_shot/include/snow_shot/presentation/screenshotselectionexportuiservices.h`
- `snow_shot/include/snow_shot/presentation/screenshottableeditor.h`
- `snow_shot/include/snow_shot/presentation/screenshottoolbarcommands.h`
- `snow_shot/include/snow_shot/presentation/screenshottoolcommandworkflow.h`
- `snow_shot/include/snow_shot/presentation/screenshottoolpalette.h`
- `snow_shot/include/snow_shot/presentation/screenshottypes.h`
- `snow_shot/include/snow_shot/presentation/screenshotuipreferences.h`
- `snow_shot/include/snow_shot/presentation/screenshotwheelinput.h`
- `snow_shot/include/snow_shot/presentation/settings/settingsregistry.h`
- `snow_shot/include/snow_shot/presentation/websiteurl.h`
- `snow_shot/include/snow_shot/presentation/windowshortcutmanager.h`
- `snow_shot/include/snow_shot/runtime/memoryoptimizationcontroller.h`
- `snow_shot/include/snow_shot/runtime/runtimeactivitytracker.h`
- `snow_shot/include/snow_shot/shortcuts/localshortcutcontext.h`
- `snow_shot/include/snow_shot/shortcuts/pinnedshortcutcontext.h`
- `snow_shot/include/snow_shot/shortcuts/screenshotshortcutcontext.h`
- `snow_shot/include/snow_shot/storage/configurationschema.h`
- `snow_shot/include/snow_shot/storage/exporttasks.h`
- `snow_shot/include/snow_shot/storage/pinnedwindowtypes.h`
- `snow_shot/include/snow_shot/update/startupupdate.h`
- `snow_shot/packaging/msix/AppxManifest.xml.in`
- `snow_shot/packaging/msix/Strings/en-US/Resources.resw`
- `snow_shot/packaging/msix/Strings/ru-RU/Resources.resw`
- `snow_shot/packaging/msix/Strings/zh-CN/Resources.resw`
- `snow_shot/packaging/msix/Strings/zh-TW/Resources.resw`
- `snow_shot/rust/snow-shot-mcp/src/domain_schemas.rs`
- `snow_shot/rust/snow-shot-updater/src/coordination.rs`
- `snow_shot/rust/snow-shot-updater/src/platform/windows_shell.rs`
- `snow_shot/rust/snow-shot-updater/src/protocol.rs`
- `snow_shot/rust/snow-shot-updater/tests/windows_shell.rs`
- `snow_shot/scripts/run-pin-to-screen-perf-macos.py`
- `snow_shot/scripts/test-run-pin-to-screen-perf-macos.py`
- `snow_shot/src/app/mcp/mcpdocumentservice.cpp`
- `snow_shot/src/app/mcp/mcpmediaservice.cpp`
- `snow_shot/src/app/shelllaunchcoordinator.cpp`
- `snow_shot/src/image/snowimageqtcodec.cpp`
- `snow_shot/src/platform/macos/apppermissionbackend.mm`
- `snow_shot/src/platform/macos/displaywakeobserver.h`
- `snow_shot/src/platform/macos/displaywakeobserver.mm`
- `snow_shot/src/platform/macos/globalmousebackend.mm`
- `snow_shot/src/platform/macos/globalmousebackend_p.h`
- `snow_shot/src/platform/macos/globalmouseinput.cpp`
- `snow_shot/src/platform/macos/imageclipboard.h`
- `snow_shot/src/platform/macos/imageclipboard.mm`
- `snow_shot/src/platform/macos/memoryoptimization.cpp`
- `snow_shot/src/platform/macos/memoryoptimization.mm`
- `snow_shot/src/platform/macos/nativeairdrop.mm`
- `snow_shot/src/platform/macos/nativeprint.mm`
- `snow_shot/src/platform/macos/screenshotnative.mm`
- `snow_shot/src/platform/macos/screenshotsessionfocus.mm`
- `snow_shot/src/platform/macos/windowshortcutinput.mm`
- `snow_shot/src/platform/macos/windowshortcutinput_p.h`
- `snow_shot/src/platform/windows/displayrecoverywindow.cpp`
- `snow_shot/src/platform/windows/displayrecoverywindow.h`
- `snow_shot/src/platform/windows/explorercommand/CMakeLists.txt`
- `snow_shot/src/platform/windows/explorercommand/explorercommand.cpp`
- `snow_shot/src/platform/windows/explorercommand/explorercommand.def`
- `snow_shot/src/platform/windows/explorercommand/explorercommand.h`
- `snow_shot/src/platform/windows/explorercommand/explorercommand.rc.in`
- `snow_shot/src/platform/windows/globalmousebackend.cpp`
- `snow_shot/src/platform/windows/globalshortcutbackend.cpp`
- `snow_shot/src/platform/windows/nativeprint.cpp`
- `snow_shot/src/platform/windows/pinnedwindowevents.cpp`
- `snow_shot/src/platform/windows/pinnedwindownative.cpp`
- `snow_shot/src/platform/windows/shellcontextmenuregistration.cpp`
- `snow_shot/src/platform/windows/shortcutvirtualkey.h`
- `snow_shot/src/presentation/capture/directcapturecontroller.cpp`
- `snow_shot/src/presentation/capture/directcapturenative.cpp`
- `snow_shot/src/presentation/capture/screenshotcapturepolicy.cpp`
- `snow_shot/src/presentation/capture/screenshotcapturepolicy_p.h`
- `snow_shot/src/presentation/capture/screenshotcaptureworker.cpp`
- `snow_shot/src/presentation/capture/screenshotcaptureworker.h`
- `snow_shot/src/presentation/capture/screenshotcaptureworkflow.cpp`
- `snow_shot/src/presentation/capture/screenshotglobalmousedrag.cpp`
- `snow_shot/src/presentation/capture/screenshotscrollingpipeline.cpp`
- `snow_shot/src/presentation/collage/collagedocument.cpp`
- `snow_shot/src/presentation/collage/collageeditorwidget.cpp`
- `snow_shot/src/presentation/collage/collagewindow.cpp`
- `snow_shot/src/presentation/collage/collagewindowcoordinator.cpp`
- `snow_shot/src/presentation/components/actionrow.cpp`
- `snow_shot/src/presentation/components/globalmouserow.cpp`
- `snow_shot/src/presentation/components/icons/resources/cancel-crop.svg`
- `snow_shot/src/presentation/components/icons/snowshoticons.cpp`
- `snow_shot/src/presentation/components/mouseoperationrow.cpp`
- `snow_shot/src/presentation/components/shortcutkeyrow.cpp`
- `snow_shot/src/presentation/imageediting/imageeditingdocument.cpp`
- `snow_shot/src/presentation/imageediting/imageeditingexportcontroller.cpp`
- `snow_shot/src/presentation/imageediting/imageeditingexportcontroller.h`
- `snow_shot/src/presentation/imageediting/imageeditingwindow.cpp`
- `snow_shot/src/presentation/imagesplit/imagesplitwindow.cpp`
- `snow_shot/src/presentation/ocr/screenshotocrlinegeometry.cpp`
- `snow_shot/src/presentation/ocr/screenshotocrlinegeometry.h`
- `snow_shot/src/presentation/ocr/screenshotocrprotocol.h`
- `snow_shot/src/presentation/ocr/screenshotocrtextlayout.cpp`
- `snow_shot/src/presentation/ocr/screenshotocrtransport.h`
- `snow_shot/src/presentation/ocr/screenshotqrcontroller.cpp`
- `snow_shot/src/presentation/ocr/screenshotqrrecognitionservice.cpp`
- `snow_shot/src/presentation/ocr/screenshotrecognitionimage.cpp`
- `snow_shot/src/presentation/ocr/screenshottableeditor.cpp`
- `snow_shot/src/presentation/overlay/screenshotcanvasrenderer.cpp`
- `snow_shot/src/presentation/overlay/screenshotoverlaycanvaspresenter.cpp`
- `snow_shot/src/presentation/overlay/screenshotoverlaycoordinator.cpp`
- `snow_shot/src/presentation/overlay/screenshotoverlayinputhandler.cpp`
- `snow_shot/src/presentation/overlay/screenshotoverlayinteractionadapter.cpp`
- `snow_shot/src/presentation/overlay/screenshotoverlayshortcutcontroller.cpp`
- `snow_shot/src/presentation/overlay/screenshotoverlaywindow.cpp`
- `snow_shot/src/presentation/pinned/livewindownative_p.h`
- `snow_shot/src/presentation/pinned/livewindowsource.cpp`
- `snow_shot/src/presentation/pinned/pinnedwindowselectioncontroller.cpp`
- `snow_shot/src/presentation/pinned/pinnedwindowselectioninteraction.cpp`
- `snow_shot/src/presentation/pinned/screenshotpinnedbatchsave.cpp`
- `snow_shot/src/presentation/pinned/screenshotpinnedcrop.cpp`
- `snow_shot/src/presentation/pinned/screenshotpinnededitcontroller.cpp`
- `snow_shot/src/presentation/pinned/screenshotpinnedhidetotopcontroller.cpp`
- `snow_shot/src/presentation/pinned/screenshotpinnedhidetotopcontroller.h`
- `snow_shot/src/presentation/pinned/screenshotpinnedinteraction.cpp`
- `snow_shot/src/presentation/pinned/screenshotpinnedlive.cpp`
- `snow_shot/src/presentation/pinned/screenshotpinnedwindow.cpp`
- `snow_shot/src/presentation/pinned/screenshotpintoperfinstrumentation.cpp`
- `snow_shot/src/presentation/recording/recordingrenderjob.cpp`
- `snow_shot/src/presentation/recording/recordingrenderjob.h`
- `snow_shot/src/presentation/recording/recordingtrimsession.cpp`
- `snow_shot/src/presentation/recording/recordingtrimsession.h`
- `snow_shot/src/presentation/selection/screenshotselectionexportuiservices.cpp`
- `snow_shot/src/presentation/services/apppermissionservice.cpp`
- `snow_shot/src/presentation/services/globalmousegesture.cpp`
- `snow_shot/src/presentation/services/imagecompressionwindow.cpp`
- `snow_shot/src/presentation/services/imageexporteditor.cpp`
- `snow_shot/src/presentation/services/imageexporteditor_p.h`
- `snow_shot/src/presentation/services/nativeairdropbackend.h`
- `snow_shot/src/presentation/services/nativeairdropunsupported.cpp`
- `snow_shot/src/presentation/services/nativeprintbackend.h`
- `snow_shot/src/presentation/services/nativeprintunsupported.cpp`
- `snow_shot/src/presentation/services/screenshotairdropservice.cpp`
- `snow_shot/src/presentation/services/screenshotclipboardcontent.cpp`
- `snow_shot/src/presentation/services/screenshotclipboardservice.cpp`
- `snow_shot/src/presentation/services/screenshotdisplayconfigurationobserver.cpp`
- `snow_shot/src/presentation/services/screenshotexportartifact.cpp`
- `snow_shot/src/presentation/services/screenshotexportcoordinator.cpp`
- `snow_shot/src/presentation/services/screenshotexportservice.cpp`
- `snow_shot/src/presentation/services/screenshotexporttasks.cpp`
- `snow_shot/src/presentation/services/screenshotmcpcommands_p.h`
- `snow_shot/src/presentation/services/screenshotpresentationservices.cpp`
- `snow_shot/src/presentation/services/screenshotprintservice.cpp`
- `snow_shot/src/presentation/services/screenshotrecognitionfileexport.cpp`
- `snow_shot/src/presentation/services/screenshotsaveexportpipeline.cpp`
- `snow_shot/src/presentation/services/websiteurl.cpp`
- `snow_shot/src/presentation/services/windowshortcutmanager.cpp`
- `snow_shot/src/presentation/settings/settingsregistry.cpp`
- `snow_shot/src/presentation/toolbar/floatingtoolbarcontroller.cpp`
- `snow_shot/src/presentation/toolbar/screenshottoolbarwindow.cpp`
- `snow_shot/src/presentation/tools/screenshottoolcommandworkflow.cpp`
- `snow_shot/src/runtime/memoryoptimizationcontroller.cpp`
- `snow_shot/src/runtime/runtimeactivitytracker.cpp`
- `snow_shot/src/storage/exporttasks.cpp`
- `snow_shot/src/storage/mouseoperations.cpp`
- `snow_shot/src/storage/pinnedwindowrepository.cpp`
- `snow_shot/tests/annotation_shortcut_settings_tests.cpp`
- `snow_shot/tests/app_permissions_tests.cpp`
- `snow_shot/tests/close_release_native_test_support.h`
- `snow_shot/tests/collage_document_tests.cpp`
- `snow_shot/tests/collage_editor_tests.cpp`
- `snow_shot/tests/collage_pdf_window_tests.cpp`
- `snow_shot/tests/collage_window_tests.cpp`
- `snow_shot/tests/direct_capture_native_tests.cpp`
- `snow_shot/tests/direct_capture_workflow_tests.cpp`
- `snow_shot/tests/display_recovery_tests.cpp`
- `snow_shot/tests/explorer_command_tests.cpp`
- `snow_shot/tests/fixtures/configuration-schema-windows-full.json`
- `snow_shot/tests/fixtures/configuration-schema-windows-mini.json`
- `snow_shot/tests/floating_toolbar_tests.cpp`
- `snow_shot/tests/generated_icon_pack_tests.cpp`
- `snow_shot/tests/global_mouse_native_input_tests.cpp`
- `snow_shot/tests/global_mouse_page_tests.cpp`
- `snow_shot/tests/global_mouse_tests.cpp`
- `snow_shot/tests/imagecompression_tests.cpp`
- `snow_shot/tests/imageediting_import_tests.cpp`
- `snow_shot/tests/imageediting_save_tests.cpp`
- `snow_shot/tests/imageediting_tests.cpp`
- `snow_shot/tests/imageediting_translation_tests.cpp`
- `snow_shot/tests/imagesplit_pdf_handoff_tests.cpp`
- `snow_shot/tests/imagesplit_window_tests.cpp`
- `snow_shot/tests/language_manager_tests.cpp`
- `snow_shot/tests/live_window_source_tests.cpp`
- `snow_shot/tests/local_shortcut_reset_tests.cpp`
- `snow_shot/tests/macos_clipboard_roundtrip_tests.mm`
- `snow_shot/tests/macos_display_wake_tests.mm`
- `snow_shot/tests/macos_memory_optimization_tests.cpp`
- `snow_shot/tests/macos_screenshot_focus_tests.cpp`
- `snow_shot/tests/macos_shell_policy_tests.cpp`
- `snow_shot/tests/macos_window_shortcut_input_tests.mm`
- `snow_shot/tests/mcp_fixture_tests.py`
- `snow_shot/tests/memory_optimization_controller_tests.cpp`
- `snow_shot/tests/native_print_smoke.cpp`
- `snow_shot/tests/original_image_translation_tests.cpp`
- `snow_shot/tests/pin_to_screen_shortcut_settings_tests.cpp`
- `snow_shot/tests/pinned_action_performance_benchmark.h`
- `snow_shot/tests/pinned_action_transition_tests.h`
- `snow_shot/tests/pinned_clipboard_paste_tests.h`
- `snow_shot/tests/pinned_color_swatch_window_tests.h`
- `snow_shot/tests/pinned_copy_ordering_tests.h`
- `snow_shot/tests/pinned_export_snapshot_tests.h`
- `snow_shot/tests/pinned_mouse_operation_tests.h`
- `snow_shot/tests/pinned_quick_text_annotation_tests.h`
- `snow_shot/tests/pinned_shake_window_tests.h`
- `snow_shot/tests/pinned_window_repository_tests.cpp`
- `snow_shot/tests/recapture_cursor_native_e2e_test.cpp`
- `snow_shot/tests/recognition_image_export_tests.md`
- `snow_shot/tests/saturated_export_queue.h`
- `snow_shot/tests/screen_recording_shortcut_tests.cpp`
- `snow_shot/tests/screenshot_airdrop_service_tests.cpp`
- `snow_shot/tests/screenshot_canvas_renderer_tests.cpp`
- `snow_shot/tests/screenshot_capture_policy_tests.cpp`
- `snow_shot/tests/screenshot_capture_startup_performance_benchmark_macos.mm`
- `snow_shot/tests/screenshot_capture_worker_tests.cpp`
- `snow_shot/tests/screenshot_capture_workflow_tests.cpp`
- `snow_shot/tests/screenshot_clipboard_content_tests.cpp`
- `snow_shot/tests/screenshot_clipboard_lifecycle_tests.cpp`
- `snow_shot/tests/screenshot_clipboard_placement_tests.cpp`
- `snow_shot/tests/screenshot_clipboard_roundtrip_tests.cpp`
- `snow_shot/tests/screenshot_clipboard_test_output.h`
- `snow_shot/tests/screenshot_color_picker_lifecycle_tests.cpp`
- `snow_shot/tests/screenshot_controller_clipboard_tests.cpp`
- `snow_shot/tests/screenshot_controller_lifetime_tests.cpp`
- `snow_shot/tests/screenshot_controller_test_access.h`
- `snow_shot/tests/screenshot_export_artifact_tests.cpp`
- `snow_shot/tests/screenshot_export_coordinator_tests.cpp`
- `snow_shot/tests/screenshot_export_dialog_workflow_tests.cpp`
- `snow_shot/tests/screenshot_floating_toolbar_drag_tests.cpp`
- `snow_shot/tests/screenshot_history_tests.cpp`
- `snow_shot/tests/screenshot_live_pinned_window_native_tests.h`
- `snow_shot/tests/screenshot_live_pinned_window_tests.h`
- `snow_shot/tests/screenshot_ocr_layout_tests.cpp`
- `snow_shot/tests/screenshot_ocr_process_lifecycle_tests.cpp`
- `snow_shot/tests/screenshot_ocr_text_layout_tests.cpp`
- `snow_shot/tests/screenshot_overlay_input_tests.cpp`
- `snow_shot/tests/screenshot_pin_to_screen_performance_macos.mm`
- `snow_shot/tests/screenshot_pinned_crop_tests.h`
- `snow_shot/tests/screenshot_pinned_hide_to_top_tests.cpp`
- `snow_shot/tests/screenshot_pinned_menu_lifetime_tests.h`
- `snow_shot/tests/screenshot_pinned_window_tests.cpp`
- `snow_shot/tests/screenshot_print_service_tests.cpp`
- `snow_shot/tests/screenshot_qr_controller_tests.cpp`
- `snow_shot/tests/screenshot_qr_recognition_service_tests.cpp`
- `snow_shot/tests/screenshot_recognition_image_tests.cpp`
- `snow_shot/tests/screenshot_recognition_window_tests.cpp`
- `snow_shot/tests/screenshot_selection_presentation_fixture.h`
- `snow_shot/tests/screenshot_selection_presentation_tests.cpp`
- `snow_shot/tests/screenshot_tool_command_workflow_tests.cpp`
- `snow_shot/tests/screenshot_tool_palette_tests.cpp`
- `snow_shot/tests/screenshot_toolbar_display_benchmark.cpp`
- `snow_shot/tests/screenshot_toolbar_fill_color_hover_tests.cpp`
- `snow_shot/tests/screenshot_toolbar_remembered_tool_tests.cpp`
- `snow_shot/tests/screenshot_toolbar_render_benchmark.cpp`
- `snow_shot/tests/screenshot_translation_tool_tests.cpp`
- `snow_shot/tests/scrolling_capture_settings_tests.cpp`
- `snow_shot/tests/scrolling_corpus_performance_benchmark.cpp`
- `snow_shot/tests/scrolling_image_replay_tests.cpp`
- `snow_shot/tests/settings_runtime_session_tests.cpp`
- `snow_shot/tests/shell_context_menu_registration_tests.cpp`
- `snow_shot/tests/shell_context_menu_settings_tests.cpp`
- `snow_shot/tests/shell_image_launch_tests.cpp`
- `snow_shot/tests/shell_launch_coordinator_tests.cpp`
- `snow_shot/tests/store_platform_tests.cpp`
- `snow_shot/tests/translation_settings_tests.cpp`
- `snow_shot/tests/window_element_api_tests.cpp`
- `snow_shot/tests/window_shortcut_manager_tests.cpp`

## Applied conflict resolutions — build graph

- `CMakeLists.txt`: retained 6 Community edit groups, applied 1 upstream edit groups; excluded upstream version number. Both source diffs were reconstructed exactly before combination. Native configure/build and semantic validation pending.
- `cmake/SnowShotMini.cmake`: retained 2 Community edit groups, applied 1 upstream edit groups; excluded none. Both source diffs were reconstructed exactly before combination. Native configure/build and semantic validation pending.
- `snow_shot/CMakeLists.txt`: retained 37 Community edit groups, applied 48 upstream edit groups; excluded deleted account/cloud test targets, deleted account/cloud test targets, deleted account/cloud test targets. Both source diffs were reconstructed exactly before combination. Native configure/build and semantic validation pending.

## Applied conflict resolutions — history localization

- `snow_shot/i18n/history/snow_shot_history_en_US.ts`: {"added":2,"removed":3,"updated":0,"community":3}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/history/snow_shot_history_ru.ts`: {"added":2,"removed":3,"updated":12,"community":3}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/history/snow_shot_history_zh_CN.ts`: {"added":2,"removed":3,"updated":0,"community":3}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/history/snow_shot_history_zh_TW.ts`: {"added":2,"removed":3,"updated":0,"community":3}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.

## Applied conflict resolutions — recognition localization

- `snow_shot/i18n/recognition/snow_shot_recognition_en_US.ts`: {"added":1,"removed":0,"updated":0,"community":29}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/recognition/snow_shot_recognition_ru.ts`: {"added":1,"removed":0,"updated":0,"community":29}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/recognition/snow_shot_recognition_zh_CN.ts`: {"added":1,"removed":0,"updated":0,"community":29}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/recognition/snow_shot_recognition_zh_TW.ts`: {"added":1,"removed":0,"updated":0,"community":29}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.

## Applied conflict resolutions — transactional updater

- `snow_shot/rust/snow-shot-updater/src/platform/windows_impl.rs`: retained 9 Community edit groups and applied 2 disjoint upstream groups. Preserve Community registration and transaction guards; adopt Explorer DLL retirement/unique reboot cleanup and autostart relaunch fixture. Native Windows test pending.
- `snow_shot/rust/snow-shot-updater/src/transaction.rs`: retained 4 Community edit groups and applied 5 disjoint upstream groups. Preserve Community registration and transaction guards; adopt Explorer DLL retirement/unique reboot cleanup and autostart relaunch fixture. Native Windows test pending.
- `snow_shot/src/update/startupupdate.cpp`: retained 0 Community edit groups and applied 3 disjoint upstream groups. Preserve Community registration and transaction guards; adopt Explorer DLL retirement/unique reboot cleanup and autostart relaunch fixture. Native Windows test pending.
- `snow_shot/tests/update_helper_fixture.cpp`: retained 3 Community edit groups and applied 1 disjoint upstream groups. Preserve Community registration and transaction guards; adopt Explorer DLL retirement/unique reboot cleanup and autostart relaunch fixture. Native Windows test pending.

## Applied conflict resolutions — OCR controllers and session headers

- `snow_shot/include/snow_shot/presentation/screenshotocrcontroller.h`: retained 5 Community edit groups and applied 5 disjoint upstream groups; independent PaddleOCR jobs and publication guards remain. Caller adaptation and native testing pending.
- `snow_shot/include/snow_shot/presentation/screenshotrecognitionsessioncontroller.h`: retained 2 Community edit groups and applied 15 disjoint upstream groups; independent PaddleOCR jobs and publication guards remain. Caller adaptation and native testing pending.
- `snow_shot/src/presentation/ocr/screenshotocrcontroller.cpp`: retained 8 Community edit groups and applied 13 disjoint upstream groups; independent PaddleOCR jobs and publication guards remain. Caller adaptation and native testing pending.
- `snow_shot/src/presentation/ocr/screenshotrecognitionsessioncontroller.cpp`: retained 10 Community edit groups and applied 30 disjoint upstream groups; independent PaddleOCR jobs and publication guards remain. Caller adaptation and native testing pending.

## Applied conflict resolutions — screenshot and recognition windows

- `snow_shot/include/snow_shot/presentation/screenshotcontroller.h`: retained 2 Community edit groups and applied 8 disjoint upstream groups. Preserve asynchronous history OCR queue, Markdown signal and conversion source edits. Runtime and API consistency validation pending.
- `snow_shot/src/presentation/services/screenshotcontroller.cpp`: retained 20 Community edit groups and applied 179 disjoint upstream groups. Preserve asynchronous history OCR queue, Markdown signal and conversion source edits. Runtime and API consistency validation pending.
- `snow_shot/include/snow_shot/presentation/screenshotrecognitionwindow.h`: retained 1 Community edit groups and applied 9 disjoint upstream groups. Preserve asynchronous history OCR queue, Markdown signal and conversion source edits. Runtime and API consistency validation pending.
- `snow_shot/src/presentation/ocr/screenshotrecognitionwindow.cpp`: retained 1 Community edit groups and applied 34 disjoint upstream groups. Preserve asynchronous history OCR queue, Markdown signal and conversion source edits. Runtime and API consistency validation pending.

## Applied conflict resolutions — settings localization and manifest

- `snow_shot/i18n/settings/snow_shot_settings_en_US.ts`: {"added":113,"removed":32,"updated":0,"community":28}; divergent keys retained for review: none; excluded changed keys in removed account/cloud contexts: 1. Locale key parity verified.
- `snow_shot/i18n/settings/snow_shot_settings_ru.ts`: {"added":113,"removed":32,"updated":0,"community":28}; divergent keys retained for review: none; excluded changed keys in removed account/cloud contexts: 1. Locale key parity verified.
- `snow_shot/i18n/settings/snow_shot_settings_zh_CN.ts`: {"added":113,"removed":32,"updated":0,"community":28}; divergent keys retained for review: none; excluded changed keys in removed account/cloud contexts: 1. Locale key parity verified.
- `snow_shot/i18n/settings/snow_shot_settings_zh_TW.ts`: {"added":113,"removed":32,"updated":0,"community":28}; divergent keys retained for review: none; excluded changed keys in removed account/cloud contexts: 1. Locale key parity verified.
- `snow_shot/i18n/modules.json`: added MouseOperationRow, MouseOperations; preserved CloudOcrSettingsWidget, BaiduPaddleOcrClient and intentional account/cloud deletions. Native translation check pending.

## Applied conflict resolutions — core localization

- `snow_shot/i18n/core/snow_shot_core_en_US.ts`: {"added":0,"removed":1,"updated":0,"community":12}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/core/snow_shot_core_ru.ts`: {"added":0,"removed":1,"updated":0,"community":12}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/core/snow_shot_core_zh_CN.ts`: {"added":0,"removed":1,"updated":0,"community":12}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.
- `snow_shot/i18n/core/snow_shot_core_zh_TW.ts`: {"added":0,"removed":1,"updated":0,"community":12}; divergent keys retained for review: none. Locale key parity verified. Source/runtime check pending.

## Applied conflict resolutions — configuration archive and store

- `snow_shot/include/snow_shot/storage/configurationarchive.h`: retained 3 Community groups, applied 1 upstream groups; adapted/excluded none. Preserve Community credential redaction and account-free storage. Rebuild mouse bindings under the existing mutex after set, snapshot and load. Schema and Windows regression tests pending.
- `snow_shot/src/storage/configurationarchive.cpp`: retained 2 Community groups, applied 3 upstream groups; adapted/excluded official import journal-only hunk; official import journal-only hunk. Preserve Community credential redaction and account-free storage. Rebuild mouse bindings under the existing mutex after set, snapshot and load. Schema and Windows regression tests pending.
- `snow_shot/include/snow_shot/storage/configurationstore.h`: retained 14 Community groups, applied 3 upstream groups; adapted/excluded none. Preserve Community credential redaction and account-free storage. Rebuild mouse bindings under the existing mutex after set, snapshot and load. Schema and Windows regression tests pending.
- `snow_shot/src/storage/configurationstore.cpp`: retained 44 Community groups, applied 18 upstream groups; adapted/excluded official account journal migration; official account journal migration; mouse cache relocation; mouse cache relocation. Preserve Community credential redaction and account-free storage. Rebuild mouse bindings under the existing mutex after set, snapshot and load. Schema and Windows regression tests pending.

## Applied conflict resolutions — schema and local settings adapters

- `snow_shot/src/storage/configurationschema.cpp`: retained 3 Community groups, applied 23 disjoint groups, adapted 0 overlapping groups. Preserve manual update defaults and opt-in startup; restore pure shared shortcut validation without account write-origin API. Add grid-color assertion outside deleted cloud-sync tests. Native fixtures pending.
- `snow_shot/include/snow_shot/storage/settingsadapters.h`: retained 3 Community groups, applied 10 disjoint groups, adapted 0 overlapping groups. Preserve manual update defaults and opt-in startup; restore pure shared shortcut validation without account write-origin API. Add grid-color assertion outside deleted cloud-sync tests. Native fixtures pending.
- `snow_shot/src/storage/settingsadapters.cpp`: retained 8 Community groups, applied 26 disjoint groups, adapted 8 overlapping groups. Preserve manual update defaults and opt-in startup; restore pure shared shortcut validation without account write-origin API. Add grid-color assertion outside deleted cloud-sync tests. Native fixtures pending.
- `snow_shot/tests/configuration_schema_tests.cpp`: retained 15 Community groups, applied 1 disjoint groups, adapted 3 overlapping groups. Preserve manual update defaults and opt-in startup; restore pure shared shortcut validation without account write-origin API. Add grid-color assertion outside deleted cloud-sync tests. Native fixtures pending.

## Applied conflict resolutions — settings interfaces and runtime session

- `snow_shot/include/snow_shot/presentation/settings/settingsbackend.h`: retained 3 Community groups, applied 5 upstream groups. Context-menu apply accepts a local map, not upstream account write-origin/import journal. Backend implementation and native tests pending.
- `snow_shot/include/snow_shot/presentation/settings/settingscatalog.h`: retained 3 Community groups, applied 7 upstream groups. Context-menu apply accepts a local map, not upstream account write-origin/import journal. Backend implementation and native tests pending.
- `snow_shot/include/snow_shot/presentation/settings/settingsruntimesession.h`: retained 7 Community groups, applied 2 upstream groups. Context-menu apply accepts a local map, not upstream account write-origin/import journal. Backend implementation and native tests pending.
- `snow_shot/src/presentation/settings/settingsruntimesession.cpp`: retained 6 Community groups, applied 4 upstream groups. Context-menu apply accepts a local map, not upstream account write-origin/import journal. Backend implementation and native tests pending.

## Applied conflict resolutions — settings backend, catalog, page and tests

- `snow_shot/src/presentation/settings/settingsbackend.cpp`: retained 50 Community groups, applied 43 upstream groups, adapted base lines 2575, 2620, 2802, 3233. Context menu writes use local snapshot/rollback; reset new actions through real runtime APIs. Account-free page counts and ordering include upstream mouse operations. Native tests pending.
- `snow_shot/src/presentation/settings/settingscatalog.cpp`: retained 13 Community groups, applied 39 upstream groups, adapted base lines none. Context menu writes use local snapshot/rollback; reset new actions through real runtime APIs. Account-free page counts and ordering include upstream mouse operations. Native tests pending.
- `snow_shot/src/presentation/components/settingspagewidget.cpp`: retained 3 Community groups, applied 5 upstream groups, adapted base lines none. Context menu writes use local snapshot/rollback; reset new actions through real runtime APIs. Account-free page counts and ordering include upstream mouse operations. Native tests pending.
- `snow_shot/tests/settings_catalog_tests.cpp`: retained 20 Community groups, applied 59 upstream groups, adapted base lines 603, 605, 1695, 1699, 1708. Context menu writes use local snapshot/rollback; reset new actions through real runtime APIs. Account-free page counts and ordering include upstream mouse operations. Native tests pending.

## Applied conflict resolutions — recording, input and tool palette

- `snow_shot/src/presentation/recording/screenrecordingcontroller.cpp`: retained 10 Community groups, applied 26 disjoint upstream groups. Preserve local edition constraints and runtime input handling; native tests pending.
- `snow_shot/src/presentation/services/globalmousemanager.cpp`: retained 1 Community groups, applied 19 disjoint upstream groups. Preserve local edition constraints and runtime input handling; native tests pending.
- `snow_shot/src/presentation/services/globalshortcutmanager.cpp`: retained 5 Community groups, applied 4 disjoint upstream groups. Preserve local edition constraints and runtime input handling; native tests pending.
- `snow_shot/src/presentation/tools/screenshottoolpalette.cpp`: retained 8 Community groups, applied 43 disjoint upstream groups. Preserve local edition constraints and runtime input handling; native tests pending.

## Applied conflict resolutions — conversion and OCR service

- `snow_shot/include/snow_shot/presentation/screenshotimageconversionview.h`: retained 3 Community groups, applied 4 disjoint upstream groups; preserve independent OCR engine configuration, source edits and regression tests. Native tests pending.
- `snow_shot/src/presentation/ocr/screenshotimageconversionview.cpp`: retained 10 Community groups, applied 4 disjoint upstream groups; preserve independent OCR engine configuration, source edits and regression tests. Native tests pending.
- `snow_shot/src/presentation/ocr/screenshotocrrecognitionservice.cpp`: retained 33 Community groups, applied 7 disjoint upstream groups; preserve independent OCR engine configuration, source edits and regression tests. Native tests pending.
- `snow_shot/tests/screenshot_recognition_session_controller_tests.cpp`: retained 11 Community groups, applied 24 disjoint upstream groups; preserve independent OCR engine configuration, source edits and regression tests. Native tests pending.

## Applied conflict resolutions — pinned OCR runtime and Inno staging

- `scripts/package-snow-shot.ps1`: retained 16 Community groups and applied 4 upstream groups; adapted none.
- `snow_shot/packaging/ocr-models.md`: retained 3 Community groups and applied 6 upstream groups; adapted none.
- `snow_shot/tests/screenshot_ocr_assets_tests.cpp`: retained 10 Community groups and applied 8 upstream groups; adapted retain Community cached-payload branch.
- `snow-crates/crates/simd-paddle-ocr-rs`: exact upstream object `b0d8bf38c6e83e3bfe8b58a3dad6ac24a743df7f`; adopted together with runtime 1.5.3 and protocol 8.
- `snow_shot/packaging/snow-shot-ocr-asset-manifest.json`: exact upstream object `9d1beab34336318898d956973f9ade8200152ec3`; adopted together with runtime 1.5.3 and protocol 8.
- `snow_shot/packaging/snow-shot-ocr-asset-manifest-arm64.json`: exact upstream object `3bc405aa46abd73adddb9a3dcfe2c1fdaeae93ac`; adopted together with runtime 1.5.3 and protocol 8.
The 1.5.3 manifests keep the same model IDs/default and paragraph model; the x64 and ARM64 runtime archive hashes and worker binaries changed together with the gitlink. Inno packaging now audits the Explorer DLL in standalone staging. Runtime download, native worker and offline installer acceptance are still pending.

## Applied conflict resolutions — native regression suites

- `snow_shot/tests/global_shortcut_manager_tests.cpp`: retained 3 Community groups, applied 4 disjoint upstream groups; preserve existing assertions and add upstream regression coverage. Native tests pending.
- `snow_shot/tests/image_conversion_tests.cpp`: retained 18 Community groups, applied 5 disjoint upstream groups; preserve existing assertions and add upstream regression coverage. Native tests pending.
- `snow_shot/tests/screen_recording_controller_tests.cpp`: retained 2 Community groups, applied 16 disjoint upstream groups; preserve existing assertions and add upstream regression coverage. Native tests pending.
- `snow_shot/tests/screenshot_ocr_recognition_service_tests.cpp`: retained 6 Community groups, applied 8 disjoint upstream groups; preserve existing assertions and add upstream regression coverage. Native tests pending.

## Applied conflict resolutions — storage and UI fixture coverage

- `snow_shot/tests/application_storage_tests.cpp`: retained 8 Community groups, applied 16 disjoint upstream groups. Native tests pending.
- `snow_shot/tests/configuration_archive_tests.cpp`: retained 2 Community groups, applied 6 disjoint upstream groups. Native tests pending.
- `snow_shot/tests/settings_page_loading_tests.cpp`: retained 15 Community groups, applied 5 disjoint upstream groups. Native tests pending.
- `snow_shot/tests/mcp_application_fixture_tests.py`: retained 1 Community groups, applied 4 disjoint upstream groups. Native tests pending.

## Applied conflict resolutions — startup and application services

- `snow_shot/resources/icons.manifest.json`: retained 1 Community groups, applied 1 disjoint upstream groups; account service removal preserved. Native tests pending.
- `snow_shot/src/app/main.cpp`: retained 5 Community groups, applied 13 disjoint upstream groups; account service removal preserved. Native tests pending.

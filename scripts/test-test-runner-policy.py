"""Exercise workspace test selection using a small CTest project without Qt."""

import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
CMAKE = shutil.which('cmake')
CTEST = shutil.which('ctest')


@unittest.skipUnless(CMAKE and CTEST, 'CMake and CTest are required')
class TestRunnerPolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        directory = tempfile.TemporaryDirectory(prefix='snow test runner ')
        cls.addClassCleanup(directory.cleanup)
        cls.source = Path(directory.name)
        cls.build = cls.source / 'build'
        # Test the real CMake module and executable runner, not text
        # extracted from a particular indentation/layout in CMakeLists.txt.
        cls.exclusion = 'interactive|e2e|benchmark'
        (cls.source / 'CMakeLists.txt').write_text('''
cmake_minimum_required(VERSION 3.21)
project(SnowTestRunnerPolicy NONE)
enable_testing()
add_custom_target(snow-all)
add_test(NAME portable-unit COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_BINARY_DIR}/portable-unit.ran")
set_tests_properties(portable-unit PROPERTIES LABELS "unit")
add_test(NAME windows-unit COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_BINARY_DIR}/windows-unit.ran")
set_tests_properties(windows-unit PROPERTIES LABELS "unit;windows")
add_test(NAME renamed-community-ocr-contract COMMAND "${CMAKE_COMMAND}" -E true)
set_tests_properties(renamed-community-ocr-contract PROPERTIES LABELS "unit;community-fast")
add_test(NAME native-interactive COMMAND "${CMAKE_COMMAND}" -E false)
set_tests_properties(native-interactive PROPERTIES LABELS "unit;interactive")
add_test(NAME native-e2e COMMAND "${CMAKE_COMMAND}" -E false)
set_tests_properties(native-e2e PROPERTIES LABELS "e2e")
add_test(NAME measurement COMMAND "${CMAKE_COMMAND}" -E false)
set_tests_properties(measurement PROPERTIES LABELS "benchmark")
''' + f'''
list(APPEND CMAKE_MODULE_PATH "{(ROOT / 'cmake').as_posix()}")
include(SnowTestRunner)
snow_add_deterministic_test_runner()
''', encoding='utf-8')
        configured = cls.run_command(CMAKE, '-S', str(cls.source), '-B', str(cls.build),
                                     '-DCMAKE_BUILD_TYPE=Debug')
        if configured.returncode != 0:
            raise AssertionError(configured.stdout + configured.stderr)

    @staticmethod
    def run_command(*arguments):
        return subprocess.run(arguments, capture_output=True, text=True,
                              encoding='utf-8', errors='replace', timeout=30)

    def selected_tests(self, exclusion):
        result = self.run_command(CTEST, '--test-dir', str(self.build), '-C', 'Debug',
                                  '--show-only=json-v1', '-LE', exclusion)
        self.assertEqual(result.returncode, 0, result.stderr)
        return {test['name'] for test in json.loads(result.stdout)['tests']}

    def test_runner_filter_is_valid_without_a_platform_shell(self):
        self.assertEqual(self.selected_tests(self.exclusion), {'portable-unit', 'windows-unit', 'renamed-community-ocr-contract'})

    def test_generated_runner_excludes_native_and_benchmark_fixtures(self):
        result = self.run_command(CMAKE, '--build', str(self.build), '--config', 'Debug',
                                  '--target', 'snow-tests')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.build / 'portable-unit.ran').is_file())
        self.assertTrue((self.build / 'windows-unit.ran').is_file())

    def test_default_presets_keep_the_same_noninteractive_scope(self):
        presets = json.loads((ROOT / 'CMakePresets.json').read_text(encoding='utf-8'))
        for preset in presets['testPresets']:
            exclusion = preset.get('filter', {}).get('exclude', {}).get('label')
            if exclusion is None:
                continue
            with self.subTest(preset=preset['name']):
                expected = {'portable-unit', 'renamed-community-ocr-contract'}
                if 'macos' not in preset['name']:
                    expected.add('windows-unit')
                self.assertEqual(self.selected_tests(exclusion), expected)


    def test_community_acceptance_preset_is_label_driven(self):
        presets = json.loads((ROOT / 'CMakePresets.json').read_text(encoding='utf-8'))
        community = next(item for item in presets['testPresets']
                         if item['name'] == 'test-snow-shot-msvc-community')
        self.assertEqual(community['filter'], {'include': {'label': '^community-fast$'}})
        result = self.run_command(
            CTEST, '--test-dir', str(self.build), '-C', 'Debug',
            '--show-only=json-v1', '-L', community['filter']['include']['label'])
        self.assertEqual(result.returncode, 0, result.stderr)
        selected = {test['name'] for test in json.loads(result.stdout)['tests']}
        # Deliberately unrelated to the production test names.
        self.assertEqual(selected, {'renamed-community-ocr-contract'})


    def test_nsis_registration_is_not_a_community_native_contract(self):
        # Exercise the same registration function as the Windows application,
        # without requiring Qt, PowerShell or the obsolete NSIS compiler.
        source = self.source / 'installer-labels'
        source.mkdir()
        build = source / 'build'
        (source / 'CMakeLists.txt').write_text(f'''
cmake_minimum_required(VERSION 3.21)
project(SnowInstallerLabels NONE)
enable_testing()
list(APPEND CMAKE_MODULE_PATH "{(ROOT / 'cmake').as_posix()}")
include(SnowShotNsisInstallerTests)
snow_shot_add_nsis_running_app_test("${{CMAKE_COMMAND}}" "fixture.ps1")
add_test(NAME community-native COMMAND "${{CMAKE_COMMAND}}" -E true)
set_tests_properties(community-native PROPERTIES LABELS "unit;community-fast")
''', encoding='utf-8')
        configured = self.run_command(CMAKE, '-S', str(source), '-B', str(build))
        self.assertEqual(configured.returncode, 0, configured.stdout + configured.stderr)
        for label, expected in (
            ('^community-fast$', {'community-native'}),
            ('^installer-nsis$', {'snow-shot-installer-running-app-tests'}),
            ('^unit$', {'community-native', 'snow-shot-installer-running-app-tests'}),
        ):
            with self.subTest(label=label):
                result = self.run_command(CTEST, '--test-dir', str(build),
                                          '--show-only=json-v1', '-L', label)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual({item['name'] for item in json.loads(result.stdout)['tests']},
                                 expected)



@unittest.skipUnless(CMAKE and CTEST, 'CMake and CTest are required')
class ClipboardResourcePolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        directory = tempfile.TemporaryDirectory(prefix='snow clipboard resources ')
        cls.addClassCleanup(directory.cleanup)
        cls.source = Path(directory.name)

        # Cover every declared resource family with an executable CTest fixture.
        # Do not parse add_test() source text and guess that every alias of an
        # executable uses the system clipboard.
        cls.owners = {
            'snow-shot-pinned-window-tests',
            'snow-shot-image-editor-tests',
            'snow-shot-clipboard-placement-tests',
            'snow-shot-clipboard-roundtrip-tests',
            'snow-shot-export-coordinator-tests',
            'snow-shot-screenshot-history-page-tests',
            'snow-shot-file-pin-presentation-tests',
            'snow-shot-collage-history-selection-tests',
            'snow-shot-decoded-content-presentation-tests',
        }
        cls.late_owner = 'snow-shot-pinned-future-alias-tests'
        cls.owners.add(cls.late_owner)
        cls.existing_owner = 'snow-shot-export-coordinator-tests'
        cls.unrelated = {
            'snow-shot-about-page-tests',
            'snow-shot-clipboard-preparation-tests',
            'snow-shot-mcp-document-tests',
            'snow-shot-export-artifact-tests',
            'snow-shot-history-page-size-tests',
        }
        fixture = '''cmake_minimum_required(VERSION 3.21)
project(SnowClipboardResourcePolicy NONE)
enable_testing()
set(WIN32 "${FIXTURE_WINDOWS}")
'''
        for name in sorted((cls.owners - {cls.late_owner}) | cls.unrelated):
            fixture += f'add_test(NAME {name} COMMAND "${{CMAKE_COMMAND}}" -E true)\n'
        fixture += f'''set_tests_properties({cls.existing_owner} PROPERTIES
    RESOURCE_LOCK fixture-existing-resource)
list(APPEND CMAKE_MODULE_PATH "{(ROOT / 'cmake').as_posix()}")
include(SnowShotTestResources)
add_test(NAME {cls.late_owner} COMMAND "${{CMAKE_COMMAND}}" -E true)
'''
        (cls.source / 'CMakeLists.txt').write_text(fixture, encoding='utf-8')
        cls.tests = {}
        for platform in ('ON', 'OFF'):
            build = cls.source / platform
            configured = TestRunnerPolicyTests.run_command(
                CMAKE, '-S', str(cls.source), '-B', str(build),
                '-DCMAKE_BUILD_TYPE=Debug', f'-DFIXTURE_WINDOWS={platform}')
            if configured.returncode != 0:
                raise AssertionError(configured.stdout + configured.stderr)
            inventory = TestRunnerPolicyTests.run_command(
                CTEST, '--test-dir', str(build), '-C', 'Debug', '--show-only=json-v1')
            if inventory.returncode != 0:
                raise AssertionError(inventory.stdout + inventory.stderr)
            cls.tests[platform] = {
                test['name']: {entry['name']: entry['value']
                               for entry in test.get('properties', [])}
                for test in json.loads(inventory.stdout)['tests']}

    def test_every_shared_host_alias_owns_the_windows_clipboard(self):
        for name in self.owners:
            with self.subTest(test=name):
                resources = self.tests['ON'][name].get('RESOURCE_LOCK', [])
                self.assertIn('snow-shot-system-clipboard', resources)

    def test_unrelated_hosts_keep_parallel_execution(self):
        for name in self.unrelated:
            with self.subTest(test=name):
                self.assertNotIn('RESOURCE_LOCK', self.tests['ON'][name])

    def test_clipboard_resource_appends_to_existing_ownership(self):
        self.assertEqual(set(self.tests['ON'][self.existing_owner]['RESOURCE_LOCK']),
                         {'fixture-existing-resource', 'snow-shot-system-clipboard'})

    def test_other_platforms_keep_their_existing_resources(self):
        for name, properties in self.tests['OFF'].items():
            with self.subTest(test=name):
                self.assertNotIn('snow-shot-system-clipboard',
                                 properties.get('RESOURCE_LOCK', []))
        self.assertEqual(self.tests['OFF'][self.existing_owner]['RESOURCE_LOCK'],
                         ['fixture-existing-resource'])


if __name__ == '__main__':
    unittest.main()

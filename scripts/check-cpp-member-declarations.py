#!/usr/bin/env python3
"""Fast, parser-based C++ class declaration/definition preflight.

This is intentionally narrower than a C++ compiler: it compares names of
out-of-class member definitions with names declared in the actual class AST.
It does not type-check overloads, generated moc code, platform SDK headers,
or the contents of method bodies. Full Clang/MSVC compilation is still required.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import tree_sitter_cpp
from tree_sitter import Language, Parser

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "snow_shot/include/snow_shot/presentation/screenshottoolpalette.h"
SOURCE = ROOT / "snow_shot/src/presentation/tools/screenshottoolpalette.cpp"
CLASS_NAME = "ScreenshotToolPalette"
MIN_DECLARATIONS = 180
MIN_DEFINITIONS = 250


def parser() -> Parser:
    return Parser(Language(tree_sitter_cpp.language()))


def children(node):
    """Walk named AST nodes without text-based function-name matching."""
    stack = [node]
    while stack:
        current = stack.pop()
        yield current
        stack.extend(reversed(current.named_children))


def node_text(node, source: bytes) -> str:
    return source[node.start_byte:node.end_byte].decode("utf-8")


def function_name(node, source: bytes):
    """Return (scope, name) from an AST function declarator, when available."""
    current = node
    for _ in range(12):
        if current is None:
            return None
        if current.type == "qualified_identifier":
            scope = current.child_by_field_name("scope")
            name = current.child_by_field_name("name")
            if scope is not None and name is not None:
                return (node_text(scope, source), node_text(name, source))
            return None
        if current.type in {
            "identifier", "field_identifier", "type_identifier",
            "destructor_name", "operator_name", "operator_cast",
        }:
            return ("", "".join(node_text(current, source).split()))
        next_node = current.child_by_field_name("declarator")
        if next_node is None and current.type in {
            "parenthesized_declarator", "pointer_declarator", "reference_declarator"
        }:
            next_node = next((c for c in current.named_children if c.type != "type_qualifier"), None)
        current = next_node
    return None


def declared_members(header: bytes, class_name: str, parser_instance: Parser):
    root = parser_instance.parse(header).root_node
    classes = []
    for node in children(root):
        if node.type not in {"class_specifier", "struct_specifier"}:
            continue
        name = node.child_by_field_name("name")
        body = node.child_by_field_name("body")
        if name is not None and body is not None and node_text(name, header) == class_name:
            classes.append(body)
    if len(classes) != 1:
        raise RuntimeError(
            f"expected exactly one parsed {class_name} class body, found {len(classes)}"
        )

    declared = set()
    stack = list(classes[0].named_children)
    while stack:
        node = stack.pop()
        # A nested class does not declare members of the enclosing class.
        if node.type in {"class_specifier", "struct_specifier", "compound_statement"}:
            continue
        if node.type == "function_declarator":
            candidate = function_name(node.child_by_field_name("declarator"), header)
            if candidate is not None and not candidate[0]:
                declared.add(candidate[1])
        stack.extend(node.named_children)
    return declared


def defined_members(source: bytes, class_name: str, parser_instance: Parser):
    root = parser_instance.parse(source).root_node
    result = []
    for node in children(root):
        if node.type != "function_definition":
            continue
        decl = node.child_by_field_name("declarator")
        member = function_name(decl, source) if decl is not None else None
        if member is None:
            continue
        scope, name = member
        if scope.split("::")[-1] == class_name:
            result.append((name, node.start_point.row + 1))
    return result


def audit(header: bytes, source: bytes, class_name: str, *,
          min_declarations: int = 0, min_definitions: int = 0):
    p = parser()
    declarations = declared_members(header, class_name, p)
    definitions = defined_members(source, class_name, p)
    if len(declarations) < min_declarations or len(definitions) < min_definitions:
        raise RuntimeError(
            f"AST coverage unexpectedly low: {len(declarations)} declarations, "
            f"{len(definitions)} definitions; parser/preprocessor may have lost the class"
        )
    missing = [(name, line) for name, line in definitions if name not in declarations]
    return len(declarations), len(definitions), missing


def self_test(header: bytes, source: bytes):
    fixture_header = b"class Demo { public: void ready(); void overloaded(int); };"
    fixture_source = (b"void Demo::ready() {}\n"
                      b"void Demo::overloaded(int) {}\n"
                      b"void Demo::orphan() {}\n")
    count, definitions, missing = audit(fixture_header, fixture_source, "Demo",
                                        min_declarations=2, min_definitions=3)
    assert count == 2 and definitions == 3 and missing == [("orphan", 3)], (
        "AST checker must reject an out-of-class definition without a declaration"
    )
    # Re-introduce the exact class/member bug found in real Windows CI #139.
    regression = source + (
        b"\n#if SNOW_SHOT_ENABLE_IMAGE_CONVERSION\n"
        b"void ScreenshotToolPalette::createImageConversionActionFamily() {}\n"
        b"#endif\n"
    )
    _, _, real_missing = audit(header, regression, CLASS_NAME,
                               min_declarations=MIN_DECLARATIONS,
                               min_definitions=MIN_DEFINITIONS)
    assert [name for name, _ in real_missing] == ["createImageConversionActionFamily"], (
        f"original production regression must be caught exactly, got {real_missing}"
    )
    print("AST self-test passed: undeclared Demo and #139 production member rejected")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--self-test", action="store_true",
                    help="prove that the original MSVC C2039 regression fails the AST gate")
    args = ap.parse_args()
    header = HEADER.read_bytes()
    source = SOURCE.read_bytes()
    try:
        if args.self_test:
            self_test(header, source)
        count, definitions, missing = audit(
            header, source, CLASS_NAME,
            min_declarations=MIN_DECLARATIONS, min_definitions=MIN_DEFINITIONS
        )
    except (AssertionError, RuntimeError) as exc:
        print(f"C++ member preflight failed: {exc}", file=sys.stderr)
        return 1
    print(f"C++ AST preflight: {count} declared member names, "
          f"{definitions} parsed out-of-class definitions")
    for name, line in missing:
        print(f"::error file={SOURCE.relative_to(ROOT)},line={line}::"
              f"{CLASS_NAME}::{name} has no declaration in the class header",
              file=sys.stderr)
    if missing:
        print(f"{len(missing)} undeclared member definition(s); "
              "full C++ compilation remains required", file=sys.stderr)
        return 1
    print("C++ member declaration/name check passed (not a full compiler check)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

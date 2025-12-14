#!/usr/bin/env python3
"""
Codebase Reviewer - Comprehensive C/C++ Codebase Analysis Tool

A portable tool for analyzing C/C++ projects, generating a comprehensive
HTML report covering:
  - Architecture & Modules (directory structure, module relationships)
  - Interfaces & Data Structures (structs, enums, typedefs, function signatures)
  - Pipelines & Workflows (call graphs, data flow)
  - Procedures (function analysis, complexity metrics)
  - Dependencies (reuses cdep_analyzer)

Compatible with Python 3.6.3+ (stdlib only, no external dependencies)

Usage:
    python3 codebase_reviewer.py /path/to/project

Output files (in analyzer directory):
    - review_report.html: Comprehensive codebase review

For dependency-only analysis, use cdep_analyzer.py directly.

Author: Codebase Review Tool
"""

from __future__ import print_function

import argparse
import json
import os
import re
import sys
from collections import defaultdict
from datetime import datetime

# Version check for Python 3.6+
if sys.version_info < (3, 6):
    print("Error: Python 3.6 or higher is required")
    sys.exit(1)

# Import cdep_analyzer components (same directory)
try:
    from cdep_analyzer import (
        DependencyScanner,
        CleanArchAnalyzer,
        DEFAULT_EXCLUDES,
        ALL_EXTENSIONS,
        C_EXTENSIONS,
        get_d3_script_tag,
    )
except ImportError:
    print("Error: cdep_analyzer.py must be in the same directory")
    sys.exit(1)


# =============================================================================
# Configuration
# =============================================================================

# Regex patterns for C/C++ parsing
PATTERNS = {
    # Struct/union definitions: matches both "struct Name { ... }" and "typedef struct { ... } Name;"
    'struct': re.compile(
        r'(?:typedef\s+)?(?:struct|union)\s+(\w+)\s*\{([^}]*)\}|'  # struct Name { ... }
        r'typedef\s+(?:struct|union)\s*\{([^}]*)\}\s*(\w+)\s*;',   # typedef struct { ... } Name;
        re.MULTILINE | re.DOTALL
    ),
    # Enum definitions: matches both "enum Name { ... }" and "typedef enum { ... } Name;"
    'enum': re.compile(
        r'(?:typedef\s+)?enum\s+(\w*)\s*\{([^}]*)\}|'             # enum Name { ... }
        r'typedef\s+enum\s*\{([^}]*)\}\s*(\w+)\s*;',              # typedef enum { ... } Name;
        re.MULTILINE | re.DOTALL
    ),
    # Typedef (simple)
    'typedef': re.compile(
        r'typedef\s+(.+?)\s+(\w+)\s*;',
        re.MULTILINE
    ),
    # Function declarations in headers
    'func_decl': re.compile(
        r'^[\s]*(?:extern\s+)?(?:static\s+)?(?:inline\s+)?'
        r'(?:const\s+)?(\w+(?:\s*\*)*)\s+'
        r'(\w+)\s*\(([^)]*)\)\s*;',
        re.MULTILINE
    ),
    # Function definitions (implementation)
    'func_def': re.compile(
        r'^(?:static\s+)?(?:inline\s+)?(?:const\s+)?'
        r'(\w+(?:\s*\*)*)\s+(\w+)\s*\(([^)]*)\)\s*\{',
        re.MULTILINE
    ),
    # Function calls
    'func_call': re.compile(
        r'\b(\w+)\s*\(',
        re.MULTILINE
    ),
    # Macro definitions
    'macro': re.compile(
        r'^#define\s+(\w+)(?:\([^)]*\))?\s+(.*)$',
        re.MULTILINE
    ),
    # Control flow keywords (for complexity)
    'if': re.compile(r'\bif\s*\('),
    'else': re.compile(r'\belse\b'),
    'for': re.compile(r'\bfor\s*\('),
    'while': re.compile(r'\bwhile\s*\('),
    'switch': re.compile(r'\bswitch\s*\('),
    'case': re.compile(r'\bcase\s+'),
    'return': re.compile(r'\breturn\b'),
}

# Keywords to ignore as function calls
C_KEYWORDS = {
    'if', 'else', 'for', 'while', 'do', 'switch', 'case', 'default',
    'return', 'break', 'continue', 'goto', 'sizeof', 'typeof',
    'struct', 'union', 'enum', 'typedef', 'const', 'static', 'extern',
    'inline', 'volatile', 'register', 'auto', 'signed', 'unsigned',
    'void', 'char', 'short', 'int', 'long', 'float', 'double',
    'NULL', 'true', 'false',
}

# Default patterns for auto-categorizing data structures
DEFAULT_CATEGORY_PATTERNS = {
    'config': ['Config'],           # Matches *Config*
    'algorithm': ['Param', 'Algorithm', 'Buffer', 'Scalar', 'Op'],  # Algorithm-related
    'platform': ['Env', 'Context', 'CL'],  # Platform/OpenCL related
}

# Category colors for visualization
CATEGORY_COLORS = {
    'config': '#2196F3',      # Blue
    'algorithm': '#4CAF50',   # Green
    'platform': '#FF9800',    # Orange
    'unknown': '#9E9E9E',     # Gray
}


# =============================================================================
# Data Focus Configuration
# =============================================================================

class DataFocusConfig:
    """Manages data structure focus configuration."""

    def __init__(self, config_path=None):
        self.config_path = config_path
        self.categories = {}       # category_name -> [struct_names]
        self.struct_categories = {}  # struct_name -> category
        self.show_only_focused = False
        self.focus_categories = ['config', 'algorithm']  # Default focus

        if config_path and os.path.exists(config_path):
            self._load_config()

    def _load_config(self):
        """Load configuration from JSON file."""
        try:
            with open(self.config_path, 'r', encoding='utf-8') as f:
                config = json.load(f)

            if 'categories' in config:
                self.categories = config['categories']
                # Build reverse mapping
                for cat, structs in self.categories.items():
                    for s in structs:
                        self.struct_categories[s] = cat

            if 'show_only_focused' in config:
                self.show_only_focused = config['show_only_focused']

            if 'focus_categories' in config:
                self.focus_categories = config['focus_categories']

        except (IOError, json.JSONDecodeError) as e:
            print("Warning: Could not load config {}: {}".format(self.config_path, e))

    def auto_categorize(self, struct_name):
        """Auto-categorize a struct based on naming patterns."""
        # Check if already in config
        if struct_name in self.struct_categories:
            return self.struct_categories[struct_name]

        # Auto-detect based on patterns
        for category, patterns in DEFAULT_CATEGORY_PATTERNS.items():
            for pattern in patterns:
                if pattern in struct_name:
                    return category

        return 'unknown'

    def generate_config(self, structs, output_path):
        """Generate a configuration file with auto-detected categories."""
        categories = defaultdict(list)

        for s in structs:
            name = s['name']
            cat = self.auto_categorize(name)
            categories[cat].append(name)

        # Sort structs within each category
        for cat in categories:
            categories[cat] = sorted(set(categories[cat]))

        config = {
            '_comment': 'Data structure focus configuration. Edit to customize.',
            '_usage': 'Set show_only_focused=true to filter the graph to focus_categories only.',
            'categories': dict(categories),
            'focus_categories': ['config', 'algorithm'],
            'show_only_focused': False,
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2)

        return output_path

    def is_focused(self, struct_name):
        """Check if a struct is in a focused category."""
        if not self.show_only_focused:
            return True
        cat = self.struct_categories.get(struct_name, self.auto_categorize(struct_name))
        return cat in self.focus_categories

    def get_category(self, struct_name):
        """Get the category for a struct."""
        if struct_name in self.struct_categories:
            return self.struct_categories[struct_name]
        return self.auto_categorize(struct_name)

    def get_color(self, struct_name):
        """Get the color for a struct based on its category."""
        cat = self.get_category(struct_name)
        return CATEGORY_COLORS.get(cat, CATEGORY_COLORS['unknown'])


# =============================================================================
# Interface Scanner - Extracts structs, enums, typedefs, function signatures
# =============================================================================

class InterfaceScanner:
    """Scans C/C++ headers for interface definitions and usage."""

    def __init__(self, scanner):
        """
        Initialize with a DependencyScanner instance.

        Args:
            scanner: DependencyScanner with scanned files
        """
        self.scanner = scanner
        self.structs = []      # List of struct definitions
        self.enums = []        # List of enum definitions
        self.typedefs = []     # List of typedef definitions
        self.functions = []    # List of function declarations
        self.macros = []       # List of macro definitions

        # Usage tracking: struct_name -> {file -> {reads, writes, refs}}
        self.struct_usage = defaultdict(lambda: defaultdict(lambda: {'reads': 0, 'writes': 0, 'refs': 0}))
        self.enum_usage = defaultdict(lambda: defaultdict(lambda: {'refs': 0}))

    def scan(self):
        """Scan all header files for interface definitions."""
        # First pass: find definitions in headers
        for rel_path, info in self.scanner.files.items():
            if not info['is_header']:
                continue

            try:
                with open(info['full_path'], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                self._parse_structs(content, rel_path)
                self._parse_enums(content, rel_path)
                self._parse_typedefs(content, rel_path)
                self._parse_functions(content, rel_path)
                self._parse_macros(content, rel_path)

            except (IOError, OSError) as e:
                print("Warning: Could not read {}: {}".format(rel_path, e))

        # Second pass: scan all files for usage
        self._scan_usage()

        return self

    def _scan_usage(self):
        """Scan all files for data structure usage."""
        # Build lookup sets for quick matching
        struct_names = {s['name'] for s in self.structs}
        enum_names = {e['name'] for e in self.enums if e['name'] != '(anonymous)'}

        # Patterns for detecting read/write access
        # Write: assignment, passing to function that modifies, -> member =
        # Read: accessing members, passing to functions, comparisons
        write_pattern = re.compile(
            r'(\w+)\s*(?:->|\.)\s*\w+\s*=|'        # ptr->field = or obj.field =
            r'(\w+)\s*=\s*[^=]|'                   # var =
            r'\*\s*(\w+)\s*=',                      # *ptr =
            re.MULTILINE
        )

        read_pattern = re.compile(
            r'(\w+)\s*(?:->|\.)\s*\w+(?!\s*=)|'    # ptr->field or obj.field (not followed by =)
            r'[=!<>]=?\s*(\w+)|'                    # comparison or assignment RHS
            r'\(\s*(\w+)\s*\)|'                     # function argument
            r'return\s+(\w+)',                      # return value
            re.MULTILINE
        )

        for rel_path, info in self.scanner.files.items():
            try:
                with open(info['full_path'], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                # Check for struct type usage
                for struct_name in struct_names:
                    # Pattern to find usage of struct type
                    type_pattern = re.compile(
                        r'\b(?:struct\s+)?' + re.escape(struct_name) + r'\b\s*[\*\s]+(\w+)',
                        re.MULTILINE
                    )

                    # Find variable declarations of this struct type
                    var_names = set()
                    for match in type_pattern.finditer(content):
                        var_name = match.group(1)
                        if var_name not in C_KEYWORDS:
                            var_names.add(var_name)
                            self.struct_usage[struct_name][rel_path]['refs'] += 1

                    # Check if these variables are read or written
                    for var_name in var_names:
                        # Count writes
                        var_write = re.compile(
                            r'\b' + re.escape(var_name) + r'\s*(?:->|\.)\s*\w+\s*=|'
                            r'\b' + re.escape(var_name) + r'\s*=\s*[^=]',
                            re.MULTILINE
                        )
                        writes = len(var_write.findall(content))
                        self.struct_usage[struct_name][rel_path]['writes'] += writes

                        # Count reads (approximate)
                        var_read = re.compile(
                            r'\b' + re.escape(var_name) + r'\s*(?:->|\.)\s*\w+(?!\s*=)|'
                            r'\(\s*' + re.escape(var_name) + r'\s*[,)]',
                            re.MULTILINE
                        )
                        reads = len(var_read.findall(content))
                        self.struct_usage[struct_name][rel_path]['reads'] += reads

                # Check for enum usage
                for enum_name in enum_names:
                    if re.search(r'\b' + re.escape(enum_name) + r'\b', content):
                        self.enum_usage[enum_name][rel_path]['refs'] += 1

            except (IOError, OSError):
                pass

    def get_struct_accessors(self, struct_name):
        """Get modules that access a struct, grouped by access type."""
        usage = self.struct_usage.get(struct_name, {})

        readers = []
        writers = []
        all_accessors = []

        for file_path, access in usage.items():
            module = os.path.dirname(file_path) or '.'
            if access['writes'] > 0:
                writers.append({'file': file_path, 'module': module, 'writes': access['writes']})
            if access['reads'] > 0:
                readers.append({'file': file_path, 'module': module, 'reads': access['reads']})
            if access['refs'] > 0:
                all_accessors.append({'file': file_path, 'module': module, 'refs': access['refs']})

        return {
            'readers': readers,
            'writers': writers,
            'all': all_accessors,
        }

    def _parse_structs(self, content, file_path):
        """Extract struct/union definitions."""
        for match in PATTERNS['struct'].finditer(content):
            # Handle both patterns:
            # Group 1,2: struct Name { body }
            # Group 3,4: typedef struct { body } Name;
            if match.group(1):
                name = match.group(1)
                body = match.group(2)
            elif match.group(4):
                name = match.group(4)
                body = match.group(3)
            else:
                continue

            # Parse fields
            fields = []
            field_types = []  # Track referenced types
            for line in body.split('\n'):
                line = line.strip()
                if line and not line.startswith('//') and not line.startswith('/*'):
                    # Simple field extraction
                    if ';' in line:
                        field = line.rstrip(';').strip()
                        fields.append(field)
                        # Extract type from field (first word(s) before last identifier)
                        parts = field.split()
                        if len(parts) >= 2:
                            # Type is everything except the last word (variable name)
                            type_part = ' '.join(parts[:-1]).replace('*', '').strip()
                            if type_part and type_part not in C_KEYWORDS:
                                field_types.append(type_part)

            self.structs.append({
                'name': name,
                'file': file_path,
                'module': os.path.dirname(file_path) or '.',
                'fields': fields,
                'field_count': len(fields),
                'references': field_types,  # Types referenced by this struct
            })

    def _parse_enums(self, content, file_path):
        """Extract enum definitions."""
        for match in PATTERNS['enum'].finditer(content):
            # Handle both patterns:
            # Group 1,2: enum Name { body }
            # Group 3,4: typedef enum { body } Name;
            if match.group(1) is not None:  # Could be empty string for anonymous
                name = match.group(1) or '(anonymous)'
                body = match.group(2)
            elif match.group(4):
                name = match.group(4)
                body = match.group(3)
            else:
                continue

            # Parse values
            values = []
            for item in body.split(','):
                item = item.strip()
                if item and not item.startswith('//'):
                    # Extract just the name (before = if present)
                    value_name = item.split('=')[0].strip()
                    if value_name:
                        values.append(value_name)

            self.enums.append({
                'name': name,
                'file': file_path,
                'values': values,
                'value_count': len(values),
            })

    def _parse_typedefs(self, content, file_path):
        """Extract typedef definitions."""
        for match in PATTERNS['typedef'].finditer(content):
            original = match.group(1).strip()
            alias = match.group(2).strip()

            # Skip struct/enum typedefs (already captured)
            if 'struct' in original or 'enum' in original:
                continue

            self.typedefs.append({
                'name': alias,
                'original': original,
                'file': file_path,
            })

    def _parse_functions(self, content, file_path):
        """Extract function declarations."""
        for match in PATTERNS['func_decl'].finditer(content):
            return_type = match.group(1).strip()
            name = match.group(2).strip()
            params = match.group(3).strip()

            # Skip if it looks like a macro or keyword
            if name.isupper() or name in C_KEYWORDS:
                continue

            self.functions.append({
                'name': name,
                'return_type': return_type,
                'params': params,
                'file': file_path,
                'is_declaration': True,
            })

    def _parse_macros(self, content, file_path):
        """Extract macro definitions."""
        for match in PATTERNS['macro'].finditer(content):
            name = match.group(1)
            value = match.group(2).strip() if match.group(2) else ''

            # Skip include guards and common patterns
            if name.startswith('_') and name.endswith('_H'):
                continue
            if name in ('__cplusplus', '__STDC__'):
                continue

            self.macros.append({
                'name': name,
                'value': value[:50] + ('...' if len(value) > 50 else ''),
                'file': file_path,
            })

    def get_stats(self):
        """Get interface statistics."""
        return {
            'structs': len(self.structs),
            'enums': len(self.enums),
            'typedefs': len(self.typedefs),
            'functions': len(self.functions),
            'macros': len(self.macros),
        }


# =============================================================================
# Call Graph Analyzer - Builds function call relationships
# =============================================================================

class CallGraphAnalyzer:
    """Analyzes function calls to build call graph."""

    def __init__(self, scanner):
        """
        Initialize with a DependencyScanner instance.

        Args:
            scanner: DependencyScanner with scanned files
        """
        self.scanner = scanner
        self.functions = {}      # func_name -> {file, return_type, params, calls, called_by}
        self.call_edges = []     # List of (caller, callee) pairs
        self.file_functions = defaultdict(list)  # file -> list of functions

    def scan(self):
        """Scan all source files for function definitions and calls."""
        # First pass: find all function definitions
        for rel_path, info in self.scanner.files.items():
            try:
                with open(info['full_path'], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                self._find_definitions(content, rel_path)

            except (IOError, OSError) as e:
                print("Warning: Could not read {}: {}".format(rel_path, e))

        # Second pass: find function calls
        for rel_path, info in self.scanner.files.items():
            if info['is_header']:
                continue  # Focus on source files for calls

            try:
                with open(info['full_path'], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                self._find_calls(content, rel_path)

            except (IOError, OSError):
                pass

        return self

    def _find_definitions(self, content, file_path):
        """Find function definitions in a file."""
        for match in PATTERNS['func_def'].finditer(content):
            return_type = match.group(1).strip()
            name = match.group(2).strip()
            params = match.group(3).strip()

            if name in C_KEYWORDS:
                continue

            self.functions[name] = {
                'name': name,
                'file': file_path,
                'return_type': return_type,
                'params': params,
                'calls': set(),
                'called_by': set(),
            }
            self.file_functions[file_path].append(name)

    def _find_calls(self, content, file_path):
        """Find function calls in a file."""
        # Get functions defined in this file
        local_funcs = set(self.file_functions.get(file_path, []))

        # Find the current function context
        current_func = None
        lines = content.split('\n')

        for i, line in enumerate(lines):
            # Check if this line starts a function definition
            match = PATTERNS['func_def'].match(line)
            if match:
                current_func = match.group(2).strip()
                continue

            if current_func is None:
                continue

            # Find function calls in this line
            for call_match in PATTERNS['func_call'].finditer(line):
                callee = call_match.group(1)

                if callee in C_KEYWORDS:
                    continue
                if callee == current_func:
                    continue  # Skip recursion for simplicity

                # Record the call if callee is a known function
                if callee in self.functions:
                    self.functions[current_func]['calls'].add(callee)
                    self.functions[callee]['called_by'].add(current_func)
                    self.call_edges.append((current_func, callee))

    def get_entry_points(self):
        """Get functions that are not called by any other function."""
        return [
            name for name, info in self.functions.items()
            if not info['called_by']
        ]

    def get_leaf_functions(self):
        """Get functions that don't call any other tracked function."""
        return [
            name for name, info in self.functions.items()
            if not info['calls']
        ]

    def get_most_called(self, limit=20):
        """Get most frequently called functions."""
        counts = [
            (name, len(info['called_by']))
            for name, info in self.functions.items()
        ]
        counts.sort(key=lambda x: -x[1])
        return counts[:limit]

    def get_most_calling(self, limit=20):
        """Get functions with most outgoing calls."""
        counts = [
            (name, len(info['calls']))
            for name, info in self.functions.items()
        ]
        counts.sort(key=lambda x: -x[1])
        return counts[:limit]

    def get_stats(self):
        """Get call graph statistics."""
        return {
            'total_functions': len(self.functions),
            'total_calls': len(self.call_edges),
            'entry_points': len(self.get_entry_points()),
            'leaf_functions': len(self.get_leaf_functions()),
        }


# =============================================================================
# Procedure Analyzer - Function complexity and control flow
# =============================================================================

class ProcedureAnalyzer:
    """Analyzes function complexity and control flow."""

    def __init__(self, scanner):
        """
        Initialize with a DependencyScanner instance.

        Args:
            scanner: DependencyScanner with scanned files
        """
        self.scanner = scanner
        self.procedures = []  # List of procedure analysis results

    def scan(self):
        """Analyze all source files for procedure complexity."""
        for rel_path, info in self.scanner.files.items():
            if info['is_header']:
                continue

            try:
                with open(info['full_path'], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                self._analyze_file(content, rel_path)

            except (IOError, OSError) as e:
                print("Warning: Could not read {}: {}".format(rel_path, e))

        return self

    def _analyze_file(self, content, file_path):
        """Analyze functions in a file."""
        # Find function boundaries
        func_pattern = re.compile(
            r'^(?:static\s+)?(?:inline\s+)?(?:const\s+)?'
            r'(\w+(?:\s*\*)*)\s+(\w+)\s*\(([^)]*)\)\s*\{',
            re.MULTILINE
        )

        matches = list(func_pattern.finditer(content))

        for i, match in enumerate(matches):
            name = match.group(2)
            if name in C_KEYWORDS:
                continue

            start = match.end()

            # Find the matching closing brace
            end = self._find_function_end(content, start)
            if end == -1:
                continue

            func_body = content[start:end]

            # Calculate complexity metrics
            metrics = self._calculate_complexity(func_body)
            metrics['name'] = name
            metrics['file'] = file_path
            metrics['return_type'] = match.group(1).strip()
            metrics['params'] = match.group(3).strip()
            metrics['line_count'] = func_body.count('\n') + 1

            self.procedures.append(metrics)

    def _find_function_end(self, content, start):
        """Find the matching closing brace for a function."""
        depth = 1
        i = start
        while i < len(content) and depth > 0:
            if content[i] == '{':
                depth += 1
            elif content[i] == '}':
                depth -= 1
            i += 1
        return i if depth == 0 else -1

    def _calculate_complexity(self, body):
        """Calculate complexity metrics for a function body."""
        # Count control flow statements
        if_count = len(PATTERNS['if'].findall(body))
        else_count = len(PATTERNS['else'].findall(body))
        for_count = len(PATTERNS['for'].findall(body))
        while_count = len(PATTERNS['while'].findall(body))
        switch_count = len(PATTERNS['switch'].findall(body))
        case_count = len(PATTERNS['case'].findall(body))
        return_count = len(PATTERNS['return'].findall(body))

        # Simple cyclomatic complexity approximation
        # CC = 1 + if + for + while + case
        cyclomatic = 1 + if_count + for_count + while_count + case_count

        return {
            'if_count': if_count,
            'else_count': else_count,
            'for_count': for_count,
            'while_count': while_count,
            'switch_count': switch_count,
            'case_count': case_count,
            'return_count': return_count,
            'cyclomatic': cyclomatic,
        }

    def get_complex_functions(self, threshold=10):
        """Get functions with cyclomatic complexity above threshold."""
        return [
            p for p in self.procedures
            if p['cyclomatic'] >= threshold
        ]

    def get_large_functions(self, threshold=50):
        """Get functions with more lines than threshold."""
        return [
            p for p in self.procedures
            if p['line_count'] >= threshold
        ]

    def get_stats(self):
        """Get procedure statistics."""
        if not self.procedures:
            return {
                'total_procedures': 0,
                'avg_complexity': 0,
                'avg_lines': 0,
                'max_complexity': 0,
                'max_lines': 0,
            }

        complexities = [p['cyclomatic'] for p in self.procedures]
        lines = [p['line_count'] for p in self.procedures]

        return {
            'total_procedures': len(self.procedures),
            'avg_complexity': sum(complexities) / len(complexities),
            'avg_lines': sum(lines) / len(lines),
            'max_complexity': max(complexities),
            'max_lines': max(lines),
        }


# =============================================================================
# Module Analyzer - Directory/module structure analysis
# =============================================================================

class ModuleAnalyzer:
    """Analyzes module/directory structure."""

    def __init__(self, scanner, ca_analyzer=None):
        """
        Initialize with scanners.

        Args:
            scanner: DependencyScanner with scanned files
            ca_analyzer: Optional CleanArchAnalyzer for layer info
        """
        self.scanner = scanner
        self.ca_analyzer = ca_analyzer
        self.modules = {}  # directory -> module info

    def analyze(self):
        """Analyze module structure."""
        # Group files by directory
        for rel_path, info in self.scanner.files.items():
            dir_name = info['directory']

            if dir_name not in self.modules:
                self.modules[dir_name] = {
                    'name': dir_name,
                    'files': [],
                    'headers': 0,
                    'sources': 0,
                    'lines': 0,
                    'dependencies': set(),
                    'dependents': set(),
                    'layer': None,
                }

            mod = self.modules[dir_name]
            mod['files'].append(rel_path)
            mod['lines'] += info['line_count']

            if info['is_header']:
                mod['headers'] += 1
            else:
                mod['sources'] += 1

        # Calculate inter-module dependencies
        for src_file, deps in self.scanner.dependencies.items():
            src_dir = os.path.dirname(src_file) or '.'

            for dep_file in deps:
                dep_dir = os.path.dirname(dep_file) or '.'

                if src_dir != dep_dir:
                    if src_dir in self.modules:
                        self.modules[src_dir]['dependencies'].add(dep_dir)
                    if dep_dir in self.modules:
                        self.modules[dep_dir]['dependents'].add(src_dir)

        # Add layer info if available
        if self.ca_analyzer:
            for dir_name, mod in self.modules.items():
                # Get dominant layer for this module
                layer_counts = defaultdict(int)
                for f in mod['files']:
                    layer = self.ca_analyzer.file_layers.get(f)
                    if layer:
                        layer_counts[layer] += 1

                if layer_counts:
                    mod['layer'] = max(layer_counts.items(), key=lambda x: x[1])[0]

        return self

    def get_stats(self):
        """Get module statistics."""
        return {
            'total_modules': len(self.modules),
            'total_files': sum(len(m['files']) for m in self.modules.values()),
            'total_lines': sum(m['lines'] for m in self.modules.values()),
        }


# =============================================================================
# HTML Report Generator
# =============================================================================

def generate_review_report(
    scanner,
    interface_scanner,
    call_graph,
    procedure_analyzer,
    module_analyzer,
    ca_analyzer,
    data_focus,
    output_path
):
    """Generate comprehensive HTML review report."""

    # Gather statistics
    dep_stats = scanner.get_stats()
    interface_stats = interface_scanner.get_stats()
    call_stats = call_graph.get_stats()
    proc_stats = procedure_analyzer.get_stats()
    mod_stats = module_analyzer.get_stats()

    # Prepare data for visualization
    modules_data = []
    for name, mod in module_analyzer.modules.items():
        modules_data.append({
            'name': name,
            'files': len(mod['files']),
            'headers': mod['headers'],
            'sources': mod['sources'],
            'lines': mod['lines'],
            'deps': list(mod['dependencies']),
            'dependents': list(mod['dependents']),
            'layer': mod['layer'],
        })

    # Call graph data
    call_nodes = []
    call_links = []
    func_index = {}

    for idx, (name, info) in enumerate(call_graph.functions.items()):
        func_index[name] = idx
        call_nodes.append({
            'id': idx,
            'name': name,
            'file': info['file'],
            'calls': len(info['calls']),
            'called_by': len(info['called_by']),
        })

    for caller, callee in call_graph.call_edges:
        if caller in func_index and callee in func_index:
            call_links.append({
                'source': func_index[caller],
                'target': func_index[callee],
            })

    # Module-level call graph aggregation
    module_funcs = defaultdict(list)  # module -> list of function names
    for name, info in call_graph.functions.items():
        module = os.path.dirname(info['file']) or info['file']
        module_funcs[module].append(name)

    module_call_nodes = []
    module_index = {}
    for idx, (module, funcs) in enumerate(module_funcs.items()):
        module_index[module] = idx
        # Calculate aggregated stats
        total_calls = sum(len(call_graph.functions[f]['calls']) for f in funcs)
        total_called_by = sum(len(call_graph.functions[f]['called_by']) for f in funcs)
        module_call_nodes.append({
            'id': idx,
            'name': os.path.basename(module) if module else 'root',
            'path': module,
            'func_count': len(funcs),
            'functions': funcs,
            'calls': total_calls,
            'called_by': total_called_by,
        })

    # Module-to-module call links (aggregate function calls)
    module_call_links = []
    module_edge_counts = defaultdict(int)  # (src_mod, tgt_mod) -> count
    for caller, callee in call_graph.call_edges:
        if caller in call_graph.functions and callee in call_graph.functions:
            src_mod = os.path.dirname(call_graph.functions[caller]['file']) or call_graph.functions[caller]['file']
            tgt_mod = os.path.dirname(call_graph.functions[callee]['file']) or call_graph.functions[callee]['file']
            if src_mod != tgt_mod:  # Only cross-module calls
                module_edge_counts[(src_mod, tgt_mod)] += 1

    for (src_mod, tgt_mod), count in module_edge_counts.items():
        if src_mod in module_index and tgt_mod in module_index:
            module_call_links.append({
                'source': module_index[src_mod],
                'target': module_index[tgt_mod],
                'count': count,
            })

    # Procedure data
    procedures_data = sorted(
        procedure_analyzer.procedures,
        key=lambda x: -x['cyclomatic']
    )[:50]  # Top 50 complex functions

    # Clean Architecture layers
    ca_layers = ca_analyzer.layers if ca_analyzer else []
    layer_stats = ca_analyzer.get_layer_stats() if ca_analyzer else {}

    # Data relations graph: struct <-> module access
    data_nodes = []
    data_links = []
    node_id_map = {}  # name -> id

    # Add struct nodes with category info
    for idx, s in enumerate(interface_scanner.structs):
        node_id = 'struct_' + s['name']
        node_id_map[node_id] = len(data_nodes)

        # Get category from data_focus config
        category = data_focus.get_category(s['name']) if data_focus else 'unknown'

        # Skip if show_only_focused and not in focused categories
        if data_focus and data_focus.show_only_focused:
            if not data_focus.is_focused(s['name']):
                continue

        data_nodes.append({
            'id': node_id,
            'name': s['name'],
            'type': 'struct',
            'fields': s['field_count'],
            'module': s['module'],
            'category': category,
        })

    # Collect all modules that access structs
    accessing_modules = set()
    for struct_name, usage in interface_scanner.struct_usage.items():
        for file_path in usage.keys():
            module = os.path.dirname(file_path) or '.'
            accessing_modules.add(module)

    # Add module nodes
    for mod_name in accessing_modules:
        node_id = 'module_' + mod_name
        if node_id not in node_id_map:
            node_id_map[node_id] = len(data_nodes)
            # Get layer for this module
            mod_layer = None
            if mod_name in module_analyzer.modules:
                mod_layer = module_analyzer.modules[mod_name].get('layer')
            data_nodes.append({
                'id': node_id,
                'name': mod_name,
                'type': 'module',
                'layer': mod_layer,
            })

    # Add links (module -> struct for read, module -> struct for write)
    for struct_name, usage in interface_scanner.struct_usage.items():
        struct_id = 'struct_' + struct_name
        if struct_id not in node_id_map:
            continue

        # Aggregate by module
        module_access = defaultdict(lambda: {'reads': 0, 'writes': 0})
        for file_path, access in usage.items():
            module = os.path.dirname(file_path) or '.'
            module_access[module]['reads'] += access.get('reads', 0)
            module_access[module]['writes'] += access.get('writes', 0)

        for mod_name, access in module_access.items():
            mod_id = 'module_' + mod_name
            if mod_id not in node_id_map:
                continue

            if access['writes'] > 0:
                data_links.append({
                    'source': mod_id,
                    'target': struct_id,
                    'type': 'write',
                    'count': access['writes'],
                })
            if access['reads'] > 0:
                data_links.append({
                    'source': mod_id,
                    'target': struct_id,
                    'type': 'read',
                    'count': access['reads'],
                })

    # Generate HTML
    html = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Codebase Review Report</title>
    <style>
        * {{
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: #0f0f1a;
            color: #e0e0e0;
            line-height: 1.6;
        }}
        .container {{
            max-width: 1600px;
            margin: 0 auto;
            padding: 20px;
        }}
        header {{
            text-align: center;
            padding: 40px 0;
            border-bottom: 2px solid #2a2a4a;
            margin-bottom: 30px;
            background: linear-gradient(180deg, #1a1a2e 0%, #0f0f1a 100%);
        }}
        header h1 {{
            color: #00d4ff;
            font-size: 2.8em;
            margin-bottom: 10px;
            text-shadow: 0 0 20px rgba(0, 212, 255, 0.3);
        }}
        header .subtitle {{
            color: #888;
            font-size: 1.1em;
        }}

        /* Navigation */
        .nav {{
            display: flex;
            justify-content: center;
            gap: 10px;
            margin-bottom: 30px;
            flex-wrap: wrap;
        }}
        .nav a {{
            background: #1a1a2e;
            color: #00d4ff;
            padding: 12px 24px;
            border-radius: 8px;
            text-decoration: none;
            border: 1px solid #2a2a4a;
            transition: all 0.2s;
        }}
        .nav a:hover {{
            background: #2a2a4a;
            transform: translateY(-2px);
        }}

        /* Stats Grid */
        .stats-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 20px;
            margin-bottom: 40px;
        }}
        .stat-card {{
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            border-radius: 12px;
            padding: 25px;
            text-align: center;
            border: 1px solid #2a2a4a;
            transition: transform 0.2s;
        }}
        .stat-card:hover {{
            transform: translateY(-3px);
        }}
        .stat-card .value {{
            font-size: 2.5em;
            font-weight: bold;
            color: #00d4ff;
        }}
        .stat-card .label {{
            color: #888;
            margin-top: 5px;
            font-size: 0.95em;
        }}
        .stat-card.warning .value {{
            color: #ff9800;
        }}
        .stat-card.success .value {{
            color: #4caf50;
        }}

        /* Section */
        .section {{
            background: #1a1a2e;
            border-radius: 12px;
            padding: 30px;
            margin-bottom: 30px;
            border: 1px solid #2a2a4a;
        }}
        .section h2 {{
            color: #00d4ff;
            margin-bottom: 20px;
            padding-bottom: 15px;
            border-bottom: 1px solid #2a2a4a;
            display: flex;
            align-items: center;
            gap: 10px;
        }}
        .section h2 .icon {{
            font-size: 1.2em;
        }}
        .section h3 {{
            color: #aaa;
            margin: 20px 0 15px 0;
            font-size: 1.1em;
        }}

        /* Tables */
        table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 15px;
        }}
        th, td {{
            padding: 12px 15px;
            text-align: left;
            border-bottom: 1px solid #2a2a4a;
        }}
        th {{
            color: #00d4ff;
            font-weight: 600;
            background: #0f0f1a;
        }}
        tr:hover {{
            background: rgba(0, 212, 255, 0.05);
        }}
        .mono {{
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 0.9em;
        }}

        /* Bars */
        .bar-container {{
            background: #2a2a4a;
            border-radius: 4px;
            overflow: hidden;
            width: 150px;
            height: 20px;
        }}
        .bar {{
            height: 100%;
            border-radius: 4px;
            transition: width 0.3s;
        }}
        .bar.blue {{ background: linear-gradient(90deg, #00d4ff, #0099cc); }}
        .bar.green {{ background: linear-gradient(90deg, #4caf50, #2e7d32); }}
        .bar.orange {{ background: linear-gradient(90deg, #ff9800, #f57c00); }}
        .bar.red {{ background: linear-gradient(90deg, #f44336, #c62828); }}

        /* Graph Container */
        .graph-container {{
            width: 100%;
            height: 500px;
            background: #ffffff;
            border-radius: 8px;
            overflow: hidden;
            position: relative;
        }}
        .graph-controls {{
            position: absolute;
            top: 10px;
            left: 10px;
            z-index: 10;
            display: flex;
            gap: 8px;
        }}
        .graph-controls button {{
            background: #f0f0f0;
            color: #333;
            border: 1px solid #ccc;
            padding: 6px 12px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 12px;
        }}
        .graph-controls button:hover {{
            background: #e0e0e0;
        }}

        /* Tabs */
        .tabs {{
            display: flex;
            gap: 5px;
            margin-bottom: 20px;
            border-bottom: 1px solid #2a2a4a;
            padding-bottom: 10px;
        }}
        .tab {{
            background: transparent;
            border: none;
            color: #888;
            padding: 10px 20px;
            cursor: pointer;
            border-radius: 6px 6px 0 0;
            transition: all 0.2s;
        }}
        .tab:hover {{
            color: #00d4ff;
        }}
        .tab.active {{
            background: #2a2a4a;
            color: #00d4ff;
        }}
        .tab-content {{
            display: none;
        }}
        .tab-content.active {{
            display: block;
        }}

        /* Cards */
        .card-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
        }}
        .card {{
            background: #0f0f1a;
            border-radius: 8px;
            padding: 20px;
            border: 1px solid #2a2a4a;
        }}
        .card h4 {{
            color: #00d4ff;
            margin-bottom: 10px;
        }}
        .card .path {{
            color: #666;
            font-size: 0.85em;
            margin-bottom: 10px;
        }}
        .card ul {{
            list-style: none;
            padding: 0;
        }}
        .card li {{
            padding: 4px 0;
            color: #aaa;
            font-family: monospace;
            font-size: 0.9em;
        }}

        /* Badges */
        .badge {{
            display: inline-block;
            padding: 3px 10px;
            border-radius: 12px;
            font-size: 0.8em;
            font-weight: 600;
        }}
        .badge.blue {{ background: #00d4ff22; color: #00d4ff; }}
        .badge.green {{ background: #4caf5022; color: #4caf50; }}
        .badge.orange {{ background: #ff980022; color: #ff9800; }}
        .badge.red {{ background: #f4433622; color: #f44336; }}

        /* Complexity indicator */
        .complexity {{
            display: inline-block;
            width: 30px;
            height: 30px;
            line-height: 30px;
            text-align: center;
            border-radius: 50%;
            font-weight: bold;
            font-size: 0.9em;
        }}
        .complexity.low {{ background: #4caf5022; color: #4caf50; }}
        .complexity.medium {{ background: #ff980022; color: #ff9800; }}
        .complexity.high {{ background: #f4433622; color: #f44336; }}

        /* Tooltip */
        .tooltip {{
            position: absolute;
            background: #1a1a2e;
            border: 1px solid #00d4ff;
            border-radius: 6px;
            padding: 12px;
            pointer-events: none;
            font-size: 12px;
            max-width: 300px;
            z-index: 100;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        }}

        /* Layer colors */
        .layer-presentation {{ color: #F44336; }}
        .layer-application {{ color: #2196F3; }}
        .layer-core {{ color: #4CAF50; }}
        .layer-infrastructure {{ color: #FF9800; }}

        /* Footer */
        footer {{
            text-align: center;
            padding: 30px;
            color: #666;
            border-top: 1px solid #2a2a4a;
            margin-top: 40px;
        }}

        /* Scrollable table wrapper */
        .table-wrapper {{
            max-height: 400px;
            overflow-y: auto;
        }}
        .table-wrapper::-webkit-scrollbar {{
            width: 8px;
        }}
        .table-wrapper::-webkit-scrollbar-track {{
            background: #1a1a2e;
        }}
        .table-wrapper::-webkit-scrollbar-thumb {{
            background: #2a2a4a;
            border-radius: 4px;
        }}
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Codebase Review Report</h1>
            <div class="subtitle">
                Project: {project_path}<br>
                Generated: {timestamp}
            </div>
        </header>

        <nav class="nav">
            <a href="#overview">Overview</a>
            <a href="#modules">Modules</a>
            <a href="#interfaces">Interfaces</a>
            <a href="#callgraph">Call Graph</a>
            <a href="#procedures">Procedures</a>
        </nav>

        <!-- Overview Section -->
        <section id="overview" class="section">
            <h2><span class="icon">&#128202;</span> Overview</h2>
            <div class="stats-grid">
                <div class="stat-card">
                    <div class="value">{total_files}</div>
                    <div class="label">Total Files</div>
                </div>
                <div class="stat-card">
                    <div class="value">{total_lines:,}</div>
                    <div class="label">Lines of Code</div>
                </div>
                <div class="stat-card">
                    <div class="value">{total_modules}</div>
                    <div class="label">Modules</div>
                </div>
                <div class="stat-card">
                    <div class="value">{total_functions}</div>
                    <div class="label">Functions</div>
                </div>
                <div class="stat-card">
                    <div class="value">{total_structs}</div>
                    <div class="label">Structs/Unions</div>
                </div>
                <div class="stat-card">
                    <div class="value">{total_enums}</div>
                    <div class="label">Enums</div>
                </div>
                <div class="stat-card {complexity_class}">
                    <div class="value">{avg_complexity:.1f}</div>
                    <div class="label">Avg Complexity</div>
                </div>
                <div class="stat-card">
                    <div class="value">{total_deps}</div>
                    <div class="label">Dependencies</div>
                </div>
            </div>

            <!-- Layer Distribution -->
            <h3>Clean Architecture Layers</h3>
            <div class="stats-grid" style="grid-template-columns: repeat(4, 1fr);">
                {layer_cards}
            </div>
        </section>

        <!-- Modules Section -->
        <section id="modules" class="section">
            <h2><span class="icon">&#128193;</span> Modules & Architecture</h2>
            <p style="color: #888; margin-bottom: 20px;">
                Module structure analysis showing directory organization, file counts, and inter-module dependencies.
            </p>

            <div class="graph-container" id="module-graph">
                <div class="graph-controls">
                    <button onclick="resetModuleZoom()">Reset View</button>
                </div>
            </div>

            <h3>Module Details</h3>
            <div class="table-wrapper">
                <table>
                    <thead>
                        <tr>
                            <th>Module</th>
                            <th>Layer</th>
                            <th>Files</th>
                            <th>Lines</th>
                            <th>Dependencies</th>
                            <th>Dependents</th>
                        </tr>
                    </thead>
                    <tbody>
                        {module_rows}
                    </tbody>
                </table>
            </div>
        </section>

        <!-- Interfaces Section -->
        <section id="interfaces" class="section">
            <h2><span class="icon">&#128221;</span> Interfaces & Data Structures</h2>

            <div class="tabs">
                <button class="tab active" onclick="showInterfaceTab('data-relations')">Data Relations</button>
                <button class="tab" onclick="showInterfaceTab('structs')">Structs ({struct_count})</button>
                <button class="tab" onclick="showInterfaceTab('enums')">Enums ({enum_count})</button>
                <button class="tab" onclick="showInterfaceTab('funcs')">Functions ({func_count})</button>
                <button class="tab" onclick="showInterfaceTab('macros')">Macros ({macro_count})</button>
            </div>

            <div id="data-relations" class="tab-content active">
                <p style="color: #888; margin-bottom: 15px;">
                    Data structure access patterns: which modules read/write each struct.
                    <span class="badge green">Read</span> <span class="badge orange">Write</span> <span class="badge blue">Reference</span>
                </p>
                <div class="graph-container" id="data-graph" style="height: 500px;">
                    <div class="graph-controls">
                        <button onclick="resetDataZoom()">Reset View</button>
                    </div>
                </div>
                <h3 style="margin-top: 20px;">Struct Access Details</h3>
                <div class="table-wrapper">
                    <table>
                        <thead>
                            <tr>
                                <th>Struct</th>
                                <th>Defined In</th>
                                <th>Readers</th>
                                <th>Writers</th>
                                <th>Modules</th>
                            </tr>
                        </thead>
                        <tbody>
                            {struct_access_rows}
                        </tbody>
                    </table>
                </div>
            </div>

            <div id="structs" class="tab-content">
                <div class="card-grid">
                    {struct_cards}
                </div>
            </div>

            <div id="enums" class="tab-content">
                <div class="card-grid">
                    {enum_cards}
                </div>
            </div>

            <div id="funcs" class="tab-content">
                <div class="table-wrapper">
                    <table>
                        <thead>
                            <tr>
                                <th>Function</th>
                                <th>Return Type</th>
                                <th>Parameters</th>
                                <th>File</th>
                            </tr>
                        </thead>
                        <tbody>
                            {func_rows}
                        </tbody>
                    </table>
                </div>
            </div>

            <div id="macros" class="tab-content">
                <div class="table-wrapper">
                    <table>
                        <thead>
                            <tr>
                                <th>Macro</th>
                                <th>Value</th>
                                <th>File</th>
                            </tr>
                        </thead>
                        <tbody>
                            {macro_rows}
                        </tbody>
                    </table>
                </div>
            </div>
        </section>

        <!-- Call Graph Section -->
        <section id="callgraph" class="section">
            <h2><span class="icon">&#128279;</span> Call Graph & Pipelines</h2>
            <p style="color: #888; margin-bottom: 20px;">
                Function call relationships showing entry points, call chains, and function dependencies.
            </p>

            <div class="stats-grid" style="grid-template-columns: repeat(4, 1fr); margin-bottom: 20px;">
                <div class="stat-card">
                    <div class="value">{cg_functions}</div>
                    <div class="label">Functions</div>
                </div>
                <div class="stat-card">
                    <div class="value">{cg_calls}</div>
                    <div class="label">Call Edges</div>
                </div>
                <div class="stat-card success">
                    <div class="value">{cg_entry_points}</div>
                    <div class="label">Entry Points</div>
                </div>
                <div class="stat-card">
                    <div class="value">{cg_leaf_funcs}</div>
                    <div class="label">Leaf Functions</div>
                </div>
            </div>

            <div class="graph-container" id="call-graph" style="height: 600px;">
                <div class="graph-controls">
                    <button onclick="toggleCallView()" id="call-view-btn">Show Functions</button>
                    <button onclick="resetCallZoom()">Reset View</button>
                    <button onclick="toggleCallLabels()">Toggle Labels</button>
                </div>
            </div>

            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px;">
                <div>
                    <h3>Most Called Functions</h3>
                    <table>
                        <thead>
                            <tr><th>Function</th><th>Called By</th><th></th></tr>
                        </thead>
                        <tbody>
                            {most_called_rows}
                        </tbody>
                    </table>
                </div>
                <div>
                    <h3>Functions With Most Calls</h3>
                    <table>
                        <thead>
                            <tr><th>Function</th><th>Calls</th><th></th></tr>
                        </thead>
                        <tbody>
                            {most_calling_rows}
                        </tbody>
                    </table>
                </div>
            </div>
        </section>

        <!-- Procedures Section -->
        <section id="procedures" class="section">
            <h2><span class="icon">&#9881;</span> Procedures & Complexity</h2>
            <p style="color: #888; margin-bottom: 20px;">
                Function complexity analysis using cyclomatic complexity and size metrics.
            </p>

            <div class="stats-grid" style="grid-template-columns: repeat(5, 1fr); margin-bottom: 20px;">
                <div class="stat-card">
                    <div class="value">{proc_total}</div>
                    <div class="label">Total Functions</div>
                </div>
                <div class="stat-card">
                    <div class="value">{proc_avg_lines:.0f}</div>
                    <div class="label">Avg Lines/Func</div>
                </div>
                <div class="stat-card {complexity_class}">
                    <div class="value">{proc_avg_cc:.1f}</div>
                    <div class="label">Avg Complexity</div>
                </div>
                <div class="stat-card warning">
                    <div class="value">{proc_max_cc}</div>
                    <div class="label">Max Complexity</div>
                </div>
                <div class="stat-card">
                    <div class="value">{proc_max_lines}</div>
                    <div class="label">Max Lines</div>
                </div>
            </div>

            <h3>Functions by Complexity (Top 50)</h3>
            <div class="table-wrapper">
                <table>
                    <thead>
                        <tr>
                            <th>CC</th>
                            <th>Function</th>
                            <th>File</th>
                            <th>Lines</th>
                            <th>If/Else</th>
                            <th>Loops</th>
                            <th>Switch/Case</th>
                        </tr>
                    </thead>
                    <tbody>
                        {procedure_rows}
                    </tbody>
                </table>
            </div>
        </section>

        <footer>
            Generated by Codebase Reviewer | Python {python_version}
        </footer>
    </div>

    <div id="tooltip" class="tooltip" style="display: none;"></div>

    {d3_script_tag}
    <script>
        // Data
        const modules = {modules_json};
        const callNodes = {call_nodes_json};
        const callLinks = {call_links_json};
        const moduleCallNodes = {module_call_nodes_json};
        const moduleCallLinks = {module_call_links_json};
        const caLayers = {ca_layers_json};
        let showModuleView = true;  // Module-level view by default

        // Layer colors (used by multiple graphs)
        const layerColors = {{}};
        caLayers.forEach(l => layerColors[l.name] = l.color);

        // =====================================================================
        // Module Graph
        // =====================================================================
        const modContainer = document.getElementById('module-graph');
        if (modContainer && modules.length > 0) {{
            const modWidth = modContainer.clientWidth;
            const modHeight = modContainer.clientHeight;

            const modSvg = d3.select('#module-graph')
                .append('svg')
                .attr('width', modWidth)
                .attr('height', modHeight);

            const modDefs = modSvg.append('defs');

            // Arrow marker
            modDefs.append('marker')
                .attr('id', 'mod-arrow')
                .attr('viewBox', '0 -5 10 10')
                .attr('refX', 20)
                .attr('refY', 0)
                .attr('markerWidth', 6)
                .attr('markerHeight', 6)
                .attr('orient', 'auto')
                .append('path')
                .attr('d', 'M0,-4L10,0L0,4')
                .attr('fill', '#666');

            const modG = modSvg.append('g');

            const modZoom = d3.zoom()
                .scaleExtent([0.2, 3])
                .on('zoom', (event) => modG.attr('transform', event.transform));
            modSvg.call(modZoom);

            window.resetModuleZoom = function() {{
                modSvg.transition().duration(500).call(modZoom.transform, d3.zoomIdentity);
            }};

            // Build module links
            const modLinks = [];
            const modNodeMap = new Map(modules.map(m => [m.name, m]));

            modules.forEach(m => {{
                m.deps.forEach(dep => {{
                    if (modNodeMap.has(dep)) {{
                        modLinks.push({{ source: m.name, target: dep }});
                    }}
                }});
            }});

            // Force simulation
            const modSimulation = d3.forceSimulation(modules)
                .force('link', d3.forceLink(modLinks).id(d => d.name).distance(120))
                .force('charge', d3.forceManyBody().strength(-400))
                .force('center', d3.forceCenter(modWidth / 2, modHeight / 2))
                .force('collision', d3.forceCollide().radius(50));

            // Draw links
            const modLink = modG.append('g')
                .selectAll('line')
                .data(modLinks)
                .join('line')
                .attr('stroke', '#666')
                .attr('stroke-width', 1.5)
                .attr('marker-end', 'url(#mod-arrow)');

            // Draw nodes
            const modNode = modG.append('g')
                .selectAll('g')
                .data(modules)
                .join('g')
                .call(d3.drag()
                    .on('start', (event, d) => {{
                        if (!event.active) modSimulation.alphaTarget(0.3).restart();
                        d.fx = d.x;
                        d.fy = d.y;
                    }})
                    .on('drag', (event, d) => {{
                        d.fx = event.x;
                        d.fy = event.y;
                    }})
                    .on('end', (event, d) => {{
                        if (!event.active) modSimulation.alphaTarget(0);
                        d.fx = null;
                        d.fy = null;
                    }}));

            modNode.append('circle')
                .attr('r', d => Math.min(40, 15 + Math.sqrt(d.lines) / 5))
                .attr('fill', d => layerColors[d.layer] || '#888')
                .attr('fill-opacity', 0.7)
                .attr('stroke', d => layerColors[d.layer] || '#888')
                .attr('stroke-width', 2);

            modNode.append('text')
                .text(d => d.name.length > 12 ? d.name.slice(0, 10) + '..' : d.name)
                .attr('text-anchor', 'middle')
                .attr('dominant-baseline', 'central')
                .attr('font-size', '11px')
                .attr('fill', '#333')
                .attr('font-weight', 'bold');

            const tooltip = document.getElementById('tooltip');
            modNode.on('mouseover', (event, d) => {{
                tooltip.innerHTML = `
                    <strong>${{d.name}}</strong><br>
                    <span style="color: ${{layerColors[d.layer] || '#888'}}">${{d.layer || 'Unknown'}}</span><br>
                    Files: ${{d.files}} (${{d.headers}}h / ${{d.sources}}c)<br>
                    Lines: ${{d.lines.toLocaleString()}}<br>
                    Depends on: ${{d.deps.length}} modules<br>
                    Used by: ${{d.dependents.length}} modules
                `;
                tooltip.style.display = 'block';
                tooltip.style.left = (event.pageX + 10) + 'px';
                tooltip.style.top = (event.pageY - 10) + 'px';
            }})
            .on('mouseout', () => {{
                tooltip.style.display = 'none';
            }});

            modSimulation.on('tick', () => {{
                modLink
                    .attr('x1', d => d.source.x)
                    .attr('y1', d => d.source.y)
                    .attr('x2', d => d.target.x)
                    .attr('y2', d => d.target.y);

                modNode.attr('transform', d => `translate(${{d.x}},${{d.y}})`);
            }});
        }}

        // =====================================================================
        // Call Graph - Doxygen Style Layered Layout (Module + Function views)
        // =====================================================================
        let showCallLabels = true;
        const cgContainer = document.getElementById('call-graph');
        let cgSvg, cgG, cgZoom;

        if (cgContainer && (callNodes.length > 0 || moduleCallNodes.length > 0)) {{
            const cgWidth = cgContainer.clientWidth;
            const cgHeight = cgContainer.clientHeight;

            cgSvg = d3.select('#call-graph')
                .append('svg')
                .attr('width', cgWidth)
                .attr('height', cgHeight);

            const cgDefs = cgSvg.append('defs');

            // Doxygen-style gradients
            const cgGradients = {{
                'entry': ['#d0ffd0', '#90ee90', '#228b22'],    // Green - entry points
                'leaf': ['#bfdfff', '#a8c8e8', '#84b0c7'],     // Blue - leaf functions
                'normal': ['#ffffd0', '#f0e68c', '#c0a000'],   // Yellow - normal functions
                'module': ['#e8d0ff', '#d0a8e8', '#9060c0'],   // Purple - modules
                'highlight': ['#ffd0d0', '#ffa0a0', '#cc6666'], // Red - highlight
            }};

            Object.entries(cgGradients).forEach(([name, colors]) => {{
                const grad = cgDefs.append('linearGradient')
                    .attr('id', `cg-gradient-${{name}}`)
                    .attr('x1', '0%').attr('y1', '0%')
                    .attr('x2', '0%').attr('y2', '100%');
                grad.append('stop').attr('offset', '0%').attr('stop-color', colors[0]);
                grad.append('stop').attr('offset', '100%').attr('stop-color', colors[1]);
            }});

            // Arrow marker
            cgDefs.append('marker')
                .attr('id', 'cg-arrow')
                .attr('viewBox', '0 -5 10 10')
                .attr('refX', 10)
                .attr('refY', 0)
                .attr('markerWidth', 8)
                .attr('markerHeight', 8)
                .attr('orient', 'auto')
                .append('path')
                .attr('d', 'M0,-4L10,0L0,4')
                .attr('fill', '#606060');

            cgG = cgSvg.append('g');

            cgZoom = d3.zoom()
                .scaleExtent([0.1, 4])
                .on('zoom', (event) => cgG.attr('transform', event.transform));
            cgSvg.call(cgZoom);

            window.resetCallZoom = function() {{
                cgSvg.transition().duration(500).call(cgZoom.transform, d3.zoomIdentity);
            }};

            window.toggleCallLabels = function() {{
                showCallLabels = !showCallLabels;
                cgG.selectAll('.cg-label').style('display', showCallLabels ? 'block' : 'none');
            }};

            // Toggle between module and function views
            window.toggleCallView = function() {{
                showModuleView = !showModuleView;
                document.getElementById('call-view-btn').textContent = showModuleView ? 'Show Functions' : 'Show Modules';
                renderCallGraph();
            }};

            // Function to render the call graph (module or function view)
            function renderCallGraph() {{
                cgG.selectAll('*').remove();

                const nodes = showModuleView ? moduleCallNodes : callNodes;
                const links = showModuleView ? moduleCallLinks : callLinks;

                if (nodes.length === 0) return;

                // Build adjacency lists for layer calculation
                const nodeById = new Map(nodes.map(n => [n.id, n]));
                const outgoing = new Map();
                const incoming = new Map();

                nodes.forEach(n => {{
                    outgoing.set(n.id, new Set());
                    incoming.set(n.id, new Set());
                }});

                links.forEach(l => {{
                    const srcId = typeof l.source === 'object' ? l.source.id : l.source;
                    const tgtId = typeof l.target === 'object' ? l.target.id : l.target;
                    if (outgoing.has(srcId)) outgoing.get(srcId).add(tgtId);
                    if (incoming.has(tgtId)) incoming.get(tgtId).add(srcId);
                }});

                // Calculate layers
                const layers = new Map();
                const visited = new Set();

                function calcLayer(nodeId) {{
                    if (layers.has(nodeId)) return layers.get(nodeId);
                    if (visited.has(nodeId)) return 0;
                    visited.add(nodeId);
                    const callees = outgoing.get(nodeId);
                    if (!callees || callees.size === 0) {{
                        layers.set(nodeId, 0);
                        return 0;
                    }}
                    let maxLayer = 0;
                    callees.forEach(id => {{
                        maxLayer = Math.max(maxLayer, calcLayer(id) + 1);
                    }});
                    layers.set(nodeId, maxLayer);
                    return maxLayer;
                }}

                nodes.forEach(n => calcLayer(n.id));

                // Group nodes by layer
                const layerGroups = new Map();
                let maxLayer = 0;

                nodes.forEach(n => {{
                    const layer = layers.get(n.id) || 0;
                    maxLayer = Math.max(maxLayer, layer);
                    if (!layerGroups.has(layer)) layerGroups.set(layer, []);
                    layerGroups.get(layer).push(n);
                }});

                // Node dimensions
                const nodeHeight = showModuleView ? 36 : 24;
                const nodeMinWidth = showModuleView ? 100 : 70;
                const nodePadding = 8;
                const layerSpacing = showModuleView ? 70 : 55;
                const nodeSpacingH = 15;
                const padding = 40;

                // Calculate node widths based on text
                const tempText = cgSvg.append('text')
                    .attr('font-family', 'Consolas, Monaco, monospace')
                    .attr('font-size', showModuleView ? '11px' : '10px');

                nodes.forEach(n => {{
                    const displayName = showModuleView ? n.name : n.name;
                    tempText.text(displayName);
                    n.textWidth = tempText.node().getComputedTextLength();
                    n.nodeWidth = Math.max(nodeMinWidth, n.textWidth + nodePadding * 2);
                }});
                tempText.remove();

                // Sort and position nodes
                layerGroups.forEach((nodesInLayer) => {{
                    nodesInLayer.sort((a, b) => a.name.localeCompare(b.name));
                }});

                const startY = padding;

                function positionLayer(layer) {{
                    const nodesInLayer = layerGroups.get(layer);
                    if (!nodesInLayer) return;

                    const y = startY + (maxLayer - layer) * layerSpacing;
                    const totalWidth = nodesInLayer.reduce((sum, n) => sum + n.nodeWidth, 0) + (nodesInLayer.length - 1) * nodeSpacingH;
                    let x = (cgWidth - totalWidth) / 2;

                    nodesInLayer.forEach(n => {{
                        n.x = x + n.nodeWidth / 2;
                        n.y = y;
                        n.graphDepth = layer;
                        x += n.nodeWidth + nodeSpacingH;
                    }});
                }}

                for (let layer = 0; layer <= maxLayer; layer++) {{
                    positionLayer(layer);
                }}

                // Barycenter crossing reduction
                for (let iter = 0; iter < 4; iter++) {{
                    for (let layer = maxLayer - 1; layer >= 0; layer--) {{
                        const nodesInLayer = layerGroups.get(layer);
                        if (!nodesInLayer || nodesInLayer.length < 2) continue;
                        nodesInLayer.forEach(n => {{
                            const neighbors = [];
                            outgoing.get(n.id).forEach(targetId => {{
                                const target = nodeById.get(targetId);
                                if (target && target.x !== undefined) neighbors.push(target.x);
                            }});
                            n.barycenter = neighbors.length > 0 ? neighbors.reduce((a, b) => a + b, 0) / neighbors.length : n.x;
                        }});
                        nodesInLayer.sort((a, b) => a.barycenter - b.barycenter);
                        positionLayer(layer);
                    }}
                    for (let layer = 1; layer <= maxLayer; layer++) {{
                        const nodesInLayer = layerGroups.get(layer);
                        if (!nodesInLayer || nodesInLayer.length < 2) continue;
                        nodesInLayer.forEach(n => {{
                            const neighbors = [];
                            incoming.get(n.id).forEach(sourceId => {{
                                const source = nodeById.get(sourceId);
                                if (source && source.x !== undefined) neighbors.push(source.x);
                            }});
                            n.barycenter = neighbors.length > 0 ? neighbors.reduce((a, b) => a + b, 0) / neighbors.length : n.x;
                        }});
                        nodesInLayer.sort((a, b) => a.barycenter - b.barycenter);
                        positionLayer(layer);
                    }}
                }}

                // Draw links
                const link = cgG.append('g')
                    .selectAll('path')
                    .data(links)
                    .join('path')
                    .attr('class', 'cg-link')
                    .attr('fill', 'none')
                    .attr('stroke', '#606060')
                    .attr('stroke-width', d => showModuleView && d.count ? Math.min(4, 1 + Math.log(d.count)) : 1)
                    .attr('marker-end', 'url(#cg-arrow)')
                    .attr('d', d => {{
                        const src = nodeById.get(typeof d.source === 'object' ? d.source.id : d.source);
                        const tgt = nodeById.get(typeof d.target === 'object' ? d.target.id : d.target);
                        if (!src || !tgt) return '';
                        const srcY = src.y + nodeHeight / 2;
                        const tgtY = tgt.y - nodeHeight / 2;
                        const controlOffset = Math.abs(tgtY - srcY) * 0.5;
                        return `M${{src.x}},${{srcY}} C${{src.x}},${{srcY + controlOffset}} ${{tgt.x}},${{tgtY - controlOffset}} ${{tgt.x}},${{tgtY}}`;
                    }});

                // Draw nodes
                const node = cgG.append('g')
                    .selectAll('g')
                    .data(nodes)
                    .join('g')
                    .attr('class', 'cg-node')
                    .attr('transform', d => `translate(${{d.x}},${{d.y}})`);

                function getNodeType(d) {{
                    if (showModuleView) return 'module';
                    if (d.called_by === 0) return 'entry';
                    if (d.calls === 0) return 'leaf';
                    return 'normal';
                }}

                node.append('rect')
                    .attr('x', d => -d.nodeWidth / 2)
                    .attr('y', -nodeHeight / 2)
                    .attr('width', d => d.nodeWidth)
                    .attr('height', nodeHeight)
                    .attr('rx', 3)
                    .attr('ry', 3)
                    .attr('fill', d => `url(#cg-gradient-${{getNodeType(d)}})`)
                    .attr('stroke', d => {{
                        const type = getNodeType(d);
                        return {{ module: '#9060c0', entry: '#228b22', leaf: '#84b0c7', normal: '#c0a000' }}[type];
                    }})
                    .attr('stroke-width', 1);

                node.append('text')
                    .attr('class', 'cg-label')
                    .attr('text-anchor', 'middle')
                    .attr('dominant-baseline', 'central')
                    .attr('font-family', 'Consolas, Monaco, monospace')
                    .attr('font-size', showModuleView ? '11px' : '10px')
                    .attr('font-weight', showModuleView ? 'bold' : 'normal')
                    .attr('fill', '#333')
                    .text(d => d.name);

                // Add function count badge for module view
                if (showModuleView) {{
                    node.append('text')
                        .attr('class', 'cg-label')
                        .attr('text-anchor', 'middle')
                        .attr('dominant-baseline', 'middle')
                        .attr('y', nodeHeight / 2 - 8)
                        .attr('font-family', 'Consolas, Monaco, monospace')
                        .attr('font-size', '9px')
                        .attr('fill', '#666')
                        .text(d => `${{d.func_count}} func${{d.func_count !== 1 ? 's' : ''}}`);
                }}

                // Tooltip and hover
                const tooltip = document.getElementById('tooltip');

                node.on('mouseover', (event, d) => {{
                    // Build connected set
                    const connected = new Set([d.id]);
                    links.forEach(l => {{
                        const srcId = typeof l.source === 'object' ? l.source.id : l.source;
                        const tgtId = typeof l.target === 'object' ? l.target.id : l.target;
                        if (srcId === d.id) connected.add(tgtId);
                        if (tgtId === d.id) connected.add(srcId);
                    }});

                    // Highlight current node
                    d3.select(event.currentTarget).select('rect')
                        .attr('stroke-width', 3)
                        .attr('stroke', '#000');

                    // Gray out unconnected
                    node.select('rect')
                        .attr('opacity', n => connected.has(n.id) ? 1 : 0.15)
                        .attr('fill', n => {{
                            if (!connected.has(n.id)) return '#d0d0d0';
                            return `url(#cg-gradient-${{getNodeType(n)}})`;
                        }});

                    node.selectAll('text')
                        .attr('opacity', n => connected.has(n.id) ? 1 : 0.2)
                        .attr('fill', n => connected.has(n.id) ? '#333' : '#999');

                    link
                        .attr('stroke', l => {{
                            const srcId = typeof l.source === 'object' ? l.source.id : l.source;
                            const tgtId = typeof l.target === 'object' ? l.target.id : l.target;
                            if (srcId === d.id) return '#ff9800';
                            if (tgtId === d.id) return '#4caf50';
                            return '#e0e0e0';
                        }})
                        .attr('stroke-width', l => {{
                            const srcId = typeof l.source === 'object' ? l.source.id : l.source;
                            const tgtId = typeof l.target === 'object' ? l.target.id : l.target;
                            if (srcId === d.id || tgtId === d.id) {{
                                return showModuleView && l.count ? Math.min(5, 2 + Math.log(l.count)) : 2;
                            }}
                            return 0.3;
                        }})
                        .attr('stroke-opacity', l => {{
                            const srcId = typeof l.source === 'object' ? l.source.id : l.source;
                            const tgtId = typeof l.target === 'object' ? l.target.id : l.target;
                            return (srcId === d.id || tgtId === d.id) ? 1 : 0.08;
                        }});

                    // Build tooltip
                    let html = '';
                    if (showModuleView) {{
                        const funcs = d.functions || [];
                        html = `<strong>${{d.name}}</strong><br>`;
                        html += `<span style="color:#666">Path: ${{d.path || 'root'}}</span><br>`;
                        html += `<span style="color:#9060c0">Functions: ${{d.func_count}}</span><br>`;
                        html += `<span style="color:#ff9800">Outgoing calls: ${{d.calls}}</span><br>`;
                        html += `<span style="color:#4caf50">Incoming calls: ${{d.called_by}}</span>`;
                        if (funcs.length > 0) {{
                            html += `<br><hr style="margin:4px 0;border-color:#ddd">`;
                            html += `<div style="max-height:200px;overflow-y:auto;font-size:11px">`;
                            funcs.slice(0, 20).forEach(f => {{
                                html += `<span style="color:#333">\u2022 ${{f}}</span><br>`;
                            }});
                            if (funcs.length > 20) {{
                                html += `<span style="color:#999">... and ${{funcs.length - 20}} more</span>`;
                            }}
                            html += `</div>`;
                        }}
                    }} else {{
                        const type = getNodeType(d);
                        const typeLabel = {{ entry: 'Entry Point', leaf: 'Leaf Function', normal: 'Function' }}[type];
                        html = `<strong>${{d.name}}</strong><br>`;
                        html += `<span style="color:#666">${{typeLabel}} | Depth: ${{d.graphDepth}}</span><br>`;
                        html += `<span style="color:#228b22">File: ${{d.file}}</span><br>`;
                        html += `<span style="color:#ff9800">Calls: ${{d.calls}} functions</span><br>`;
                        html += `<span style="color:#4caf50">Called by: ${{d.called_by}} functions</span>`;
                    }}

                    tooltip.innerHTML = html;
                    tooltip.style.display = 'block';
                    tooltip.style.left = (event.pageX + 10) + 'px';
                    tooltip.style.top = (event.pageY - 10) + 'px';
                }})
                .on('mouseout', (event, d) => {{
                    d3.select(event.currentTarget).select('rect')
                        .attr('stroke-width', 1)
                        .attr('stroke', dd => {{
                            const type = getNodeType(dd);
                            return {{ module: '#9060c0', entry: '#228b22', leaf: '#84b0c7', normal: '#c0a000' }}[type];
                        }});

                    // Reset all nodes
                    node.select('rect')
                        .attr('opacity', 1)
                        .attr('fill', n => `url(#cg-gradient-${{getNodeType(n)}})`);

                    node.selectAll('text')
                        .attr('opacity', 1)
                        .attr('fill', '#333');

                    // Reset links
                    link
                        .attr('stroke', '#606060')
                        .attr('stroke-width', l => showModuleView && l.count ? Math.min(4, 1 + Math.log(l.count)) : 1)
                        .attr('stroke-opacity', 1);

                    tooltip.style.display = 'none';
                }});
            }}

            // Initial render (module view by default)
            renderCallGraph();
        }}

        // =====================================================================
        // Data Relations Graph - Doxygen Style Layered Layout
        // =====================================================================
        const dataNodes = {data_nodes_json};
        const dataLinks = {data_links_json};
        const categoryColors = {category_colors_json};

        const dataContainer = document.getElementById('data-graph');
        if (dataContainer && dataNodes.length > 0) {{
            const dataWidth = dataContainer.clientWidth;
            const dataHeight = dataContainer.clientHeight;

            const dataSvg = d3.select('#data-graph')
                .append('svg')
                .attr('width', dataWidth)
                .attr('height', dataHeight);

            const dataDefs = dataSvg.append('defs');

            // Doxygen-style gradients for categories
            const gradients = {{
                'config': ['#bfdfff', '#a8c8e8', '#84b0c7'],     // Blue
                'algorithm': ['#d0ffd0', '#90ee90', '#228b22'], // Green
                'platform': ['#ffe4b3', '#ffc966', '#cc8800'],  // Orange
                'unknown': ['#e0e0e0', '#c0c0c0', '#909090'],   // Gray
                'module': ['#ffffd0', '#f0e68c', '#c0a000'],    // Yellow (for modules)
            }};

            Object.entries(gradients).forEach(([name, colors]) => {{
                const grad = dataDefs.append('linearGradient')
                    .attr('id', `data-gradient-${{name}}`)
                    .attr('x1', '0%').attr('y1', '0%')
                    .attr('x2', '0%').attr('y2', '100%');
                grad.append('stop').attr('offset', '0%').attr('stop-color', colors[0]);
                grad.append('stop').attr('offset', '100%').attr('stop-color', colors[1]);
            }});

            // Arrow markers
            dataDefs.append('marker')
                .attr('id', 'data-read-arrow')
                .attr('viewBox', '0 -5 10 10')
                .attr('refX', 10)
                .attr('refY', 0)
                .attr('markerWidth', 8)
                .attr('markerHeight', 8)
                .attr('orient', 'auto')
                .append('path')
                .attr('d', 'M0,-4L10,0L0,4')
                .attr('fill', '#4caf50');

            dataDefs.append('marker')
                .attr('id', 'data-write-arrow')
                .attr('viewBox', '0 -5 10 10')
                .attr('refX', 10)
                .attr('refY', 0)
                .attr('markerWidth', 8)
                .attr('markerHeight', 8)
                .attr('orient', 'auto')
                .append('path')
                .attr('d', 'M0,-4L10,0L0,4')
                .attr('fill', '#ff9800');

            const dataG = dataSvg.append('g');

            // Zoom
            const dataZoom = d3.zoom()
                .scaleExtent([0.1, 4])
                .on('zoom', (event) => dataG.attr('transform', event.transform));
            dataSvg.call(dataZoom);

            window.resetDataZoom = function() {{
                dataSvg.transition().duration(500).call(dataZoom.transform, d3.zoomIdentity);
            }};

            // Separate nodes by type
            const moduleNodes = dataNodes.filter(n => n.type === 'module');
            const structNodes = dataNodes.filter(n => n.type === 'struct');

            // Node dimensions
            const nodeHeight = 26;
            const nodeMinWidth = 80;
            const nodePadding = 10;
            const nodeSpacingH = 15;
            const layerSpacing = 120;
            const padding = 40;

            // Calculate node widths based on text
            const tempText = dataSvg.append('text')
                .attr('font-family', 'Consolas, Monaco, monospace')
                .attr('font-size', '11px');

            dataNodes.forEach(n => {{
                tempText.text(n.name);
                n.textWidth = tempText.node().getComputedTextLength();
                n.nodeWidth = Math.max(nodeMinWidth, n.textWidth + nodePadding * 2);
            }});
            tempText.remove();

            // Position modules (top row)
            moduleNodes.sort((a, b) => a.name.localeCompare(b.name));
            let totalModuleWidth = moduleNodes.reduce((sum, n) => sum + n.nodeWidth, 0) + (moduleNodes.length - 1) * nodeSpacingH;
            let moduleX = (dataWidth - totalModuleWidth) / 2;
            const moduleY = padding + nodeHeight;

            moduleNodes.forEach(n => {{
                n.x = moduleX + n.nodeWidth / 2;
                n.y = moduleY;
                moduleX += n.nodeWidth + nodeSpacingH;
            }});

            // Group structs by category and position (bottom rows)
            const structsByCategory = {{}};
            structNodes.forEach(n => {{
                const cat = n.category || 'unknown';
                if (!structsByCategory[cat]) structsByCategory[cat] = [];
                structsByCategory[cat].push(n);
            }});

            // Sort each category
            Object.values(structsByCategory).forEach(arr => arr.sort((a, b) => a.name.localeCompare(b.name)));

            // Position each category as a row
            const categoryOrder = ['config', 'algorithm', 'platform', 'unknown'];
            let structY = moduleY + layerSpacing;

            categoryOrder.forEach(cat => {{
                const nodesInCat = structsByCategory[cat] || [];
                if (nodesInCat.length === 0) return;

                const totalWidth = nodesInCat.reduce((sum, n) => sum + n.nodeWidth, 0) + (nodesInCat.length - 1) * nodeSpacingH;
                let x = (dataWidth - totalWidth) / 2;

                nodesInCat.forEach(n => {{
                    n.x = x + n.nodeWidth / 2;
                    n.y = structY;
                    x += n.nodeWidth + nodeSpacingH;
                }});

                structY += layerSpacing * 0.7;
            }});

            // Build node map for link drawing
            const nodeById = new Map(dataNodes.map(n => [n.id, n]));

            // Draw curved links (Doxygen style)
            const dataLink = dataG.append('g')
                .selectAll('path')
                .data(dataLinks)
                .join('path')
                .attr('fill', 'none')
                .attr('stroke', d => d.type === 'write' ? '#ff9800' : '#4caf50')
                .attr('stroke-width', d => Math.min(3, 1 + Math.log(d.count + 1)))
                .attr('stroke-dasharray', d => d.type === 'read' ? '5,3' : 'none')
                .attr('marker-end', d => d.type === 'write' ? 'url(#data-write-arrow)' : 'url(#data-read-arrow)')
                .attr('d', d => {{
                    const src = nodeById.get(d.source);
                    const tgt = nodeById.get(d.target);
                    if (!src || !tgt) return '';

                    const srcY = src.y + nodeHeight / 2;
                    const tgtY = tgt.y - nodeHeight / 2;
                    const controlOffset = Math.abs(tgtY - srcY) * 0.5;

                    return `M${{src.x}},${{srcY}} C${{src.x}},${{srcY + controlOffset}} ${{tgt.x}},${{tgtY - controlOffset}} ${{tgt.x}},${{tgtY}}`;
                }});

            // Draw nodes (Doxygen-style rectangles)
            const dataNode = dataG.append('g')
                .selectAll('g')
                .data(dataNodes)
                .join('g')
                .attr('class', 'data-node')
                .attr('transform', d => `translate(${{d.x}},${{d.y}})`);

            // Rectangle background
            dataNode.append('rect')
                .attr('x', d => -d.nodeWidth / 2)
                .attr('y', -nodeHeight / 2)
                .attr('width', d => d.nodeWidth)
                .attr('height', nodeHeight)
                .attr('rx', 2)
                .attr('ry', 2)
                .attr('fill', d => {{
                    if (d.type === 'module') return 'url(#data-gradient-module)';
                    return `url(#data-gradient-${{d.category || 'unknown'}})`;
                }})
                .attr('stroke', d => {{
                    if (d.type === 'module') return '#c0a000';
                    const colors = {{ config: '#84b0c7', algorithm: '#228b22', platform: '#cc8800', unknown: '#909090' }};
                    return colors[d.category] || '#909090';
                }})
                .attr('stroke-width', 1);

            // Node label
            dataNode.append('text')
                .attr('class', 'node-label')
                .attr('text-anchor', 'middle')
                .attr('dominant-baseline', 'central')
                .attr('font-family', 'Consolas, Monaco, monospace')
                .attr('font-size', '11px')
                .attr('fill', '#333')
                .text(d => d.name);

            // Tooltip
            const tooltip = document.getElementById('tooltip');
            tooltip.style.background = '#ffffcc';
            tooltip.style.border = '1px solid #333';
            tooltip.style.color = '#333';

            dataNode.on('mouseover', (event, d) => {{
                d3.select(event.currentTarget).select('rect')
                    .attr('stroke-width', 3)
                    .attr('stroke', '#000');

                // Build connected node set for efficient lookup
                const connectedNodes = new Set([d.id]);
                dataLinks.forEach(l => {{
                    if (l.source === d.id) connectedNodes.add(l.target);
                    if (l.target === d.id) connectedNodes.add(l.source);
                }});

                // Highlight only connected links, gray out others completely
                dataLink
                    .attr('stroke', l => {{
                        if (l.source === d.id || l.target === d.id) {{
                            return l.type === 'write' ? '#ff9800' : '#4caf50';
                        }}
                        return '#e0e0e0';
                    }})
                    .attr('stroke-width', l => {{
                        if (l.source === d.id || l.target === d.id) {{
                            return Math.min(5, 2 + Math.log(l.count + 1));
                        }}
                        return 0.3;
                    }})
                    .attr('stroke-opacity', l => {{
                        return (l.source === d.id || l.target === d.id) ? 1 : 0.08;
                    }});

                // Gray out unconnected nodes (both rect and text)
                dataNode.select('rect')
                    .attr('opacity', n => connectedNodes.has(n.id) ? 1 : 0.15)
                    .attr('fill', n => {{
                        if (!connectedNodes.has(n.id)) return '#d0d0d0';
                        // Return original fill
                        if (n.type === 'module') return '#fff8dc';
                        const fills = {{ config: '#e3f2fd', algorithm: '#e8f5e9', platform: '#fff3e0', unknown: '#f5f5f5' }};
                        return fills[n.category] || '#f5f5f5';
                    }});

                dataNode.select('text')
                    .attr('opacity', n => connectedNodes.has(n.id) ? 1 : 0.2)
                    .attr('fill', n => connectedNodes.has(n.id) ? '#333' : '#999');

                let info = `<strong>${{d.name}}</strong><br>`;
                if (d.type === 'struct') {{
                    // Count readers and writers
                    const readers = dataLinks.filter(l => l.target === d.id && l.type === 'read').length;
                    const writers = dataLinks.filter(l => l.target === d.id && l.type === 'write').length;
                    info += `Category: ${{d.category || 'unknown'}}<br>`;
                    info += `Fields: ${{d.fields || 0}}<br>`;
                    info += `<span style="color:#4caf50">Readers: ${{readers}}</span> | <span style="color:#ff9800">Writers: ${{writers}}</span>`;
                }} else {{
                    // Module: count what it reads/writes
                    const reads = dataLinks.filter(l => l.source === d.id && l.type === 'read').length;
                    const writes = dataLinks.filter(l => l.source === d.id && l.type === 'write').length;
                    info += `Type: Module<br>`;
                    info += `Layer: ${{d.layer || 'Unknown'}}<br>`;
                    info += `<span style="color:#4caf50">Reads: ${{reads}}</span> | <span style="color:#ff9800">Writes: ${{writes}}</span>`;
                }}

                tooltip.innerHTML = info;
                tooltip.style.display = 'block';
                tooltip.style.left = (event.pageX + 10) + 'px';
                tooltip.style.top = (event.pageY - 10) + 'px';
            }})
            .on('mouseout', (event, d) => {{
                d3.select(event.currentTarget).select('rect')
                    .attr('stroke-width', 1)
                    .attr('stroke', d => {{
                        if (d.type === 'module') return '#c0a000';
                        const colors = {{ config: '#84b0c7', algorithm: '#228b22', platform: '#cc8800', unknown: '#909090' }};
                        return colors[d.category] || '#909090';
                    }});

                // Reset all links to original state
                dataLink
                    .attr('stroke', l => l.type === 'write' ? '#ff9800' : '#4caf50')
                    .attr('stroke-width', l => Math.min(3, 1 + Math.log(l.count + 1)))
                    .attr('stroke-opacity', 1);

                // Reset all nodes (rect fill, opacity, and text)
                dataNode.select('rect')
                    .attr('opacity', 1)
                    .attr('fill', n => {{
                        if (n.type === 'module') return '#fff8dc';
                        const fills = {{ config: '#e3f2fd', algorithm: '#e8f5e9', platform: '#fff3e0', unknown: '#f5f5f5' }};
                        return fills[n.category] || '#f5f5f5';
                    }});

                dataNode.select('text')
                    .attr('opacity', 1)
                    .attr('fill', '#333');

                tooltip.style.display = 'none';
            }});
        }}

        // =====================================================================
        // Tab switching
        // =====================================================================
        function showInterfaceTab(tabId) {{
            document.querySelectorAll('#interfaces .tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('#interfaces .tab-content').forEach(c => c.classList.remove('active'));

            event.target.classList.add('active');
            document.getElementById(tabId).classList.add('active');
        }}
    </script>
</body>
</html>
'''

    # Generate dynamic content
    # Layer cards
    layer_cards = ''
    for layer in ca_layers:
        layer_name = layer['name']
        stats = layer_stats.get(layer_name, {'files': 0, 'lines': 0})
        layer_cards += '''
            <div class="stat-card" style="border-left: 4px solid {color};">
                <div class="value" style="color: {color};">{files}</div>
                <div class="label">{name}<br><small>{lines:,} lines</small></div>
            </div>
        '''.format(
            name=layer_name,
            color=layer['color'],
            files=stats['files'],
            lines=stats['lines'],
        )

    # Module rows
    module_rows = ''
    for mod in sorted(modules_data, key=lambda x: -x['lines']):
        layer_class = 'layer-' + (mod['layer'] or 'unknown').lower()
        module_rows += '''
            <tr>
                <td class="mono">{name}</td>
                <td><span class="{layer_class}">{layer}</span></td>
                <td>{files} ({headers}h/{sources}c)</td>
                <td>{lines:,}</td>
                <td>{deps}</td>
                <td>{dependents}</td>
            </tr>
        '''.format(
            name=mod['name'],
            layer=mod['layer'] or '-',
            layer_class=layer_class,
            files=mod['files'],
            headers=mod['headers'],
            sources=mod['sources'],
            lines=mod['lines'],
            deps=len(mod['deps']),
            dependents=len(mod['dependents']),
        )

    # Struct cards
    struct_cards = ''
    for s in interface_scanner.structs[:30]:
        fields_html = ''.join('<li>{}</li>'.format(f[:40]) for f in s['fields'][:5])
        if len(s['fields']) > 5:
            fields_html += '<li style="color:#666;">... +{} more</li>'.format(len(s['fields']) - 5)
        struct_cards += '''
            <div class="card">
                <h4>{name}</h4>
                <div class="path">{file}</div>
                <ul>{fields}</ul>
            </div>
        '''.format(name=s['name'], file=s['file'], fields=fields_html)

    # Struct access rows (data relations table)
    struct_access_rows = ''
    for s in interface_scanner.structs:
        accessors = interface_scanner.get_struct_accessors(s['name'])
        readers = accessors['readers']
        writers = accessors['writers']

        # Get unique modules
        reader_modules = set(r['module'] for r in readers)
        writer_modules = set(w['module'] for w in writers)
        all_modules = reader_modules | writer_modules

        # Format module badges
        modules_html = ''
        for mod in sorted(all_modules):
            is_reader = mod in reader_modules
            is_writer = mod in writer_modules
            if is_writer:
                modules_html += '<span class="badge orange" title="Writer">{}</span> '.format(mod)
            elif is_reader:
                modules_html += '<span class="badge green" title="Reader">{}</span> '.format(mod)

        struct_access_rows += '''
            <tr>
                <td class="mono">{name}</td>
                <td>{file}</td>
                <td>{readers}</td>
                <td>{writers}</td>
                <td>{modules}</td>
            </tr>
        '''.format(
            name=s['name'],
            file=s['module'],
            readers=len(readers),
            writers=len(writers),
            modules=modules_html if modules_html else '-',
        )

    # Enum cards
    enum_cards = ''
    for e in interface_scanner.enums[:30]:
        values_html = ''.join('<li>{}</li>'.format(v) for v in e['values'][:5])
        if len(e['values']) > 5:
            values_html += '<li style="color:#666;">... +{} more</li>'.format(len(e['values']) - 5)
        enum_cards += '''
            <div class="card">
                <h4>{name}</h4>
                <div class="path">{file}</div>
                <ul>{values}</ul>
            </div>
        '''.format(name=e['name'], file=e['file'], values=values_html)

    # Function rows
    func_rows = ''
    for f in interface_scanner.functions[:100]:
        params = f['params'][:50] + ('...' if len(f['params']) > 50 else '')
        func_rows += '''
            <tr>
                <td class="mono">{name}</td>
                <td class="mono">{return_type}</td>
                <td class="mono">{params}</td>
                <td>{file}</td>
            </tr>
        '''.format(
            name=f['name'],
            return_type=f['return_type'],
            params=params,
            file=f['file'],
        )

    # Macro rows
    macro_rows = ''
    for m in interface_scanner.macros[:100]:
        macro_rows += '''
            <tr>
                <td class="mono">{name}</td>
                <td class="mono">{value}</td>
                <td>{file}</td>
            </tr>
        '''.format(name=m['name'], value=m['value'], file=m['file'])

    # Call graph stats
    most_called = call_graph.get_most_called(10)
    max_called = most_called[0][1] if most_called else 1
    most_called_rows = ''
    for name, count in most_called:
        most_called_rows += '''
            <tr>
                <td class="mono">{}</td>
                <td>{}</td>
                <td><div class="bar-container"><div class="bar green" style="width: {}%;"></div></div></td>
            </tr>
        '''.format(name, count, int(count / max_called * 100))

    most_calling = call_graph.get_most_calling(10)
    max_calling = most_calling[0][1] if most_calling else 1
    most_calling_rows = ''
    for name, count in most_calling:
        most_calling_rows += '''
            <tr>
                <td class="mono">{}</td>
                <td>{}</td>
                <td><div class="bar-container"><div class="bar orange" style="width: {}%;"></div></div></td>
            </tr>
        '''.format(name, count, int(count / max_calling * 100))

    # Procedure rows
    procedure_rows = ''
    for p in procedures_data:
        cc = p['cyclomatic']
        if cc < 5:
            cc_class = 'low'
        elif cc < 10:
            cc_class = 'medium'
        else:
            cc_class = 'high'

        procedure_rows += '''
            <tr>
                <td><span class="complexity {cc_class}">{cc}</span></td>
                <td class="mono">{name}</td>
                <td>{file}</td>
                <td>{lines}</td>
                <td>{if_count}/{else_count}</td>
                <td>{for_count}/{while_count}</td>
                <td>{switch_count}/{case_count}</td>
            </tr>
        '''.format(
            cc=cc,
            cc_class=cc_class,
            name=p['name'],
            file=p['file'],
            lines=p['line_count'],
            if_count=p['if_count'],
            else_count=p['else_count'],
            for_count=p['for_count'],
            while_count=p['while_count'],
            switch_count=p['switch_count'],
            case_count=p['case_count'],
        )

    # Complexity class
    avg_cc = proc_stats['avg_complexity']
    if avg_cc < 5:
        complexity_class = 'success'
    elif avg_cc < 10:
        complexity_class = ''
    else:
        complexity_class = 'warning'

    # Format HTML
    html = html.format(
        project_path=scanner.root_path,
        timestamp=datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        total_files=dep_stats['total_files'],
        total_lines=dep_stats['total_lines'],
        total_modules=mod_stats['total_modules'],
        total_functions=call_stats['total_functions'],
        total_structs=interface_stats['structs'],
        total_enums=interface_stats['enums'],
        avg_complexity=avg_cc,
        complexity_class=complexity_class,
        total_deps=dep_stats['total_dependencies'],
        layer_cards=layer_cards,
        module_rows=module_rows,
        struct_count=interface_stats['structs'],
        enum_count=interface_stats['enums'],
        func_count=interface_stats['functions'],
        macro_count=interface_stats['macros'],
        struct_cards=struct_cards,
        struct_access_rows=struct_access_rows,
        enum_cards=enum_cards,
        func_rows=func_rows,
        macro_rows=macro_rows,
        data_nodes_json=json.dumps(data_nodes),
        data_links_json=json.dumps(data_links),
        category_colors_json=json.dumps(CATEGORY_COLORS),
        cg_functions=call_stats['total_functions'],
        cg_calls=call_stats['total_calls'],
        cg_entry_points=call_stats['entry_points'],
        cg_leaf_funcs=call_stats['leaf_functions'],
        most_called_rows=most_called_rows,
        most_calling_rows=most_calling_rows,
        proc_total=proc_stats['total_procedures'],
        proc_avg_lines=proc_stats['avg_lines'],
        proc_avg_cc=proc_stats['avg_complexity'],
        proc_max_cc=proc_stats['max_complexity'],
        proc_max_lines=proc_stats['max_lines'],
        procedure_rows=procedure_rows,
        modules_json=json.dumps(modules_data),
        call_nodes_json=json.dumps(call_nodes),
        call_links_json=json.dumps(call_links),
        module_call_nodes_json=json.dumps(module_call_nodes),
        module_call_links_json=json.dumps(module_call_links),
        ca_layers_json=json.dumps(ca_layers),
        d3_script_tag=get_d3_script_tag(),
        python_version='{}.{}.{}'.format(*sys.version_info[:3]),
    )

    # Write output
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(html)

    return output_path


# =============================================================================
# CLI Interface
# =============================================================================

def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Comprehensive C/C++ codebase review and analysis.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Example:
  %(prog)s /path/to/project

Output:
  review_report.html - Comprehensive codebase analysis including:
    - Module/architecture overview
    - Interface definitions (structs, enums, functions)
    - Call graph and pipeline analysis
    - Procedure complexity metrics

For dependency-only analysis, use:
  python3 cdep_analyzer.py /path/to/project
        '''
    )

    parser.add_argument(
        'project_path',
        help='Path to the project root directory'
    )

    parser.add_argument(
        '--version',
        action='version',
        version='%(prog)s 1.0.0 (Python {}.{}.{})'.format(*sys.version_info[:3])
    )

    args = parser.parse_args()

    # Validate project path
    if not os.path.isdir(args.project_path):
        print("Error: '{}' is not a valid directory".format(args.project_path))
        sys.exit(1)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = os.path.join(script_dir, 'ca_layers.json')
    data_focus_path = os.path.join(script_dir, 'data_focus.json')

    print("Codebase Reviewer - Comprehensive Analysis")
    print("=" * 60)
    print("Project: {}".format(os.path.abspath(args.project_path)))
    print()

    # Step 1: Dependency scanning (reuse cdep_analyzer)
    print("[1/5] Scanning files and dependencies...")
    scanner = DependencyScanner(
        args.project_path,
        exclude_dirs=DEFAULT_EXCLUDES
    )
    scanner.scan()

    dep_stats = scanner.get_stats()
    print("      Found {} files ({} headers, {} sources)".format(
        dep_stats['total_files'],
        dep_stats['header_files'],
        dep_stats['source_files']
    ))
    print("      Total lines: {:,}".format(dep_stats['total_lines']))

    # Step 2: Clean Architecture analysis
    print("[2/5] Analyzing Clean Architecture layers...")
    config_exists = os.path.exists(config_path)
    if config_exists:
        ca_analyzer = CleanArchAnalyzer(scanner, config_path=config_path)
    else:
        ca_analyzer = CleanArchAnalyzer(scanner)
    ca_analyzer.analyze()

    if not config_exists:
        ca_analyzer.generate_config_template(config_path)
        print("      Generated ca_layers.json (edit to customize)")

    layer_stats = ca_analyzer.get_layer_stats()
    for layer_name, stats in layer_stats.items():
        print("      {}: {} files".format(layer_name, stats['files']))

    # Step 3: Interface scanning
    print("[3/5] Scanning interfaces and data structures...")
    interface_scanner = InterfaceScanner(scanner)
    interface_scanner.scan()

    iface_stats = interface_scanner.get_stats()
    print("      Structs: {}, Enums: {}, Functions: {}, Macros: {}".format(
        iface_stats['structs'],
        iface_stats['enums'],
        iface_stats['functions'],
        iface_stats['macros']
    ))

    # Data focus configuration
    data_focus_exists = os.path.exists(data_focus_path)
    data_focus = DataFocusConfig(data_focus_path if data_focus_exists else None)

    if not data_focus_exists and interface_scanner.structs:
        data_focus.generate_config(interface_scanner.structs, data_focus_path)
        print("      Generated data_focus.json (edit to customize struct categories)")
        # Reload with the generated config
        data_focus = DataFocusConfig(data_focus_path)

    # Step 4: Call graph analysis
    print("[4/5] Building call graph...")
    call_graph = CallGraphAnalyzer(scanner)
    call_graph.scan()

    cg_stats = call_graph.get_stats()
    print("      Functions: {}, Calls: {}, Entry points: {}".format(
        cg_stats['total_functions'],
        cg_stats['total_calls'],
        cg_stats['entry_points']
    ))

    # Step 5: Procedure analysis
    print("[5/5] Analyzing procedure complexity...")
    procedure_analyzer = ProcedureAnalyzer(scanner)
    procedure_analyzer.scan()

    proc_stats = procedure_analyzer.get_stats()
    print("      Procedures: {}, Avg complexity: {:.1f}, Max: {}".format(
        proc_stats['total_procedures'],
        proc_stats['avg_complexity'],
        proc_stats['max_complexity']
    ))

    # Module analysis
    module_analyzer = ModuleAnalyzer(scanner, ca_analyzer)
    module_analyzer.analyze()

    # Generate report
    print()
    print("Generating HTML report...")
    output_file = os.path.join(script_dir, 'review_report.html')

    output_path = generate_review_report(
        scanner,
        interface_scanner,
        call_graph,
        procedure_analyzer,
        module_analyzer,
        ca_analyzer,
        data_focus,
        output_file
    )

    print("Report saved to: {}".format(os.path.abspath(output_path)))
    print()
    print("Open the HTML file in a browser to view the interactive report.")

    return 0


if __name__ == '__main__':
    sys.exit(main())

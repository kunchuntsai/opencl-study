#!/usr/bin/env python3
"""
C/C++ Code Style Scanner - Google Naming Convention Checker

Scans C/C++ source files (.c, .h, .cpp, .hpp) for naming convention
violations based on Google C++ Style Guide.

Google C++ Style Guide Naming Conventions:
  - Functions: PascalCase (e.g., AddTableEntry, OpenFile)
  - Variables: snake_case (e.g., table_name, num_entries)
  - Constants: kCamelCase (e.g., kDaysInWeek) or UPPER_SNAKE_CASE for macros
  - Types (struct, class, enum, typedef): PascalCase (e.g., UrlTable)

Usage:
    python3 func_scanning.py /path/to/project
    python3 func_scanning.py /path/to/project --fix
    python3 func_scanning.py /path/to/project --fix --dry-run

Compatible with Python 3.6.3+
"""

from __future__ import print_function

import argparse
import os
import re
import sys
from collections import defaultdict

# Version check
if sys.version_info < (3, 6):
    print("Error: Python 3.6 or higher is required")
    sys.exit(1)


# =============================================================================
# Configuration
# =============================================================================

# File extensions to scan
C_EXTENSIONS = {'.c', '.h'}
CPP_EXTENSIONS = {'.cpp', '.hpp', '.cc', '.hh', '.cxx', '.hxx'}
ALL_EXTENSIONS = C_EXTENSIONS | CPP_EXTENSIONS

# Directories to exclude
DEFAULT_EXCLUDES = {
    'build', 'cmake-build-debug', 'cmake-build-release',
    '.git', '.svn', '.hg',
    'third_party', 'external', 'vendor', 'deps',
    '__pycache__', 'node_modules',
}

# C/C++ keywords to ignore
C_KEYWORDS = {
    'auto', 'break', 'case', 'char', 'const', 'continue', 'default', 'do',
    'double', 'else', 'enum', 'extern', 'float', 'for', 'goto', 'if',
    'inline', 'int', 'long', 'register', 'restrict', 'return', 'short',
    'signed', 'sizeof', 'static', 'struct', 'switch', 'typedef', 'union',
    'unsigned', 'void', 'volatile', 'while', '_Bool', '_Complex', '_Imaginary',
    'bool', 'true', 'false', 'NULL', 'size_t', 'ssize_t', 'ptrdiff_t',
    'int8_t', 'int16_t', 'int32_t', 'int64_t',
    'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t',
    'intptr_t', 'uintptr_t', 'intmax_t', 'uintmax_t',
    # C++ keywords
    'class', 'namespace', 'template', 'typename', 'this', 'new', 'delete',
    'public', 'private', 'protected', 'virtual', 'override', 'final',
    'nullptr', 'constexpr', 'noexcept', 'decltype', 'explicit',
}

# Common library function prefixes to ignore (these follow their own conventions)
IGNORED_PREFIXES = (
    # C standard library
    'printf', 'fprintf', 'sprintf', 'snprintf', 'scanf', 'sscanf',
    'malloc', 'calloc', 'realloc', 'free',
    'memcpy', 'memmove', 'memset', 'memcmp',
    'strcpy', 'strncpy', 'strcat', 'strncat', 'strcmp', 'strncmp',
    'strlen', 'strstr', 'strchr', 'strrchr',
    'fopen', 'fclose', 'fread', 'fwrite', 'fseek', 'ftell', 'fflush',
    'getchar', 'putchar', 'getc', 'putc', 'gets', 'puts',
    'atoi', 'atol', 'atof', 'strtol', 'strtoul', 'strtod',
    'isalpha', 'isdigit', 'isalnum', 'isspace', 'isupper', 'islower',
    'toupper', 'tolower',
    'abs', 'labs', 'fabs', 'sqrt', 'pow', 'sin', 'cos', 'tan',
    'ceil', 'floor', 'round', 'log', 'log10', 'exp',
    'exit', 'abort', 'assert', 'perror', 'strerror',
    'clock', 'time', 'difftime', 'mktime',
    'pthread_', 'sem_', 'mutex_',
    # OpenCL (uses its own naming convention)
    'cl', 'CL_',
    # Common macros
    'MIN', 'MAX', 'ABS', 'CLAMP',
)

# Functions that should remain as-is (main, etc.)
SPECIAL_FUNCTIONS = {'main', 'wmain', 'WinMain', 'DllMain'}


# =============================================================================
# Naming Convention Helpers
# =============================================================================

def is_snake_case(name):
    """Check if name follows snake_case convention (lowercase with underscores)."""
    if not name:
        return True
    # Allow leading underscore for private symbols
    check_name = name.lstrip('_')
    if not check_name:
        return True
    # snake_case: lowercase letters, digits, underscores
    return bool(re.match(r'^[a-z][a-z0-9_]*$', check_name))


def is_upper_snake_case(name):
    """Check if name follows UPPER_SNAKE_CASE convention."""
    if not name:
        return True
    return bool(re.match(r'^[A-Z][A-Z0-9_]*$', name))


def is_pascal_case(name):
    """Check if name follows PascalCase (UpperCamelCase) convention."""
    if not name:
        return True
    # PascalCase: starts with uppercase, no underscores, mixed case allowed
    # Also allow acronyms like "HTTPServer" or "URLParser"
    if '_' in name:
        return False
    if not name[0].isupper():
        return False
    # Must have at least one lowercase letter (to distinguish from UPPER_SNAKE)
    # unless it's a short name (2-3 chars which could be an acronym)
    if len(name) > 3 and name.isupper():
        return False
    return bool(re.match(r'^[A-Z][a-zA-Z0-9]*$', name))


def is_k_constant(name):
    """Check if name follows kConstantName convention."""
    if not name:
        return True
    return bool(re.match(r'^k[A-Z][a-zA-Z0-9]*$', name))


def to_snake_case(name):
    """Convert name to snake_case."""
    # Handle leading underscores
    leading = len(name) - len(name.lstrip('_'))
    prefix = '_' * leading
    name = name.lstrip('_')

    if not name:
        return prefix

    # Insert underscore before uppercase letters
    result = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', name)
    result = re.sub(r'([a-z\d])([A-Z])', r'\1_\2', result)
    result = result.replace('-', '_').lower()

    # Clean up multiple underscores
    result = re.sub(r'_+', '_', result)

    return prefix + result


def to_pascal_case(name):
    """Convert name to PascalCase."""
    # Handle leading underscores (remove them for PascalCase)
    name = name.lstrip('_')

    if not name:
        return name

    # If already PascalCase, return as-is
    if is_pascal_case(name):
        return name

    # Split by underscores or camelCase boundaries
    if '_' in name:
        # snake_case to PascalCase
        parts = name.split('_')
        return ''.join(word.capitalize() for word in parts if word)
    else:
        # camelCase to PascalCase (just capitalize first letter)
        return name[0].upper() + name[1:]


def should_ignore_function(name):
    """Check if function name should be ignored."""
    if not name:
        return True
    if name in C_KEYWORDS:
        return True
    if name in SPECIAL_FUNCTIONS:
        return True
    if name.startswith(IGNORED_PREFIXES):
        return True
    # Ignore all-caps (likely macros)
    if is_upper_snake_case(name):
        return True
    return False


def should_ignore_variable(name):
    """Check if variable name should be ignored."""
    if not name:
        return True
    if name in C_KEYWORDS:
        return True
    if name.startswith(IGNORED_PREFIXES):
        return True
    # Ignore all-caps (likely macros/constants - those are OK)
    if is_upper_snake_case(name):
        return True
    # Ignore kConstant style (Google constant convention)
    if is_k_constant(name):
        return True
    # Ignore single letter variables (common in loops)
    if len(name) == 1:
        return True
    # Ignore common short names
    if name in {'i', 'j', 'k', 'n', 'm', 'x', 'y', 'z', 'fd', 'fp', 'ch', 'c', 'p', 'q', 's', 't'}:
        return True
    return False


# =============================================================================
# C Code Parser
# =============================================================================

class CCodeParser:
    """Simple C/C++ code parser for extracting function and variable names."""

    def __init__(self, content, filename):
        self.content = content
        self.filename = filename
        self.issues = []

        # Remove comments and strings for analysis
        self.clean_content = self._remove_comments_and_strings(content)

    def _remove_comments_and_strings(self, code):
        """Remove comments and string literals from code."""
        result = []
        i = 0
        in_string = False
        in_char = False
        string_char = None

        while i < len(code):
            # Check for string start
            if not in_string and not in_char:
                if code[i] == '"' and (i == 0 or code[i-1] != '\\'):
                    in_string = True
                    string_char = '"'
                    result.append(' ')
                    i += 1
                    continue
                elif code[i] == "'" and (i == 0 or code[i-1] != '\\'):
                    in_char = True
                    result.append(' ')
                    i += 1
                    continue

            # Check for string end
            if in_string and code[i] == string_char and code[i-1] != '\\':
                in_string = False
                result.append(' ')
                i += 1
                continue

            if in_char and code[i] == "'" and code[i-1] != '\\':
                in_char = False
                result.append(' ')
                i += 1
                continue

            # Skip content inside strings
            if in_string or in_char:
                result.append(' ')
                i += 1
                continue

            # Check for single-line comment
            if code[i:i+2] == '//':
                while i < len(code) and code[i] != '\n':
                    result.append(' ')
                    i += 1
                continue

            # Check for multi-line comment
            if code[i:i+2] == '/*':
                result.append(' ')
                result.append(' ')
                i += 2
                while i < len(code) - 1 and code[i:i+2] != '*/':
                    result.append(' ' if code[i] != '\n' else '\n')
                    i += 1
                if i < len(code) - 1:
                    result.append(' ')
                    result.append(' ')
                    i += 2
                continue

            result.append(code[i])
            i += 1

        return ''.join(result)

    def _get_line_number(self, pos):
        """Get line number for a position in the original content."""
        return self.content[:pos].count('\n') + 1

    def find_functions(self):
        """Find function declarations and definitions."""
        functions = []

        # Pattern for function definitions/declarations
        # Matches: return_type function_name(params)
        pattern = r'''
            (?:^|\n)\s*                           # Start of line
            (?:static\s+|inline\s+|extern\s+)*    # Optional modifiers
            (?:const\s+)?                         # Optional const
            (?:unsigned\s+|signed\s+)?            # Optional sign
            (?:struct\s+|enum\s+|union\s+)?       # Optional type keyword
            [a-zA-Z_][a-zA-Z0-9_]*                # Return type
            (?:\s*\*+\s*|\s+)                     # Pointer or space
            ([a-zA-Z_][a-zA-Z0-9_]*)              # Function name (captured)
            \s*\([^)]*\)                          # Parameters
            \s*(?:\{|;)                           # Body start or declaration end
        '''

        for match in re.finditer(pattern, self.clean_content, re.VERBOSE | re.MULTILINE):
            name = match.group(1)
            pos = match.start(1)
            line = self._get_line_number(pos)

            if not should_ignore_function(name):
                functions.append({
                    'name': name,
                    'line': line,
                    'pos': pos,
                    'type': 'function',
                })

        return functions

    def find_variables(self):
        """Find variable declarations."""
        variables = []

        # Pattern for variable declarations
        # Matches common patterns like: int foo, char* bar, struct X baz
        pattern = r'''
            (?:^|\n|[;{}])\s*                     # Start context
            (?:static\s+|const\s+|volatile\s+|register\s+|extern\s+)*  # Modifiers
            (?:unsigned\s+|signed\s+)?            # Sign
            (?:struct\s+|enum\s+|union\s+)?       # Type keyword
            [a-zA-Z_][a-zA-Z0-9_]*                # Type name
            (?:\s*\*+)?                           # Pointer
            \s+
            ([a-zA-Z_][a-zA-Z0-9_]*)              # Variable name (captured)
            \s*(?:=|;|,|\[)                       # Assignment, end, comma, or array
        '''

        for match in re.finditer(pattern, self.clean_content, re.VERBOSE | re.MULTILINE):
            name = match.group(1)
            pos = match.start(1)
            line = self._get_line_number(pos)

            # Skip if it looks like a function call or definition
            after = self.clean_content[match.end(1):match.end(1)+10].lstrip()
            if after.startswith('('):
                continue

            if not should_ignore_variable(name):
                variables.append({
                    'name': name,
                    'line': line,
                    'pos': pos,
                    'type': 'variable',
                })

        # Also find function parameters
        for func_match in re.finditer(r'[a-zA-Z_][a-zA-Z0-9_]*\s*\([^)]+\)', self.clean_content):
            params_str = func_match.group(0)
            params_match = re.search(r'\(([^)]+)\)', params_str)
            if params_match:
                params = params_match.group(1)
                # Parse individual parameters
                for param in params.split(','):
                    param = param.strip()
                    if not param or param == 'void':
                        continue
                    # Extract parameter name (last identifier)
                    parts = re.findall(r'[a-zA-Z_][a-zA-Z0-9_]*', param)
                    if parts:
                        name = parts[-1]
                        # Skip type names
                        if name in C_KEYWORDS or name in {'size_t', 'ssize_t', 'FILE'}:
                            continue
                        pos = func_match.start()
                        line = self._get_line_number(pos)
                        if not should_ignore_variable(name):
                            variables.append({
                                'name': name,
                                'line': line,
                                'pos': pos,
                                'type': 'parameter',
                            })

        return variables

    def check_naming(self):
        """Check all names for convention violations."""
        issues = []

        # Check functions - should be PascalCase
        for func in self.find_functions():
            name = func['name']
            if not is_pascal_case(name):
                suggested = to_pascal_case(name)
                issues.append({
                    'file': self.filename,
                    'line': func['line'],
                    'type': 'function',
                    'name': name,
                    'suggested': suggested,
                    'message': 'Function "{}" should be PascalCase: "{}"'.format(name, suggested),
                })

        # Check variables - should be snake_case
        seen = set()
        for var in self.find_variables():
            name = var['name']
            key = (name, var['line'])
            if key in seen:
                continue
            seen.add(key)

            if not is_snake_case(name):
                suggested = to_snake_case(name)
                issues.append({
                    'file': self.filename,
                    'line': var['line'],
                    'type': var['type'],
                    'name': name,
                    'suggested': suggested,
                    'message': '{} "{}" should be snake_case: "{}"'.format(
                        var['type'].capitalize(), name, suggested
                    ),
                })

        return issues


# =============================================================================
# Scanner
# =============================================================================

class StyleScanner:
    """Scans C/C++ files for naming convention violations."""

    def __init__(self, root_path, exclude_dirs=None, extensions=None):
        self.root_path = os.path.abspath(root_path)
        self.exclude_dirs = exclude_dirs or DEFAULT_EXCLUDES
        self.extensions = extensions or C_EXTENSIONS
        self.files = []
        self.issues = []

    def scan(self):
        """Scan all C/C++ files in the project."""
        self._find_files()
        self._analyze_files()
        return self

    def _find_files(self):
        """Find all C/C++ source files."""
        for dirpath, dirnames, filenames in os.walk(self.root_path):
            # Filter excluded directories
            dirnames[:] = [d for d in dirnames if d not in self.exclude_dirs]

            for filename in filenames:
                ext = os.path.splitext(filename)[1].lower()
                if ext in self.extensions:
                    full_path = os.path.join(dirpath, filename)
                    rel_path = os.path.relpath(full_path, self.root_path)
                    self.files.append({
                        'full_path': full_path,
                        'rel_path': rel_path,
                    })

    def _analyze_files(self):
        """Analyze all files for naming violations."""
        for file_info in self.files:
            try:
                with open(file_info['full_path'], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                parser = CCodeParser(content, file_info['rel_path'])
                file_issues = parser.check_naming()

                for issue in file_issues:
                    issue['full_path'] = file_info['full_path']
                    self.issues.append(issue)

            except (IOError, OSError) as e:
                print("Warning: Could not read {}: {}".format(file_info['rel_path'], e))

    def get_summary(self):
        """Get summary of issues by type."""
        summary = defaultdict(int)
        for issue in self.issues:
            summary[issue['type']] += 1
        return dict(summary)

    def get_issues_by_file(self):
        """Get issues grouped by file."""
        by_file = defaultdict(list)
        for issue in self.issues:
            by_file[issue['file']].append(issue)
        return dict(by_file)


# =============================================================================
# Fixer
# =============================================================================

class StyleFixer:
    """Fixes naming convention violations."""

    def __init__(self, scanner, dry_run=False):
        self.scanner = scanner
        self.dry_run = dry_run
        self.changes = []

    def fix(self):
        """Apply fixes to all files."""
        issues_by_file = self.scanner.get_issues_by_file()

        for rel_path, issues in issues_by_file.items():
            full_path = issues[0]['full_path']
            self._fix_file(full_path, rel_path, issues)

        return self.changes

    def _fix_file(self, full_path, rel_path, issues):
        """Fix issues in a single file."""
        try:
            with open(full_path, 'r', encoding='utf-8') as f:
                content = f.read()
        except (IOError, OSError) as e:
            print("Error reading {}: {}".format(rel_path, e))
            return

        original = content

        # Build replacement map
        replacements = {}
        for issue in issues:
            old_name = issue['name']
            new_name = issue['suggested']
            if old_name != new_name:
                replacements[old_name] = new_name

        # Apply replacements (whole word only)
        for old_name, new_name in replacements.items():
            # Use word boundary to avoid partial replacements
            pattern = r'\b' + re.escape(old_name) + r'\b'
            content = re.sub(pattern, new_name, content)

        if content != original:
            self.changes.append({
                'file': rel_path,
                'replacements': replacements,
            })

            if not self.dry_run:
                try:
                    with open(full_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                except (IOError, OSError) as e:
                    print("Error writing {}: {}".format(rel_path, e))


# =============================================================================
# Output Formatting
# =============================================================================

def print_summary(scanner):
    """Print summary of issues found."""
    issues = scanner.issues
    summary = scanner.get_summary()
    issues_by_file = scanner.get_issues_by_file()

    print("\n" + "=" * 70)
    print("C/C++ CODE STYLE SCANNER - Google C++ Style Guide")
    print("=" * 70)
    print("\nProject: {}".format(scanner.root_path))
    print("Files scanned: {}".format(len(scanner.files)))
    print()
    print("Convention: Functions=PascalCase, Variables=snake_case")
    print()

    if not issues:
        print("No naming convention violations found!")
        print()
        return

    # Summary
    print("SUMMARY")
    print("-" * 40)
    print("  Total issues: {}".format(len(issues)))
    for issue_type, count in sorted(summary.items()):
        print("  - {}: {}".format(issue_type.capitalize(), count))
    print()

    # Issues by file
    print("ISSUES BY FILE")
    print("-" * 40)

    for rel_path in sorted(issues_by_file.keys()):
        file_issues = issues_by_file[rel_path]
        print("\n{}:".format(rel_path))

        for issue in sorted(file_issues, key=lambda x: x['line']):
            print("  Line {}: {} '{}' -> '{}'".format(
                issue['line'],
                issue['type'],
                issue['name'],
                issue['suggested']
            ))

    print("\n" + "=" * 70)
    print("Run with --fix to automatically rename these symbols.")
    print("Run with --fix --dry-run to preview changes without applying.")
    print()


def print_fix_results(changes, dry_run):
    """Print results of fix operation."""
    if dry_run:
        print("\nDRY RUN - No changes applied")
        print("-" * 40)
    else:
        print("\nCHANGES APPLIED")
        print("-" * 40)

    if not changes:
        print("No changes needed.")
        return

    total_replacements = 0
    for change in changes:
        print("\n{}:".format(change['file']))
        for old, new in change['replacements'].items():
            print("  {} -> {}".format(old, new))
            total_replacements += 1

    print("\n" + "-" * 40)
    print("Total: {} files, {} replacements".format(len(changes), total_replacements))

    if dry_run:
        print("\nRun without --dry-run to apply these changes.")


# =============================================================================
# CLI
# =============================================================================

def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Scan C/C++ code for Google C++ Style Guide naming violations.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Google C++ Style Guide Naming Conventions:
  - Functions:  PascalCase      (e.g., AddTableEntry, OpenFile)
  - Variables:  snake_case      (e.g., table_name, num_entries)
  - Parameters: snake_case      (e.g., input_buffer, max_size)
  - Constants:  kCamelCase      (e.g., kDaysInWeek, kMaxSize)
  - Macros:     UPPER_SNAKE     (e.g., MAX_BUFFER_SIZE)
  - Types:      PascalCase      (e.g., UrlTable, MyClass)

Examples:
  %(prog)s /path/to/project           # Scan and show summary
  %(prog)s /path/to/project --fix     # Auto-fix violations
  %(prog)s . --fix --dry-run          # Preview fixes without applying
  %(prog)s . --exclude build,test     # Exclude directories
  %(prog)s . --cpp                     # Include C++ files (.cpp, .hpp)
        '''
    )

    parser.add_argument(
        'project_path',
        nargs='?',
        default='.',
        help='Path to project root (default: current directory)'
    )

    parser.add_argument(
        '--fix',
        action='store_true',
        help='Automatically fix naming violations'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be fixed without making changes'
    )

    parser.add_argument(
        '-e', '--exclude',
        default='',
        help='Comma-separated list of directories to exclude'
    )

    parser.add_argument(
        '--cpp',
        action='store_true',
        help='Also scan C++ files (.cpp, .hpp, .cc, .hh)'
    )

    parser.add_argument(
        '-q', '--quiet',
        action='store_true',
        help='Only show summary, not individual issues'
    )

    parser.add_argument(
        '--version',
        action='version',
        version='%(prog)s 2.0.0'
    )

    args = parser.parse_args()

    # Validate path
    if not os.path.isdir(args.project_path):
        print("Error: '{}' is not a valid directory".format(args.project_path))
        sys.exit(1)

    # Parse exclusions
    exclude_dirs = set(DEFAULT_EXCLUDES)
    if args.exclude:
        exclude_dirs.update(e.strip() for e in args.exclude.split(','))

    # Determine extensions to scan
    extensions = C_EXTENSIONS.copy()
    if args.cpp:
        extensions.update(CPP_EXTENSIONS)

    # Scan
    scanner = StyleScanner(args.project_path, exclude_dirs, extensions)
    scanner.scan()

    if args.fix:
        # Fix mode
        fixer = StyleFixer(scanner, dry_run=args.dry_run)
        changes = fixer.fix()
        print_fix_results(changes, args.dry_run)
    else:
        # Scan mode
        print_summary(scanner)

    # Exit code
    return 0 if not scanner.issues else 1


if __name__ == '__main__':
    sys.exit(main())

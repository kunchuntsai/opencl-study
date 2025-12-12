#!/usr/bin/env python3
"""
C/C++ Dependency Analyzer - Portable Edition

A portable tool for analyzing header dependencies in C/C++ projects.
Generates an interactive HTML visualization of the dependency graph.

Compatible with Python 3.6.3+

Usage:
    python3 cdep_analyzer.py /path/to/project
    python3 cdep_analyzer.py /path/to/project --output deps.html
    python3 cdep_analyzer.py /path/to/project --exclude build,third_party

Author: Generated for portable dependency analysis
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


# =============================================================================
# Configuration
# =============================================================================

# File extensions to scan
C_EXTENSIONS = {'.c', '.h'}
CPP_EXTENSIONS = {'.cpp', '.hpp', '.cc', '.hh', '.cxx', '.hxx', '.c++', '.h++'}
ALL_EXTENSIONS = C_EXTENSIONS | CPP_EXTENSIONS

# Common system headers to ignore (partial match)
SYSTEM_HEADER_PREFIXES = (
    # C standard library
    'stdio', 'stdlib', 'string', 'math', 'time', 'ctype', 'errno',
    'limits', 'stdbool', 'stdint', 'stddef', 'stdarg', 'signal',
    'assert', 'locale', 'setjmp', 'float', 'iso646', 'wchar', 'wctype',
    # POSIX
    'sys/', 'unistd', 'fcntl', 'pthread', 'dirent', 'termios',
    # C++ standard library
    'iostream', 'fstream', 'sstream', 'iomanip', 'vector', 'list',
    'map', 'set', 'unordered_map', 'unordered_set', 'array', 'deque',
    'queue', 'stack', 'algorithm', 'functional', 'iterator', 'memory',
    'utility', 'tuple', 'type_traits', 'chrono', 'ratio', 'thread',
    'mutex', 'condition_variable', 'future', 'atomic', 'regex',
    'random', 'numeric', 'complex', 'valarray', 'bitset', 'initializer_list',
    'typeinfo', 'typeindex', 'exception', 'stdexcept', 'new', 'limits',
    'climits', 'cfloat', 'cstdint', 'cstddef', 'cstdlib', 'cstring',
    'cmath', 'ctime', 'cctype', 'cerrno', 'cassert', 'cstdio', 'cstdarg',
    'csignal', 'clocale', 'csetjmp', 'cwchar', 'cwctype', 'cuchar',
    'cinttypes', 'cfenv', 'filesystem', 'optional', 'variant', 'any',
    'string_view', 'charconv', 'execution', 'span', 'ranges', 'format',
    'source_location', 'compare', 'version', 'numbers', 'bit', 'concepts',
    'coroutine', 'semaphore', 'latch', 'barrier', 'stop_token',
    # Platform specific
    'windows', 'Windows', 'win32', 'Win32', 'windef', 'winbase',
    'OpenCL/', 'CL/', 'cuda', 'CUDA', 'vulkan', 'Vulkan',
    'gl/', 'GL/', 'glm/', 'GLFW/', 'SDL', 'X11/', 'Cocoa/',
    # Common third-party
    'boost/', 'gtest/', 'gmock/', 'catch', 'doctest', 'json',
    'rapidjson/', 'nlohmann/', 'fmt/', 'spdlog/', 'eigen', 'Eigen/',
)

# Default directories to exclude
DEFAULT_EXCLUDES = {
    'build', 'cmake-build-debug', 'cmake-build-release',
    '.git', '.svn', '.hg',
    'node_modules', 'vendor', 'third_party', 'external', 'deps',
    '__pycache__', '.pytest_cache', '.tox',
}


# =============================================================================
# Dependency Scanner
# =============================================================================

class DependencyScanner:
    """Scans C/C++ files for dependencies."""

    def __init__(self, root_path, exclude_dirs=None, include_system=False):
        """
        Initialize the scanner.

        Args:
            root_path: Path to the project root directory
            exclude_dirs: Set of directory names to exclude
            include_system: Whether to include system headers
        """
        self.root_path = os.path.abspath(root_path)
        self.exclude_dirs = exclude_dirs or DEFAULT_EXCLUDES
        self.include_system = include_system

        # Regex for #include statements
        self.include_pattern = re.compile(
            r'^\s*#\s*include\s+([<"])([^>"]+)[>"]',
            re.MULTILINE
        )

        # Storage
        self.files = {}  # file_path -> FileInfo
        self.dependencies = defaultdict(set)  # file -> set of included files
        self.reverse_deps = defaultdict(set)  # file -> set of files that include it
        self.unresolved = defaultdict(set)  # file -> set of unresolved includes

    def scan(self):
        """Scan the project and build dependency graph."""
        # Find all source files
        self._find_files()

        # Parse includes
        self._parse_includes()

        # Resolve dependencies
        self._resolve_dependencies()

        return self

    def _find_files(self):
        """Find all C/C++ files in the project."""
        for dirpath, dirnames, filenames in os.walk(self.root_path):
            # Filter out excluded directories
            dirnames[:] = [d for d in dirnames if d not in self.exclude_dirs]

            for filename in filenames:
                ext = os.path.splitext(filename)[1].lower()
                if ext in ALL_EXTENSIONS:
                    full_path = os.path.join(dirpath, filename)
                    rel_path = os.path.relpath(full_path, self.root_path)

                    self.files[rel_path] = {
                        'full_path': full_path,
                        'rel_path': rel_path,
                        'filename': filename,
                        'extension': ext,
                        'directory': os.path.relpath(dirpath, self.root_path),
                        'is_header': ext in {'.h', '.hpp', '.hh', '.hxx', '.h++'},
                        'raw_includes': [],
                        'line_count': 0,
                    }

    def _parse_includes(self):
        """Parse #include statements from all files."""
        for rel_path, info in self.files.items():
            try:
                with open(info['full_path'], 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                # Count lines
                info['line_count'] = content.count('\n') + 1

                # Find includes
                for match in self.include_pattern.finditer(content):
                    bracket_type = match.group(1)  # < or "
                    include_path = match.group(2)

                    is_system = (bracket_type == '<')

                    # Skip system headers if not requested
                    if is_system and not self.include_system:
                        if include_path.startswith(SYSTEM_HEADER_PREFIXES):
                            continue

                    info['raw_includes'].append({
                        'path': include_path,
                        'is_system': is_system,
                    })
            except (IOError, OSError) as e:
                print("Warning: Could not read {}: {}".format(rel_path, e))

    def _resolve_dependencies(self):
        """Resolve include paths to actual files."""
        # Build lookup maps
        filename_map = defaultdict(list)  # filename -> list of rel_paths
        for rel_path in self.files:
            filename = os.path.basename(rel_path)
            filename_map[filename].append(rel_path)

        for rel_path, info in self.files.items():
            file_dir = os.path.dirname(rel_path)

            for inc in info['raw_includes']:
                inc_path = inc['path']
                resolved = None

                # Try to resolve the include
                # 1. Relative to current file
                candidate = os.path.normpath(os.path.join(file_dir, inc_path))
                if candidate in self.files:
                    resolved = candidate

                # 2. From project root
                if resolved is None:
                    if inc_path in self.files:
                        resolved = inc_path

                # 3. By filename match
                if resolved is None:
                    basename = os.path.basename(inc_path)
                    candidates = filename_map.get(basename, [])

                    if len(candidates) == 1:
                        resolved = candidates[0]
                    elif len(candidates) > 1:
                        # Try to find best match by path similarity
                        for c in candidates:
                            if c.endswith(inc_path):
                                resolved = c
                                break
                        if resolved is None:
                            # Use first match
                            resolved = candidates[0]

                if resolved:
                    self.dependencies[rel_path].add(resolved)
                    self.reverse_deps[resolved].add(rel_path)
                elif not inc['is_system']:
                    self.unresolved[rel_path].add(inc_path)

    def get_stats(self):
        """Get statistics about the scan."""
        total_files = len(self.files)
        headers = sum(1 for f in self.files.values() if f['is_header'])
        sources = total_files - headers
        total_lines = sum(f['line_count'] for f in self.files.values())
        total_deps = sum(len(d) for d in self.dependencies.values())

        return {
            'total_files': total_files,
            'header_files': headers,
            'source_files': sources,
            'total_lines': total_lines,
            'total_dependencies': total_deps,
            'files_with_deps': len(self.dependencies),
            'unresolved_includes': sum(len(u) for u in self.unresolved.values()),
        }

    def find_cycles(self):
        """Find circular dependencies."""
        cycles = []
        visited = set()
        rec_stack = set()
        path = []

        def dfs(node):
            visited.add(node)
            rec_stack.add(node)
            path.append(node)

            for neighbor in self.dependencies.get(node, set()):
                if neighbor not in visited:
                    cycle = dfs(neighbor)
                    if cycle:
                        return cycle
                elif neighbor in rec_stack:
                    # Found cycle
                    cycle_start = path.index(neighbor)
                    return path[cycle_start:] + [neighbor]

            path.pop()
            rec_stack.remove(node)
            return None

        for node in self.files:
            if node not in visited:
                cycle = dfs(node)
                if cycle:
                    # Normalize cycle (start from smallest element)
                    min_idx = cycle.index(min(cycle[:-1]))
                    normalized = cycle[min_idx:-1] + cycle[:min_idx] + [cycle[min_idx]]
                    if normalized not in cycles:
                        cycles.append(normalized)

        return cycles

    def get_directory_deps(self):
        """Get dependencies aggregated by directory."""
        dir_deps = defaultdict(lambda: defaultdict(int))

        for src_file, deps in self.dependencies.items():
            src_dir = os.path.dirname(src_file) or '.'
            for dep_file in deps:
                dep_dir = os.path.dirname(dep_file) or '.'
                if src_dir != dep_dir:
                    dir_deps[src_dir][dep_dir] += 1

        return dict(dir_deps)

    def get_most_included(self, limit=20):
        """Get most frequently included files."""
        counts = [(f, len(deps)) for f, deps in self.reverse_deps.items()]
        counts.sort(key=lambda x: -x[1])
        return counts[:limit]

    def get_most_including(self, limit=20):
        """Get files with most includes."""
        counts = [(f, len(deps)) for f, deps in self.dependencies.items()]
        counts.sort(key=lambda x: -x[1])
        return counts[:limit]


# =============================================================================
# HTML Report Generator
# =============================================================================

def generate_html_report(scanner, output_path):
    """Generate an interactive HTML report."""

    stats = scanner.get_stats()
    cycles = scanner.find_cycles()
    dir_deps = scanner.get_directory_deps()
    most_included = scanner.get_most_included(15)
    most_including = scanner.get_most_including(15)

    # Prepare graph data for D3.js
    nodes = []
    node_index = {}

    # Get all directories
    directories = set()
    for rel_path in scanner.files:
        dir_name = os.path.dirname(rel_path) or '.'
        directories.add(dir_name)

    # Create nodes for files
    for idx, (rel_path, info) in enumerate(scanner.files.items()):
        node_index[rel_path] = idx
        dir_name = os.path.dirname(rel_path) or '.'
        nodes.append({
            'id': idx,
            'name': info['filename'],
            'path': rel_path,
            'directory': dir_name,
            'isHeader': info['is_header'],
            'lines': info['line_count'],
            'fanIn': len(scanner.reverse_deps.get(rel_path, set())),
            'fanOut': len(scanner.dependencies.get(rel_path, set())),
        })

    # Create links
    links = []
    for src_file, deps in scanner.dependencies.items():
        src_idx = node_index.get(src_file)
        if src_idx is not None:
            for dep_file in deps:
                tgt_idx = node_index.get(dep_file)
                if tgt_idx is not None:
                    links.append({
                        'source': src_idx,
                        'target': tgt_idx,
                    })

    # Directory summary
    dir_summary = []
    for dir_name in sorted(directories):
        files_in_dir = [f for f, i in scanner.files.items()
                       if (os.path.dirname(f) or '.') == dir_name]
        lines = sum(scanner.files[f]['line_count'] for f in files_in_dir)
        dir_summary.append({
            'name': dir_name,
            'files': len(files_in_dir),
            'lines': lines,
        })

    # Generate HTML
    html = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>C/C++ Dependency Analysis Report</title>
    <style>
        * {{
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: #1a1a2e;
            color: #eee;
            line-height: 1.6;
        }}
        .container {{
            max-width: 1400px;
            margin: 0 auto;
            padding: 20px;
        }}
        header {{
            text-align: center;
            padding: 30px 0;
            border-bottom: 1px solid #333;
            margin-bottom: 30px;
        }}
        header h1 {{
            color: #00d4ff;
            font-size: 2.5em;
            margin-bottom: 10px;
        }}
        header .subtitle {{
            color: #888;
            font-size: 1.1em;
        }}
        .stats-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }}
        .stat-card {{
            background: #16213e;
            border-radius: 10px;
            padding: 20px;
            text-align: center;
            border: 1px solid #333;
        }}
        .stat-card .value {{
            font-size: 2.5em;
            font-weight: bold;
            color: #00d4ff;
        }}
        .stat-card .label {{
            color: #888;
            margin-top: 5px;
        }}
        .section {{
            background: #16213e;
            border-radius: 10px;
            padding: 25px;
            margin-bottom: 25px;
            border: 1px solid #333;
        }}
        .section h2 {{
            color: #00d4ff;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 1px solid #333;
        }}
        .graph-container {{
            width: 100%;
            height: 600px;
            background: #0f0f23;
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
            gap: 10px;
            flex-wrap: wrap;
        }}
        .graph-controls button, .graph-controls select {{
            background: #16213e;
            color: #eee;
            border: 1px solid #333;
            padding: 8px 15px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
        }}
        .graph-controls button:hover {{
            background: #1a365d;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
        }}
        th, td {{
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #333;
        }}
        th {{
            color: #00d4ff;
            font-weight: 600;
        }}
        tr:hover {{
            background: rgba(0, 212, 255, 0.1);
        }}
        .bar {{
            background: #00d4ff;
            height: 20px;
            border-radius: 3px;
            min-width: 5px;
        }}
        .bar-container {{
            background: #333;
            border-radius: 3px;
            overflow: hidden;
            width: 200px;
        }}
        .cycle-warning {{
            background: #5a1a1a;
            border: 1px solid #ff4444;
            border-radius: 8px;
            padding: 15px;
            margin-bottom: 20px;
        }}
        .cycle-warning h3 {{
            color: #ff4444;
            margin-bottom: 10px;
        }}
        .cycle-path {{
            font-family: monospace;
            background: #2a1a1a;
            padding: 10px;
            border-radius: 5px;
            margin: 5px 0;
            overflow-x: auto;
        }}
        .no-cycles {{
            color: #44ff44;
            padding: 15px;
            background: #1a3a1a;
            border-radius: 8px;
            border: 1px solid #44ff44;
        }}
        .tooltip {{
            position: absolute;
            background: #16213e;
            border: 1px solid #00d4ff;
            border-radius: 5px;
            padding: 10px;
            pointer-events: none;
            font-size: 12px;
            max-width: 300px;
            z-index: 100;
        }}
        .legend {{
            display: flex;
            gap: 20px;
            margin-top: 15px;
            flex-wrap: wrap;
        }}
        .legend-item {{
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        .legend-color {{
            width: 16px;
            height: 16px;
            border-radius: 50%;
        }}
        .tabs {{
            display: flex;
            gap: 5px;
            margin-bottom: 20px;
        }}
        .tab {{
            background: #0f0f23;
            border: 1px solid #333;
            color: #888;
            padding: 10px 20px;
            cursor: pointer;
            border-radius: 5px 5px 0 0;
        }}
        .tab.active {{
            background: #16213e;
            color: #00d4ff;
            border-bottom-color: #16213e;
        }}
        .tab-content {{
            display: none;
        }}
        .tab-content.active {{
            display: block;
        }}
        footer {{
            text-align: center;
            padding: 20px;
            color: #666;
            border-top: 1px solid #333;
            margin-top: 30px;
        }}
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Dependency Analysis Report</h1>
            <div class="subtitle">
                Project: {project_path}<br>
                Generated: {timestamp}
            </div>
        </header>

        <div class="stats-grid">
            <div class="stat-card">
                <div class="value">{total_files}</div>
                <div class="label">Total Files</div>
            </div>
            <div class="stat-card">
                <div class="value">{header_files}</div>
                <div class="label">Header Files</div>
            </div>
            <div class="stat-card">
                <div class="value">{source_files}</div>
                <div class="label">Source Files</div>
            </div>
            <div class="stat-card">
                <div class="value">{total_lines:,}</div>
                <div class="label">Lines of Code</div>
            </div>
            <div class="stat-card">
                <div class="value">{total_deps}</div>
                <div class="label">Dependencies</div>
            </div>
            <div class="stat-card">
                <div class="value">{cycle_count}</div>
                <div class="label">Circular Deps</div>
            </div>
        </div>

        <div class="section">
            <h2>Dependency Graph</h2>
            <div class="graph-container" id="graph">
                <div class="graph-controls">
                    <select id="dirFilter">
                        <option value="">All Directories</option>
                        {dir_options}
                    </select>
                    <button onclick="resetZoom()">Reset View</button>
                    <button onclick="toggleLabels()">Toggle Labels</button>
                </div>
            </div>
            <div class="legend">
                <div class="legend-item">
                    <div class="legend-color" style="background: #00d4ff;"></div>
                    <span>Header File</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color" style="background: #ff6b6b;"></div>
                    <span>Source File</span>
                </div>
            </div>
        </div>

        <div class="section">
            <h2>Circular Dependencies</h2>
            {cycles_html}
        </div>

        <div class="section">
            <h2>File Analysis</h2>
            <div class="tabs">
                <div class="tab active" onclick="showTab('most-included')">Most Included</div>
                <div class="tab" onclick="showTab('most-including')">Most Includes</div>
                <div class="tab" onclick="showTab('directories')">Directories</div>
            </div>

            <div id="most-included" class="tab-content active">
                <table>
                    <thead>
                        <tr>
                            <th>File</th>
                            <th>Included By</th>
                            <th></th>
                        </tr>
                    </thead>
                    <tbody>
                        {most_included_rows}
                    </tbody>
                </table>
            </div>

            <div id="most-including" class="tab-content">
                <table>
                    <thead>
                        <tr>
                            <th>File</th>
                            <th>Includes</th>
                            <th></th>
                        </tr>
                    </thead>
                    <tbody>
                        {most_including_rows}
                    </tbody>
                </table>
            </div>

            <div id="directories" class="tab-content">
                <table>
                    <thead>
                        <tr>
                            <th>Directory</th>
                            <th>Files</th>
                            <th>Lines</th>
                        </tr>
                    </thead>
                    <tbody>
                        {dir_rows}
                    </tbody>
                </table>
            </div>
        </div>

        <div class="section">
            <h2>Directory Dependencies</h2>
            <div class="graph-container" id="dir-graph" style="height: 400px;"></div>
        </div>

        <footer>
            Generated by C/C++ Dependency Analyzer | Python {python_version}
        </footer>
    </div>

    <div id="tooltip" class="tooltip" style="display: none;"></div>

    <script src="https://d3js.org/d3.v7.min.js"></script>
    <script>
        // Data
        const nodes = {nodes_json};
        const links = {links_json};
        const dirDeps = {dir_deps_json};

        // Settings
        let showLabels = true;
        let currentFilter = '';

        // File Graph
        const graphContainer = document.getElementById('graph');
        const width = graphContainer.clientWidth;
        const height = graphContainer.clientHeight;

        const svg = d3.select('#graph')
            .append('svg')
            .attr('width', width)
            .attr('height', height);

        const g = svg.append('g');

        // Zoom
        const zoom = d3.zoom()
            .scaleExtent([0.1, 4])
            .on('zoom', (event) => g.attr('transform', event.transform));

        svg.call(zoom);

        function resetZoom() {{
            svg.transition().duration(500).call(zoom.transform, d3.zoomIdentity);
        }}

        function toggleLabels() {{
            showLabels = !showLabels;
            g.selectAll('.node-label').style('display', showLabels ? 'block' : 'none');
        }}

        // Force simulation
        const simulation = d3.forceSimulation(nodes)
            .force('link', d3.forceLink(links).id(d => d.id).distance(80))
            .force('charge', d3.forceManyBody().strength(-200))
            .force('center', d3.forceCenter(width / 2, height / 2))
            .force('collision', d3.forceCollide().radius(30));

        // Links
        const link = g.append('g')
            .selectAll('line')
            .data(links)
            .join('line')
            .attr('stroke', '#444')
            .attr('stroke-opacity', 0.6)
            .attr('stroke-width', 1);

        // Nodes
        const node = g.append('g')
            .selectAll('g')
            .data(nodes)
            .join('g')
            .call(d3.drag()
                .on('start', dragstarted)
                .on('drag', dragged)
                .on('end', dragended));

        node.append('circle')
            .attr('r', d => Math.min(5 + Math.sqrt(d.fanIn + d.fanOut), 15))
            .attr('fill', d => d.isHeader ? '#00d4ff' : '#ff6b6b')
            .attr('stroke', '#fff')
            .attr('stroke-width', 1);

        node.append('text')
            .attr('class', 'node-label')
            .attr('dx', 12)
            .attr('dy', 4)
            .text(d => d.name)
            .attr('fill', '#888')
            .attr('font-size', '10px');

        // Tooltip
        const tooltip = document.getElementById('tooltip');

        node.on('mouseover', (event, d) => {{
            tooltip.innerHTML = `
                <strong>${{d.path}}</strong><br>
                Lines: ${{d.lines}}<br>
                Includes: ${{d.fanOut}} files<br>
                Included by: ${{d.fanIn}} files
            `;
            tooltip.style.display = 'block';
            tooltip.style.left = (event.pageX + 10) + 'px';
            tooltip.style.top = (event.pageY - 10) + 'px';
        }})
        .on('mouseout', () => {{
            tooltip.style.display = 'none';
        }});

        simulation.on('tick', () => {{
            link
                .attr('x1', d => d.source.x)
                .attr('y1', d => d.source.y)
                .attr('x2', d => d.target.x)
                .attr('y2', d => d.target.y);

            node.attr('transform', d => `translate(${{d.x}},${{d.y}})`);
        }});

        function dragstarted(event) {{
            if (!event.active) simulation.alphaTarget(0.3).restart();
            event.subject.fx = event.subject.x;
            event.subject.fy = event.subject.y;
        }}

        function dragged(event) {{
            event.subject.fx = event.x;
            event.subject.fy = event.y;
        }}

        function dragended(event) {{
            if (!event.active) simulation.alphaTarget(0);
            event.subject.fx = null;
            event.subject.fy = null;
        }}

        // Filter by directory
        document.getElementById('dirFilter').addEventListener('change', (e) => {{
            currentFilter = e.target.value;

            node.style('opacity', d => {{
                if (!currentFilter) return 1;
                return d.directory === currentFilter ? 1 : 0.1;
            }});

            link.style('opacity', d => {{
                if (!currentFilter) return 0.6;
                return (d.source.directory === currentFilter || d.target.directory === currentFilter) ? 0.6 : 0.05;
            }});
        }});

        // Tab switching
        function showTab(tabId) {{
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));

            event.target.classList.add('active');
            document.getElementById(tabId).classList.add('active');
        }}

        // Directory dependency graph
        const dirContainer = document.getElementById('dir-graph');
        const dirWidth = dirContainer.clientWidth;
        const dirHeight = dirContainer.clientHeight;

        // Build directory nodes and links
        const dirSet = new Set();
        const dirLinks = [];

        for (const [src, targets] of Object.entries(dirDeps)) {{
            dirSet.add(src);
            for (const [tgt, count] of Object.entries(targets)) {{
                dirSet.add(tgt);
                dirLinks.push({{ source: src, target: tgt, value: count }});
            }}
        }}

        const dirNodes = Array.from(dirSet).map(d => ({{ id: d, name: d }}));

        if (dirNodes.length > 0) {{
            const dirSvg = d3.select('#dir-graph')
                .append('svg')
                .attr('width', dirWidth)
                .attr('height', dirHeight);

            const dirG = dirSvg.append('g');

            dirSvg.call(d3.zoom()
                .scaleExtent([0.1, 4])
                .on('zoom', (event) => dirG.attr('transform', event.transform)));

            const dirSim = d3.forceSimulation(dirNodes)
                .force('link', d3.forceLink(dirLinks).id(d => d.id).distance(120))
                .force('charge', d3.forceManyBody().strength(-500))
                .force('center', d3.forceCenter(dirWidth / 2, dirHeight / 2));

            // Arrow marker
            dirSvg.append('defs').append('marker')
                .attr('id', 'arrowhead')
                .attr('viewBox', '-0 -5 10 10')
                .attr('refX', 20)
                .attr('refY', 0)
                .attr('orient', 'auto')
                .attr('markerWidth', 6)
                .attr('markerHeight', 6)
                .append('path')
                .attr('d', 'M 0,-5 L 10,0 L 0,5')
                .attr('fill', '#666');

            const dirLink = dirG.append('g')
                .selectAll('line')
                .data(dirLinks)
                .join('line')
                .attr('stroke', '#666')
                .attr('stroke-width', d => Math.min(Math.sqrt(d.value), 5))
                .attr('marker-end', 'url(#arrowhead)');

            const dirNode = dirG.append('g')
                .selectAll('g')
                .data(dirNodes)
                .join('g')
                .call(d3.drag()
                    .on('start', (e) => {{ if (!e.active) dirSim.alphaTarget(0.3).restart(); e.subject.fx = e.subject.x; e.subject.fy = e.subject.y; }})
                    .on('drag', (e) => {{ e.subject.fx = e.x; e.subject.fy = e.y; }})
                    .on('end', (e) => {{ if (!e.active) dirSim.alphaTarget(0); e.subject.fx = null; e.subject.fy = null; }}));

            dirNode.append('circle')
                .attr('r', 12)
                .attr('fill', '#00d4ff')
                .attr('stroke', '#fff')
                .attr('stroke-width', 2);

            dirNode.append('text')
                .attr('dx', 15)
                .attr('dy', 4)
                .text(d => d.id)
                .attr('fill', '#fff')
                .attr('font-size', '12px');

            dirSim.on('tick', () => {{
                dirLink
                    .attr('x1', d => d.source.x)
                    .attr('y1', d => d.source.y)
                    .attr('x2', d => d.target.x)
                    .attr('y2', d => d.target.y);

                dirNode.attr('transform', d => `translate(${{d.x}},${{d.y}})`);
            }});
        }} else {{
            document.getElementById('dir-graph').innerHTML = '<p style="padding: 20px; color: #888;">No cross-directory dependencies found.</p>';
        }}
    </script>
</body>
</html>
'''

    # Generate dynamic content
    max_included = max(c for _, c in most_included) if most_included else 1
    most_included_rows = '\n'.join(
        '<tr><td>{}</td><td>{}</td><td><div class="bar-container"><div class="bar" style="width: {}%;"></div></div></td></tr>'.format(
            f, c, int(c / max_included * 100)
        )
        for f, c in most_included
    )

    max_including = max(c for _, c in most_including) if most_including else 1
    most_including_rows = '\n'.join(
        '<tr><td>{}</td><td>{}</td><td><div class="bar-container"><div class="bar" style="width: {}%;"></div></div></td></tr>'.format(
            f, c, int(c / max_including * 100)
        )
        for f, c in most_including
    )

    dir_rows = '\n'.join(
        '<tr><td>{}</td><td>{}</td><td>{:,}</td></tr>'.format(
            d['name'], d['files'], d['lines']
        )
        for d in sorted(dir_summary, key=lambda x: -x['lines'])
    )

    dir_options = '\n'.join(
        '<option value="{0}">{0}</option>'.format(d['name'])
        for d in sorted(dir_summary, key=lambda x: x['name'])
    )

    if cycles:
        cycles_html = '<div class="cycle-warning"><h3>Circular Dependencies Detected!</h3>'
        for cycle in cycles:
            cycles_html += '<div class="cycle-path">{}</div>'.format(' &rarr; '.join(cycle))
        cycles_html += '</div>'
    else:
        cycles_html = '<div class="no-cycles">No circular dependencies detected.</div>'

    # Format HTML
    html = html.format(
        project_path=scanner.root_path,
        timestamp=datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        total_files=stats['total_files'],
        header_files=stats['header_files'],
        source_files=stats['source_files'],
        total_lines=stats['total_lines'],
        total_deps=stats['total_dependencies'],
        cycle_count=len(cycles),
        nodes_json=json.dumps(nodes),
        links_json=json.dumps(links),
        dir_deps_json=json.dumps(dir_deps),
        most_included_rows=most_included_rows,
        most_including_rows=most_including_rows,
        dir_rows=dir_rows,
        dir_options=dir_options,
        cycles_html=cycles_html,
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
        description='Analyze C/C++ header dependencies and generate HTML visualization.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s /path/to/project
  %(prog)s /path/to/project --output deps.html
  %(prog)s /path/to/project --exclude build,vendor,third_party
  %(prog)s . --include-system

Output:
  The tool generates an interactive HTML file with:
  - Dependency graph visualization (drag, zoom, filter)
  - Circular dependency detection
  - Most included/including files
  - Directory-level dependency analysis
  - Statistics summary
        '''
    )

    parser.add_argument(
        'project_path',
        help='Path to the project root directory'
    )

    parser.add_argument(
        '-o', '--output',
        default='dependency_report.html',
        help='Output HTML file path (default: dependency_report.html)'
    )

    parser.add_argument(
        '-e', '--exclude',
        default='',
        help='Comma-separated list of directory names to exclude (default: build,third_party,...)'
    )

    parser.add_argument(
        '--include-system',
        action='store_true',
        help='Include system headers in analysis'
    )

    parser.add_argument(
        '-q', '--quiet',
        action='store_true',
        help='Suppress progress output'
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

    # Parse exclusions
    exclude_dirs = set(DEFAULT_EXCLUDES)
    if args.exclude:
        exclude_dirs.update(e.strip() for e in args.exclude.split(','))

    if not args.quiet:
        print("C/C++ Dependency Analyzer")
        print("=" * 50)
        print("Project: {}".format(os.path.abspath(args.project_path)))
        print("Excluding: {}".format(', '.join(sorted(exclude_dirs))))
        print()

    # Scan
    if not args.quiet:
        print("Scanning files...")

    scanner = DependencyScanner(
        args.project_path,
        exclude_dirs=exclude_dirs,
        include_system=args.include_system
    )
    scanner.scan()

    stats = scanner.get_stats()

    if not args.quiet:
        print("Found {} files ({} headers, {} sources)".format(
            stats['total_files'],
            stats['header_files'],
            stats['source_files']
        ))
        print("Total lines: {:,}".format(stats['total_lines']))
        print("Dependencies: {}".format(stats['total_dependencies']))

        if stats['unresolved_includes'] > 0:
            print("Unresolved includes: {}".format(stats['unresolved_includes']))

        cycles = scanner.find_cycles()
        if cycles:
            print("\nWarning: {} circular dependencies found!".format(len(cycles)))

        print()
        print("Generating HTML report...")

    # Generate report
    output_path = generate_html_report(scanner, args.output)

    if not args.quiet:
        print("Report saved to: {}".format(os.path.abspath(output_path)))
        print()
        print("Open the HTML file in a browser to view the interactive visualization.")

    return 0


if __name__ == '__main__':
    sys.exit(main())

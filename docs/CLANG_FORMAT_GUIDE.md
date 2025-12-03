# clang-format Usage Guide

This guide explains how to use the `.clang-format` configuration file in other projects to maintain consistent code style following the Google C++ Style Guide.

## Table of Contents

- [Quick Start](#quick-start)
- [Copying Configuration](#copying-configuration)
- [Formatting Code](#formatting-code)
- [IDE Integration](#ide-integration)
- [Automation](#automation)
- [CI/CD Integration](#cicd-integration)
- [Customization](#customization)
- [Best Practices](#best-practices)

## Quick Start

**Format a single file:**
```bash
clang-format -i myfile.c
```

**Format all C/C++ files:**
```bash
find . -name "*.c" -o -name "*.h" | xargs clang-format -i
```

**Preview changes without modifying:**
```bash
clang-format myfile.c  # Prints to stdout
```

## Copying Configuration

### Copy to New Project

```bash
# Copy .clang-format to your project root
cp /path/to/opencl-study/.clang-format /path/to/your/project/

# Or download from repository
wget https://raw.githubusercontent.com/your-repo/opencl-study/main/.clang-format
```

### Using as Template

Keep `.clang-format` in a central location:
```bash
# Store in dotfiles
cp .clang-format ~/dotfiles/

# Create alias for easy setup
alias init-format='cp ~/dotfiles/.clang-format .'
```

## Formatting Code

### Format Entire Project

**All C/C++ files recursively:**
```bash
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
  -exec clang-format -i {} \;
```

**Excluding directories:**
```bash
find . -type f \( -name "*.c" -o -name "*.h" \) \
  ! -path "*/build/*" \
  ! -path "*/third_party/*" \
  -exec clang-format -i {} \;
```

### Check Formatting (Dry Run)

**Check if a file needs formatting:**
```bash
clang-format file.c | diff file.c - > /dev/null
echo $?  # Returns 0 if formatted, 1 if needs formatting
```

**Check all files:**
```bash
for file in $(find . -name "*.c" -o -name "*.h"); do
  if ! clang-format "$file" | diff -q "$file" - > /dev/null; then
    echo "❌ $file needs formatting"
  fi
done
```

### Batch Operations

Create `scripts/format.sh`:
```bash
#!/bin/bash
# Format all C/C++ source files in the project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Formatting C/C++ files in $PROJECT_ROOT..."

find "$PROJECT_ROOT" -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
  ! -path "*/build/*" \
  ! -path "*/third_party/*" \
  -exec clang-format -i {} \;

echo "✅ Formatting complete!"
```

Create `scripts/check-format.sh`:
```bash
#!/bin/bash
# Check if code is properly formatted without modifying files

set -e

FAILED=0

echo "Checking code formatting..."

for file in $(find . -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp"); do
  if ! clang-format "$file" | diff -q "$file" - > /dev/null; then
    echo "❌ $file needs formatting"
    FAILED=1
  fi
done

if [ $FAILED -eq 0 ]; then
  echo "✅ All files are properly formatted!"
  exit 0
else
  echo ""
  echo "❌ Some files need formatting. Run: ./scripts/format.sh"
  exit 1
fi
```

Make scripts executable:
```bash
chmod +x scripts/format.sh scripts/check-format.sh
```

## IDE Integration

### Visual Studio Code

**1. Install Extension:**
- Install "C/C++" by Microsoft or "Clang-Format" by xaver

**2. Configure Settings:**

Create or edit `.vscode/settings.json`:
```json
{
  "C_Cpp.clang_format_style": "file",
  "editor.formatOnSave": true,
  "editor.defaultFormatter": "ms-vscode.cpptools",
  "files.associations": {
    "*.c": "c",
    "*.h": "c"
  },
  "[c]": {
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "ms-vscode.cpptools"
  },
  "[cpp]": {
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "ms-vscode.cpptools"
  }
}
```

**3. Usage:**
- Format: `Ctrl+Shift+I` (Windows/Linux) or `Cmd+Shift+I` (Mac)
- Format on save: Enabled automatically with above config

### Vim/Neovim

**Add to `.vimrc` or `init.vim`:**
```vim
" Auto-format on save
autocmd BufWritePre *.c,*.h,*.cpp,*.hpp :%!clang-format

" Or map to a key (without auto-format)
nnoremap <Leader>f :%!clang-format<CR>
vnoremap <Leader>f :!clang-format<CR>
```

**Using vim-clang-format plugin:**
```vim
" Install with vim-plug
Plug 'rhysd/vim-clang-format'

" Configuration
let g:clang_format#auto_format = 1
let g:clang_format#style_options = {
    \ "BasedOnStyle": "file",
    \ }

" Map to key
autocmd FileType c,cpp nnoremap <buffer><Leader>f :<C-u>ClangFormat<CR>
autocmd FileType c,cpp vnoremap <buffer><Leader>f :ClangFormat<CR>
```

### CLion / IntelliJ IDEA

**1. Enable ClangFormat:**
- Go to: `Settings → Editor → Code Style`
- Check: "Enable ClangFormat support"
- Select: "Use .clang-format file"

**2. Configure:**
- Set: "Enable ClangFormat with clangd server"
- Optional: Enable "Format on save"

**3. Usage:**
- Format: `Ctrl+Alt+L` (Windows/Linux) or `Cmd+Option+L` (Mac)

### Emacs

**Install clang-format.el:**
```elisp
;; Add to your init.el
(require 'clang-format)

;; Keybindings
(global-set-key (kbd "C-c f") 'clang-format-region)
(global-set-key (kbd "C-c b") 'clang-format-buffer)

;; Auto-format on save (optional)
(add-hook 'c-mode-hook
  (lambda ()
    (add-hook 'before-save-hook 'clang-format-buffer nil 'local)))
```

### Sublime Text

**Using Clang Format plugin:**
1. Install Package Control
2. Install "Clang Format"
3. Configure in Preferences → Package Settings → Clang Format

**Settings:**
```json
{
    "binary": "clang-format",
    "style": "file",
    "format_on_save": true
}
```

## Automation

### Git Pre-commit Hook

Automatically format staged files before commit.

**Create `.git/hooks/pre-commit`:**
```bash
#!/bin/bash
# Auto-format staged C/C++ files before commit

FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(c|h|cpp|hpp)$')

if [ -n "$FILES" ]; then
    echo "🔧 Running clang-format on staged files..."

    for FILE in $FILES; do
        # Format the file
        clang-format -i "$FILE"

        # Re-stage the formatted file
        git add "$FILE"

        echo "  ✅ Formatted: $FILE"
    done

    echo "✅ Code formatted successfully!"
fi

exit 0
```

**Make executable:**
```bash
chmod +x .git/hooks/pre-commit
```

**Alternative: Check-only hook (fails if not formatted):**
```bash
#!/bin/bash
# Check formatting before commit (doesn't auto-format)

FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(c|h|cpp|hpp)$')

if [ -z "$FILES" ]; then
    exit 0
fi

echo "🔍 Checking code formatting..."

FAILED=0
for FILE in $FILES; do
    if ! clang-format "$FILE" | diff -q "$FILE" - > /dev/null; then
        echo "❌ $FILE needs formatting"
        FAILED=1
    fi
done

if [ $FAILED -eq 1 ]; then
    echo ""
    echo "❌ Commit rejected: some files need formatting"
    echo "Run: ./scripts/format.sh"
    exit 1
fi

echo "✅ All files properly formatted!"
exit 0
```

### Git Pre-commit Framework

**Using pre-commit (https://pre-commit.com):**

**Create `.pre-commit-config.yaml`:**
```yaml
repos:
  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v17.0.6
    hooks:
      - id: clang-format
        types_or: [c, c++]
        args: ['-i']
```

**Install and use:**
```bash
# Install pre-commit
pip install pre-commit

# Install git hook
pre-commit install

# Run manually on all files
pre-commit run --all-files
```

## CI/CD Integration

### GitHub Actions

**Create `.github/workflows/clang-format.yml`:**
```yaml
name: Code Formatting Check

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main, develop ]

jobs:
  clang-format-check:
    runs-on: ubuntu-latest

    steps:
      - name: Checkout code
        uses: actions/checkout@v3

      - name: Install clang-format
        run: |
          sudo apt-get update
          sudo apt-get install -y clang-format

      - name: Check formatting
        run: |
          echo "Checking C/C++ code formatting..."
          FAILED=0

          for file in $(find . -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp"); do
            if ! clang-format "$file" | diff -q "$file" - > /dev/null; then
              echo "❌ $file needs formatting"
              FAILED=1
            fi
          done

          if [ $FAILED -eq 1 ]; then
            echo ""
            echo "❌ Some files need formatting"
            echo "Run: clang-format -i <file>"
            exit 1
          fi

          echo "✅ All files properly formatted!"

      - name: Suggest fixes (on failure)
        if: failure()
        run: |
          echo "To fix formatting issues locally, run:"
          echo "  find . -name '*.c' -o -name '*.h' | xargs clang-format -i"
```

**Using clang-format-action:**
```yaml
name: Code Formatting

on: [push, pull_request]

jobs:
  formatting-check:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: DoozyX/clang-format-lint-action@v0.16
        with:
          source: '.'
          extensions: 'h,c,cpp,hpp'
          clangFormatVersion: 16
          style: file
```

### GitLab CI

**Add to `.gitlab-ci.yml`:**
```yaml
stages:
  - lint

format-check:
  stage: lint
  image: ubuntu:22.04
  before_script:
    - apt-get update && apt-get install -y clang-format
  script:
    - |
      echo "Checking code formatting..."
      FAILED=0

      for file in $(find . -name "*.c" -o -name "*.h"); do
        if ! clang-format "$file" | diff -q "$file" - > /dev/null; then
          echo "❌ $file needs formatting"
          FAILED=1
        fi
      done

      if [ $FAILED -eq 1 ]; then
        echo "❌ Formatting check failed"
        exit 1
      fi

      echo "✅ All files properly formatted!"
  allow_failure: false
```

### Jenkins

**Jenkinsfile:**
```groovy
pipeline {
    agent any

    stages {
        stage('Format Check') {
            steps {
                sh '''
                    echo "Checking code formatting..."
                    FAILED=0

                    for file in $(find . -name "*.c" -o -name "*.h"); do
                        if ! clang-format "$file" | diff -q "$file" - > /dev/null; then
                            echo "❌ $file needs formatting"
                            FAILED=1
                        fi
                    done

                    if [ $FAILED -eq 1 ]; then
                        echo "❌ Formatting check failed"
                        exit 1
                    fi

                    echo "✅ All files properly formatted!"
                '''
            }
        }
    }
}
```

## Customization

### Modify for Your Project

Edit key settings in `.clang-format`:

**Line Length:**
```yaml
ColumnLimit: 100  # Change to 80, 120, or 0 (unlimited)
```

**Indentation:**
```yaml
IndentWidth: 2    # Change to 4 for wider indents
UseTab: Never     # Or: ForIndentation, Always
```

**Brace Style:**
```yaml
BreakBeforeBraces: Attach  # Or: Allman, GNU, Stroustrup, WebKit, Linux
```

**Pointer Alignment:**
```yaml
PointerAlignment: Left  # Or: Right, Middle
# Examples:
#   Left:   int* ptr;
#   Right:  int *ptr;
#   Middle: int * ptr;
```

**Function Declarations:**
```yaml
AllowAllParametersOfDeclarationOnNextLine: true
BinPackParameters: true  # Pack params on one line when possible
```

**Include Ordering:**
```yaml
SortIncludes: true
IncludeBlocks: Regroup
IncludeCategories:
  - Regex:    '^<.*\.h>'    # C standard headers first
    Priority:  1
  - Regex:    '^<.*'        # C++ standard headers second
    Priority:  2
  - Regex:    '.*'          # Project headers last
    Priority:  3
```

### Generate Config from Code Style

**From existing code:**
```bash
# Generate .clang-format based on existing style
clang-format -style=llvm -dump-config > .clang-format

# Or based on Google style
clang-format -style=google -dump-config > .clang-format
```

### Multiple Configurations

Use different configs for different directories:

```
project/
├── .clang-format          # Root config (general)
├── src/
│   └── legacy/
│       └── .clang-format  # Different style for legacy code
└── include/
```

clang-format will use the nearest `.clang-format` file in parent directories.

## Best Practices

### Version Control

**1. Commit .clang-format:**
```bash
git add .clang-format
git commit -m "Add clang-format configuration"
```

**2. Format in separate commit:**
```bash
# Apply formatting
./scripts/format.sh

# Commit formatting changes separately
git add -A
git commit -m "style: apply clang-format to entire codebase"
```

**3. Document in README:**
```markdown
## Code Style

This project uses clang-format for consistent code formatting.

**Format your code:**
```bash
./scripts/format.sh
```

**Check formatting:**
```bash
./scripts/check-format.sh
```

See [docs/CLANG_FORMAT_GUIDE.md](docs/CLANG_FORMAT_GUIDE.md) for details.
```

### Team Workflow

**1. Consistent clang-format version:**
```bash
# Check version
clang-format --version

# Document required version in README
# e.g., "Requires clang-format 14.0 or later"
```

**2. Pre-commit hook for team:**
```bash
# Share pre-commit hook
cp .git/hooks/pre-commit scripts/pre-commit.sh

# Team members install:
ln -s ../../scripts/pre-commit.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

**3. Code review process:**
- Format before creating PR
- CI automatically checks formatting
- Reject PRs with formatting issues

### Migration Strategy

**For existing large codebases:**

1. **Test first:**
   ```bash
   # Create branch
   git checkout -b format-codebase

   # Format a small module first
   find src/module1 -name "*.c" -o -name "*.h" | xargs clang-format -i

   # Test compilation
   make clean && make
   ```

2. **Gradual adoption:**
   - Format one module at a time
   - Separate commits for each module
   - Run tests after each formatting commit

3. **One-time formatting:**
   ```bash
   # Format everything
   ./scripts/format.sh

   # Commit separately from functional changes
   git commit -m "style: apply clang-format across codebase"
   ```

4. **Communicate with team:**
   - Announce formatting change
   - Coordinate to minimize merge conflicts
   - Update contribution guidelines

### Troubleshooting

**Format not applied:**
```bash
# Verify .clang-format is in project root or parent directories
ls -la .clang-format

# Verify clang-format finds the config
clang-format -style=file -dump-config | head
```

**Different formatting on different machines:**
```bash
# Check clang-format versions match
clang-format --version

# Use Docker for consistent formatting
docker run --rm -v $(pwd):/src silkeh/clang:16 \
  clang-format -i /src/file.c
```

**Performance issues:**
```bash
# Format files in parallel
find . -name "*.c" -print0 | xargs -0 -P8 clang-format -i

# Or use GNU parallel
find . -name "*.c" | parallel clang-format -i {}
```

## Reference

### Command Line Options

```bash
# Format in-place
clang-format -i file.c

# Output to stdout (preview)
clang-format file.c

# Use specific config
clang-format -style=file:/path/to/.clang-format -i file.c

# Use predefined style
clang-format -style=google -i file.c

# Format specific lines
clang-format -lines=10:20 file.c

# Dump current config
clang-format -style=file -dump-config
```

### Naming Conventions (This Project)

As documented in our `.clang-format`:

- **Functions:** `PascalCase` (e.g., `RunAlgorithm`, `ParseConfig`)
- **Variables:** `snake_case` (e.g., `input_buffer`, `max_error`)
- **Constants/Macros:** `UPPER_SNAKE_CASE` (e.g., `MAX_IMAGE_SIZE`)
- **Types (struct/enum):** `PascalCase` (e.g., `Algorithm`, `KernelConfig`)
- **Enum values:** `UPPER_SNAKE_CASE` (e.g., `BUFFER_TYPE_READ_ONLY`)

**Note:** clang-format handles formatting only. Use clang-tidy for naming enforcement.

## Additional Resources

- [clang-format documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [ClangFormat Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [pre-commit framework](https://pre-commit.com/)

## Examples

### Before and After

**Before (inconsistent style):**
```c
int myFunction(int x,int y,int z){
if(x>0)
{
return x+y+z;
}
  return 0;}
```

**After (Google style):**
```c
int MyFunction(int x, int y, int z) {
  if (x > 0) {
    return x + y + z;
  }
  return 0;
}
```

### Common Use Cases

**Format changed files only (git):**
```bash
git diff --name-only --diff-filter=ACM | grep -E '\.(c|h)$' | xargs clang-format -i
```

**Format modified files (uncommitted):**
```bash
git diff --name-only | grep -E '\.(c|h)$' | xargs clang-format -i
```

**Format specific directory:**
```bash
find src/module -name "*.c" -o -name "*.h" | xargs clang-format -i
```

---

**Maintained by:** [Your Team]
**Last Updated:** 2025-12-03
**clang-format Version:** 18.1.3

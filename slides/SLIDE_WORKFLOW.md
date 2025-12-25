# Slide Generation Workflow Template

> **Purpose**: Reusable template for generating technical presentations with diagrams
> **Output**: Template-portable PowerPoint + SVG diagrams

---

## Quick Start Checklist

```
□ Fill out Section 1: Presentation Overview
□ Fill out Section 2: Design Specifications  
□ Fill out Section 3: Slide Definitions
□ Submit to Claude for generation
```

---

## Section 1: Presentation Overview

### Basic Information

| Field | Your Input |
|-------|------------|
| **Title** | |
| **Subtitle** | |
| **Presenter** | |
| **Date** | |
| **Duration** | __ minutes |
| **Audience** | (Engineers / Leadership / Mixed) |

### Sections Outline

| Section # | Section Name | Slides | Time (min) |
|-----------|--------------|--------|------------|
| 1 | | - | |
| 2 | | - | |
| 3 | | - | |
| 4 | | - | |
| 5 | | - | |
| 6 | | - | |

---

## Section 2: Design Specifications

### Color Theme

| Role | Hex Code | Usage |
|------|----------|-------|
| **Primary** | #______ | Main color, use in most cases (80%+) |
| **Secondary** | #______ | Only when contrast needed |
| **Tertiary** | #______ | Light backgrounds, sparingly |
| **Text Primary** | #______ | Main text color |
| **Text Secondary** | #______ | Subtext, descriptions |
| **Background** | #______ | Slide background |

**Example (Orange + Turquoise Theme)** — Recommended complementary palette:
```
Primary:        #FA9D1B (Orange)          — Main color, use in most cases
Secondary:      #67D4E4 (Soft Turquoise)  — Only when contrast needed (success states, outputs)
Tertiary:       #FFF8F0 (Cream)           — Light backgrounds, box fills
Text Primary:   #333333 (Dark Gray)       — Main text
Text Secondary: #666666 (Gray)            — Subtext, descriptions
Background:     #FFFFFF (White)           — Slide background
```

> **Usage Guideline**: Use **Primary (#FA9D1B)** in most cases — headers, accents, borders, highlights, section dividers. Only use **Secondary (#67D4E4)** when you need visual contrast (e.g., "before vs after", "input vs output", "cache miss vs hit"). Use **Tertiary (#FFF8F0)** sparingly for light box backgrounds.

### Layout Preferences

| Setting | Choice |
|---------|--------|
| **Aspect Ratio** | 16:9 / 4:3 |
| **Diagram Flow** | Horizontal / Vertical |
| **Font Family** | Calibri (default) / Arial / Other: ______ |
| **Code Font** | Consolas (default) / Courier New / Other: ______ |

### Template Portability Notes

> These settings ensure easy transfer to company templates:

- [ ] Use standard placeholder layouts (Title, Content, Two-Column, Blank)
- [ ] Avoid hardcoded colors in content (use accent colors sparingly)
- [ ] Keep text in proper hierarchy (Title → Subtitle → Body → Bullets)
- [ ] All diagrams as separate SVG files for easy replacement

---

## Section 3: Slide Definitions

### Slide Types Reference

| Type | Use Case | Layout |
|------|----------|--------|
| `title` | Opening slide | Centered title + subtitle |
| `section` | Section dividers | Large section name |
| `content` | Bullet points, text | Title + content area |
| `diagram` | Visual/flow diagrams | Title + SVG placeholder |
| `code` | Code snippets | Title + code block |
| `two-column` | Comparisons | Title + left/right columns |
| `table` | Data tables | Title + table |
| `summary` | Key takeaways | Title + summary points |
| `qa` | Q&A closing | Simple closing slide |

---

### Slide Definitions Template

Copy and fill for each slide:

```yaml
# Slide [N]: [Slide Title]
slide_number: 
slide_type: # title / section / content / diagram / code / two-column / table / summary / qa
title: ""
subtitle: ""  # optional

# For content/bullet slides:
bullets:
  - ""
  - ""
  - ""

# For diagram slides:
diagram:
  filename: ""  # e.g., "01_architecture.svg"
  description: "" # What the diagram shows

# For code slides:
code:
  language: ""
  snippet: |
    

# For two-column slides:
left_column:
  header: ""
  content: []
right_column:
  header: ""
  content: []

# For table slides:
table:
  headers: []
  rows: []

# Speaker notes (what to say):
speaker_notes: |
  

```

---

## Section 4: Diagram Specifications

### Diagram Request Template

For each diagram needed, fill out:

```yaml
# Diagram: [Name]
filename: ""
title: ""
type: # flow / architecture / comparison / timeline / process
orientation: horizontal / vertical
dimensions: "960x540"  # or "1920x1080" for high-res

# For flow diagrams:
flow_steps:
  - step: ""
    description: ""
  - step: ""
    description: ""

# For architecture diagrams:
layers:
  - name: ""
    components: []
  - name: ""
    components: []

# For comparison diagrams:
compare:
  left:
    label: ""
    items: []
  right:
    label: ""
    items: []

# Special elements:
highlights: []  # Steps/boxes to emphasize
annotations: [] # Side notes or callouts
```

---

## Section 5: Delivery Instructions

### Output Preferences

| Output | Needed? | Notes |
|--------|---------|-------|
| PowerPoint (.pptx) | Yes / No | |
| SVG Diagrams (individual) | Yes / No | |
| PDF Export | Yes / No | |
| Speaker Notes | Yes / No | |

### Template Application Instructions

After receiving the generated PowerPoint:

**Method A - Apply your template theme:**
1. Open the generated .pptx
2. Go to **Design → Browse for Themes**
3. Select your company template
4. Adjust individual slides: **Home → Layout**

**Method B - Copy into your template:**
1. Open your company template
2. Open the generated file side-by-side
3. Copy slides (Right-click → **Use Destination Theme**)

---

## Example: Filled Template

```yaml
# Section 1: Presentation Overview
title: "OpenCL Image Processing Framework"
subtitle: "Easy Kernel Development, Zero Host Code"
presenter: "Your Name"
date: "2025-01-15"
duration: 60
audience: "Engineers"

sections:
  - name: "Introduction"
    slides: "1-5"
    time: 8
  - name: "Architecture"
    slides: "6-10"
    time: 12

# Section 2: Design Specifications
colors:
  primary: "#FA9D1B"      # Use most of the time
  secondary: "#67D4E4"    # Only when contrast needed
  tertiary: "#FFF8F0"     # Light backgrounds, sparingly
  text_primary: "#333333"
  text_secondary: "#666666"
  background: "#FFFFFF"

layout:
  aspect_ratio: "16:9"
  diagram_flow: "horizontal"
  font_family: "Calibri"

# Section 3: Slide 1
- slide_number: 1
  slide_type: title
  title: "OpenCL Image Processing Framework"
  subtitle: "Easy Kernel Development, Zero Host Code"
  speaker_notes: |
    Welcome everyone. Today I'll introduce our framework
    that eliminates host code complexity.

# Section 3: Slide 2
- slide_number: 2
  slide_type: diagram
  title: "Architecture Overview"
  diagram:
    filename: "02_architecture.svg"
    description: "Shows user space vs framework responsibilities"
  speaker_notes: |
    This diagram shows the clear separation between
    what you write and what the framework handles.
```

---

## Appendix: SVG Color Theme Template

When requesting diagrams, use these CSS variables for consistency:

```css
/* Primary Color - USE THIS MOST OF THE TIME */
--color-primary: #FA9D1B;       /* Orange - main accent, headers, borders */

/* Secondary Color - USE ONLY WHEN CONTRAST NEEDED */
--color-secondary: #67D4E4;     /* Soft turquoise - outputs, success states */

/* Tertiary Color - USE SPARINGLY */
--color-tertiary: #FFF8F0;      /* Cream - light box backgrounds */

/* Text Colors */
--color-text: #333333;          /* Main text */
--color-text-muted: #666666;    /* Secondary text */
--color-text-light: #999999;    /* Tertiary text */

/* Structural */
--color-border: #FA9D1B;        /* Box borders (use primary) */
--color-background: #FFFFFF;    /* Slide background */
```

**Color Usage Priority:**
1. **Primary (#FA9D1B)** — Use in 80%+ of cases: headers, title underlines, borders, checkmarks, section headers, highlighted boxes, accents
2. **Secondary (#67D4E4)** — Use only for contrast: output vs input, execution vs compilation, cache hit vs miss, success states
3. **Tertiary (#FFF8F0)** — Use sparingly: light box backgrounds, subtle fills

**When to use Secondary:**
- Showing "before → after" transitions
- Distinguishing "input" from "output" 
- Highlighting success/completion states
- Creating visual separation between two related concepts

---

*Template Version: 1.0*
*Compatible with Claude slide generation workflow*

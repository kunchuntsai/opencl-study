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
| **Primary** | #FA9D1B | Main accent color (headers, borders, highlights) |
| **Primary Light** | #FFF8F0 | Light backgrounds, box fills, summary boxes |
| **Text Primary** | #333333 | Main text color |
| **Text Secondary** | #666666 | Subtext, descriptions |
| **Text Light** | #AAAAAA | Grayed-out/inactive items |
| **Background** | #FFFFFF | Slide background |

**Orange Theme — Actual Hex Codes Used:**
```
PRIMARY:        #FA9D1B (Orange)     — Headers, borders, accents, highlights, active items
PRIMARY_LIGHT:  #FFF8F0 (Cream)      — Box backgrounds, summary boxes, section overviews
TEXT_DARK:      #333333 (Dark Gray)  — Main text, titles
TEXT_MUTED:     #666666 (Gray)       — Subtitles, descriptions
TEXT_LIGHT:     #AAAAAA (Light Gray) — Inactive/grayed items in agenda
BACKGROUND:     #FFFFFF (White)      — Slide background
```

> **Usage Guidelines**: 
> - Use **Primary (#FA9D1B)** for headers, title underlines, borders, checkmarks, bullets, highlighted text
> - Use **Primary Light (#FFF8F0)** for box backgrounds, summary boxes, section overview backgrounds
> - **Summary boxes** at bottom of slides should use Primary Light with Primary border (not solid orange)
> - Keep it simple and cohesive with a single accent color

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

## Section 6: Storytelling Structure

For presentations with a narrative arc, define the story structure:

### Narrative Arc

| Phase | Slides | Emotional Goal |
|-------|--------|----------------|
| **Setup** | 1-3 | Establish the problem, create tension |
| **Journey** | 4-X | Show the solution, build understanding |
| **Resolution** | X-End | Deliver payoff, empower action |

**Story Arc Example (60-min technical presentation):**
```
1. FRUSTRATION (slides 1-3): "Yes, I've felt that pain"
2. RELIEF (slides 4-6): "This looks much simpler"  
3. UNDERSTANDING (slides 7-18): "Now I see how it works"
4. CONFIDENCE (slides 19-27): "I can use this in production"
5. EXCITEMENT (slide 28): "I want to try this!"
```

### Key Messages to Reinforce

List 3-5 phrases to repeat throughout:
```
1. "[Core value proposition]" — Say this 3+ times
2. "[Key metric/benefit]" — Memorable statistic  
3. "[Tagline for Part II]" — Second half hook
4. "[Simple summary]" — Easy takeaway
5. "[Call to action]" — What to do next
```

### Audience Engagement Points

```yaml
slide_3:
  action: "Ask question"
  prompt: "How many of you have experienced...?"
  
slide_midpoint:
  action: "Pause for questions"
  note: "Natural break point — genuine pause"

slide_final:
  action: "Seed questions if needed"
  fallback_questions:
    - "A common question is about..."
    - "Some teams ask about..."
    - "You might be wondering about..."
```

### Timing Summary

```yaml
part_1:
  slides: "1-18"
  duration: "30 min"
  
part_2:
  slides: "19-27" 
  duration: "25 min"
  
qa_buffer:
  duration: "5+ min"
  
total: "60 min"
```

---

## Section 7: Speaker Notes Format

For each slide, speaker notes should follow this structure:

```
SLIDE [N]: [Title] ([Duration])

[Main talking points in conversational tone]

• Key point 1
• Key point 2
• Technical detail if needed

TIP: [Delivery advice — gestures, pacing, pauses, emphasis]

TRANSITION: "[Bridge phrase to next slide]"
```

### Speaker Notes Best Practices

| Element | Purpose |
|---------|---------|
| **Timing** | Include duration (e.g., "30 sec", "3 min") |
| **Tone** | Write conversationally — how you'd actually say it |
| **Emphasis** | Mark words/phrases to stress |
| **Pauses** | Note where to pause for effect |
| **Gestures** | Point to diagram, use two hands for comparison |
| **Eye contact** | Remind to look up from notes |
| **Questions** | Mark where to ask audience questions |
| **Transitions** | Smooth bridge phrases between slides |

### Q&A Preparation

Include at end of speaker notes:
```
POTENTIAL Q&A:

Q: [Common question 1]?
A: [Prepared answer]

Q: [Common question 2]?
A: [Prepared answer]

Q: [Technical question]?
A: [Detailed answer with code reference if needed]
```

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
  primary: "#FA9D1B"       # Use everywhere
  primary_light: "#FFF8F0" # Box backgrounds
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
/* Primary Colors */
--color-primary: #FA9D1B;       /* Orange - headers, borders, accents, highlights */
--color-primary-light: #FFF8F0; /* Cream - box backgrounds, summary boxes, section overviews */

/* Text Colors */
--color-text: #333333;          /* Main text, titles */
--color-text-muted: #666666;    /* Secondary text, descriptions */
--color-text-light: #AAAAAA;    /* Inactive/grayed items */

/* Structural */
--color-border: #FA9D1B;        /* Box borders (use primary) */
--color-background: #FFFFFF;    /* Slide background */
```

**Color Usage Summary:**
| Element | Color | Hex |
|---------|-------|-----|
| Title underlines | Primary | #FA9D1B |
| Borders | Primary | #FA9D1B |
| Highlighted text | Primary | #FA9D1B |
| Bullets/checkmarks | Primary | #FA9D1B |
| Box backgrounds | Primary Light | #FFF8F0 |
| Summary boxes | Primary Light + Primary border | #FFF8F0 + #FA9D1B |
| Section overview bg | Primary Light | #FFF8F0 |
| Main text | Text Dark | #333333 |
| Subtitles | Text Muted | #666666 |
| Inactive items | Text Light | #AAAAAA |

---

*Template Version: 1.0*
*Compatible with Claude slide generation workflow*

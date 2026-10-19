# Recibe Master Roadmap

This is the ordered implementation plan for work after v1.26. Work should move
through the phases in sequence because the later user-facing features depend on
the data model and UI foundations established earlier.

When continuing work, use the first phase whose status is **Next** or
**Planned** and whose dependencies are complete.

## Status key

- **Complete** — implemented and verified.
- **Next** — the next phase to implement.
- **Planned** — scoped but waiting on an earlier phase.
- **Future** — useful follow-up work outside the current roadmap.

## Phase 0 — Haiku-native baseline

**Status: Complete in v1.26**

- Replace SQLite with a Haiku Storage Kit recipe store.
- Store categories as directories and recipes as flattened `BMessage` files.
- Add filesystem-backed recipe browsing, searching, editing and category work.
- Correct pagination and build/link dependencies.
- Clarify automatic saving versus recipe export.
- Add Quit and its Command/Alt+Q shortcut.
- Add Australian and British `Favourites` spelling.
- Update the About window, version metadata and application icon transparency.

## Phase 1 — Versioned recipe format and safe writes

**Status: Next**

Prepare the recipe files for pictures, hyperlinks and display formatting before
adding those features to the interface.

### Work

- Add a recipe schema-version field while retaining support for existing v1.26
  recipe files that have no version field.
- Define optional fields for a source URL, picture metadata and text-display
  metadata. Missing optional fields must remain valid.
- Define a stable per-recipe location for managed picture assets; do not depend
  on the original file selected by the user remaining in place.
- Change recipe updates to use a temporary file followed by replacement so a
  failed write cannot destroy the previous recipe.
- Preserve the existing MIME type and searchable `RECIBE:*` attributes.
- Ensure moving, renumbering or deleting a recipe also handles its managed
  assets correctly.
- Add focused round-trip checks for old and new recipe records.

### Done when

- Every existing recipe opens without migration or data loss.
- Opening and saving an old recipe upgrades it safely.
- New optional fields survive save, reopen, category changes and export.
- An interrupted or failed save leaves the prior recipe readable.

## Phase 2 — Cleaner, responsive UI

**Status: Planned**

**Depends on: Phase 1**

Modernise the main window and recipe editor before adding more controls.

### Work

- Replace fixed-coordinate layouts with Haiku Layout Kit layouts where
  practical.
- Make the main window and editor behave correctly with resizing, larger fonts
  and longer localized labels.
- Give the recipe view clear title, category, ingredients and directions
  sections.
- Improve empty, no-selection, searching and error states.
- Keep commands disabled when they cannot act on a recipe.
- Review menu grouping, labels, focus order and keyboard navigation.
- Preserve the existing splitter, favourites and category-browser workflows.

### Done when

- Controls do not overlap or clip at the minimum supported window size.
- The interface remains usable with larger system fonts.
- Add, edit, find, browse, export, print and favourite workflows still work.
- A recipe can be used without relying on mouse-only actions.

## Phase 3 — Rich theme system

**Status: Planned**

**Depends on: Phase 2**

Add a central appearance system so every window and custom-drawn element uses
one consistent, accessible palette.

### Work

- Introduce a shared theme definition built from semantic roles rather than
  hard-coded colours: window, panel, text, secondary text, accent, border,
  selection, title, ingredients, directions, links, warning and error.
- Ship Light and Dark themes plus a System mode that follows Haiku's current
  appearance where the available system APIs allow it.
- Add an Appearance menu with System, Light and Dark choices.
- Persist the selected mode in Recibe's settings.
- Apply theme changes to all open Recibe windows without requiring a restart.
- Keep native controls using Haiku's control look while theming custom views,
  text areas and application-owned backgrounds consistently.
- Route all later coloured text, hyperlink and picture-surround styling through
  the same theme API.
- Verify text, selection and link contrast in every shipped theme.

### Done when

- Main, editor, browser, find, category and About windows remain usable in both
  Light and Dark modes.
- Opening a new window uses the currently selected theme immediately.
- The selected mode survives an application restart.
- No application-owned view relies on an unrelated hard-coded background or
  text colour.

## Phase 4 — Kitchen conversion table

**Status: Planned**

**Depends on: Phases 2–3**

Add a quick conversion window designed for use while reading or editing a
recipe.

### Work

- Add `Tools → Conversion Table…` and a keyboard-accessible conversion window.
- Provide a numeric entry on the left and a live converted result on the right.
- Provide a unit dropdown beside each value and a Swap button between them.
- Group units by compatible measurement type so weight cannot accidentally be
  converted to temperature or volume.
- Support an initial practical kitchen set:
  - Weight: milligrams, grams, kilograms, ounces and pounds.
  - Volume: millilitres, litres, teaspoons, tablespoons, cups and fluid ounces.
  - Temperature: Celsius, Fahrenheit and Kelvin.
- Make ambiguous cooking measures explicit. Australian/metric, US customary
  and UK imperial cups, tablespoons and fluid ounces must be labelled and use
  their correct definitions.
- Support volume-to-weight conversions such as cups to grams through an
  ingredient selector. These conversions must use ingredient-specific density
  data rather than treating every cup as the same weight.
- Include a practical starter density table for common recipe ingredients such
  as water, milk, plain flour, self-raising flour, caster sugar, brown sugar,
  butter, rice and honey.
- Mark ingredient-based results as approximate because packing, temperature,
  moisture and ingredient varieties affect real-world weight.
- Use locale-aware decimal input and output while keeping calculations
  independent of display formatting.
- Provide sensible rounding without discarding the full internal result.
- Add Copy Result and Clear actions.
- Apply the active Recibe theme and remember the last measurement type and
  selected units.

### Done when

- `1 kg` converts to approximately `2.20462 lb` and converts back without
  meaningful precision loss.
- `0 °C` converts to `32 °F`, and `100 °C` converts to `212 °F`.
- Australian and US tablespoons produce different, correctly labelled volume
  results.
- `1` Australian metric cup of water converts to approximately `250 g`, while
  flour and sugar produce their own ingredient-specific results.
- Cups cannot be converted to grams until an ingredient and cup standard have
  been selected.
- Invalid, empty and out-of-range input never crashes the application.
- The complete window is usable from the keyboard in Light and Dark modes.

## Phase 5 — Coloured and structured display text

**Status: Planned**

**Depends on: Phases 1–3**

Introduce a reusable recipe renderer instead of assembling one plain text
buffer in the main view.

### Work

- Render the recipe title, ingredients and directions as distinct sections.
- Apply readable section colours using system-aware defaults.
- Use the same rendering model for on-screen display, export and printing.
- Preserve colours or styles only where the destination format supports them;
  always retain a readable plain-text fallback.
- Decide at the start of this phase whether colour is section-based only or
  author-selectable for individual text ranges. Implement one consistent model.

### Done when

- All recipe sections are visually distinct and readable in supported themes.
- Switching recipes cannot leak styles from the previous recipe.
- Plain-text recipe content remains unchanged and searchable.

## Phase 6 — Hyperlinks

**Status: Planned**

**Depends on: Phase 5**

### Work

- Add an optional recipe source URL to the editor and recipe format.
- Recognise `http://` and `https://` links in recipe text.
- Display links with a consistent link style and pointer feedback.
- Open activated links through Haiku's preferred browser.
- Validate schemes before launching anything; non-web text remains inert.
- Include source links in full-recipe export and printing.

### Done when

- Stored source URLs and inline web links are visibly identifiable and open.
- Invalid or unsupported links cannot launch an application.
- Old recipes without links behave exactly as before.

## Phase 7 — Recipe pictures

**Status: Planned**

**Depends on: Phases 1–3 and 5**

Start with one managed primary picture per recipe. Multiple-picture galleries
can be considered later without changing the base storage design.

### Work

- Add Choose, Replace, Remove and Reveal Picture actions to the editor.
- Import a selected image into the recipe's managed asset location.
- Decode images through the Translation Kit.
- Display a scaled preview while preserving aspect ratio.
- Handle missing, damaged and unsupported images without blocking the recipe.
- Move and delete managed pictures with their recipe.
- Include the primary picture in full-recipe printing when it fits cleanly.

### Done when

- A picture remains available after the original selected file is moved or
  deleted.
- Large images do not distort the UI or exhaust memory unnecessarily.
- Removing a picture cleans up managed data without affecting recipe text.

## Phase 8 — Print modes

**Status: Planned**

**Depends on: Phases 5–7**

### Work

- Add a print-mode chooser with:
  - Full Recipe — title, picture when present, ingredients and instructions.
  - Shopping List — title and ingredients only.
  - Instructions Only — title and directions only.
- Replace the current line-count pagination with a shared page renderer.
- Respect printable margins, page ranges and multi-page recipes.
- Add clear section headings and optional page numbers.
- Keep the user's page setup for subsequent print jobs during the session.

### Done when

- Each mode prints only its promised content.
- Long recipes paginate without clipped or duplicated lines.
- Cancelling page setup or printing leaves the application usable.

## Phase 9 — Integration, accessibility and release polish

**Status: Planned**

**Depends on: Phases 1–8**

### Work

- Exercise add, edit, move, delete, search, favourite, export and all print
  paths with old and new recipe files.
- Verify keyboard shortcuts, focus order, menu states and locale spelling.
- Test missing assets, read-only stores, malformed files and failed writes.
- Check layout and colours with different font sizes and system themes.
- Update screenshots, user documentation, storage documentation and release
  notes.
- Recalculate and apply the next version globally only after the phase is
  accepted.

### Done when

- No known data-loss path remains.
- All roadmap features have a documented manual verification result.
- Application metadata, About text and documentation agree on the release.

## Future work

These ideas are deliberately outside the current first-pass roadmap:

- Multiple pictures or picture galleries per recipe.
- User-created theme files and additional bundled colour schemes.
- A combined shopping list generated from several selected recipes.
- Automatic scaling of all ingredient quantities inside a recipe.
- User-defined ingredients and custom density values for weight/volume
  conversion.
- Tags, ratings and preparation/cooking-time fields.
- HTML or PDF export.
- Automated import of the legacy SQLite database.


# Firmware web appearance

The shared stylesheet in `firmware/src/WebService.cpp` uses the shop palette
recorded in [HardwareDocumentationStyle.md](HardwareDocumentationStyle.md).

Primary buttons, headings and active navigation use dark gold (`#9E6C00`).
Button hover uses a darker gold (`#805700`) to retain contrast with white text.
The sidebar and body text use charcoal (`#262626`); page backgrounds are
neutral light grey (`#F5F5F5`) with white cards and pale gold neutral badges
(`#FAF4E6`). Secondary text is `#595959` and borders are `#D6D6D6`.
Keyboard focus and native checkbox/radio accents follow the primary color.

Success, warning and error colors retain their semantic meaning. Layout,
navigation and firmware behavior are unchanged. The monochrome OLED display
does not have a color palette.

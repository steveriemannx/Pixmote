// Pixmote - compile-time configuration.
#pragma once

// Maximum number of disjoint dirty rectangles kept by Ui.
// When the list is full, rectangles are merged (trade CPU for RAM).
#ifndef PIXMOTE_MAX_DIRTY_RECTS
#define PIXMOTE_MAX_DIRTY_RECTS 4
#endif

#if PIXMOTE_MAX_DIRTY_RECTS < 1 || PIXMOTE_MAX_DIRTY_RECTS > 16
#error "PIXMOTE_MAX_DIRTY_RECTS must be in the range 1..16"
#endif

// Set to 1 to compute rounded-corner spans with libm's sqrt() instead of the
// built-in integer loop. The default keeps the library free of libm; enable
// this only if your toolchain lacks an efficient integer square root.
#ifndef PIXMOTE_USE_LIBM
#define PIXMOTE_USE_LIBM 0
#endif

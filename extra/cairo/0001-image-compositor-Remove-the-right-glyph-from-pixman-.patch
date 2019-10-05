From d0dccb92c520556aaa02bd1fdf8f2922cef00292 Mon Sep 17 00:00:00 2001
From: "Jan Alexander Steffens (heftig)" <jan.steffens@gmail.com>
Date: Sat, 5 Oct 2019 14:07:28 +0200
Subject: [PATCH] image compositor: Remove the right glyph from pixman's cache

We need to use the index including the phase. Otherwise we leave glyphs
in the cache that cause problems later as indices are reused.
---
 src/cairo-image-compositor.c | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)

diff --git a/src/cairo-image-compositor.c b/src/cairo-image-compositor.c
index 6fccb79f1..79ad69f68 100644
--- a/src/cairo-image-compositor.c
+++ b/src/cairo-image-compositor.c
@@ -841,7 +841,7 @@ _cairo_image_scaled_glyph_fini (cairo_scaled_font_t *scaled_font,
     if (global_glyph_cache) {
 	pixman_glyph_cache_remove (
 	    global_glyph_cache, scaled_font,
-	    (void *)_cairo_scaled_glyph_index (scaled_glyph));
+	    (void *)scaled_glyph->hash_entry.hash);
     }
 
     CAIRO_MUTEX_UNLOCK (_cairo_glyph_cache_mutex);
-- 
2.23.0


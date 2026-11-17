#ifdef TRUCE_LVGL
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <cstring>
// LVGL objects, image cache and decoder buffers belong in PSRAM, not radio DRAM.
extern "C" {
void lv_mem_init(void) {}
void lv_mem_deinit(void) {}
lv_mem_pool_t lv_mem_add_pool(void *, size_t) { return nullptr; }
void lv_mem_remove_pool(lv_mem_pool_t) {}
void *lv_malloc_core(size_t n) { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
void *lv_realloc_core(void *p, size_t n) { return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
void lv_free_core(void *p) { heap_caps_free(p); }
void lv_mem_monitor_core(lv_mem_monitor_t *m) {
    memset(m,0,sizeof(*m));
    m->total_size=heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    m->free_size=heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    m->free_biggest_size=heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
}
lv_result_t lv_mem_test_core(void) { return LV_RESULT_OK; }
}
#endif

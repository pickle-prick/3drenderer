#include <assert.h>
#include "light.h"

uint32_t light_apply_intensity(uint32_t original_color, float intensity) {
        assert(intensity >= 0.0 && intensity <= 1.0);
        uint32_t a = original_color & 0xFF000000;
        uint32_t r = (original_color & 0x00FF0000) * intensity;
        uint32_t g = (original_color & 0x0000FF00) * intensity;
        uint32_t b = (original_color & 0x000000FF) * intensity;
        // TODO(@k): not sure if we need to mask sure, check it later
        uint32_t ret = a | (r & 0x00FF0000) | (g & 0x0000FF00) | (b & 0x000000FF);
        return ret;
}

#include "src/tea.h"

#include <math.h>
#define MATH_PI 3.14159

int main(int argc, char ** argv) {
  te_Config conf;
  tea_config_init(&conf, NULL, 640, 380);
  conf.window_flags |= SDL_WINDOW_RESIZABLE;

  tea_init(&conf);

  float x = 0;
	float spd = 5;

  Tea *ctx = tea_context();

  te_Texture *tex = tea_texture_load(ctx, "goblin.png");

  float t = 0;
  int frame = 0;

  while (!tea_should_close(ctx)) {
    tea_begin_render(ctx);


    x += spd * 0.002;
    if (x >= MATH_PI*2) x = 0;
    float s = sinf(x)+1;

    // int triang_sz = 32;

		for (int i = 0; i < 24; i++) {
			float ss = sinf(x + i)+1;
      float sss = sinf(x + 1 + i)+1;
      float x = (i+ss) * 16 - 32;
      float y = 48 + ss * 16;

      float xx = (i+1+sss) * 16 - 32;
      float yy = 48 + sss * 16;
      te_Point v = {x, y};
      te_Point vv = {xx, yy};
      te_Color cc = tea_color(20+ss*107, 50+ss*32, 40+ss*64);
      tea_draw_mode(ctx, TEA_FILL);
      tea_draw_color(ctx, cc);
			// tea_draw_circle(ctx, v, (ss+2)*8);
      int triang_sz = (ss+2)*12;
      // v.x += triang_sz;
      // v.y += triang_sz;
      tea_draw_triangle(ctx, tea_point(v.x-triang_sz, v.y), tea_point(v.x, v.y-triang_sz), tea_point(v.x+triang_sz, v.y));

      v.y += 64;
      vv.y += 64;
      tea_draw_line(ctx, v, vv);

      v.y += 96;
      tea_draw_mode(ctx, TEA_FILL);
      tea_draw_circle(ctx, v, (ss+2)*8);

			// tico_graphics_draw_circle((i+ss) * 16 - 32, 256 + ss*16, (ss+2)*8, tico_color(20+ss*107, 20+ss*32, 40+ss*64));
		}
    // tea_render_draw_color(ctx, tea_color(255, 0, 255));

    // tea_draw_texture(ctx, tex, NULL, tea_point(32, 32));
    s *= 16; 
    tea_draw_mode(ctx, TEA_LINE);
    tea_draw_color(ctx, WHITE);
    tea_draw_triangle(ctx, tea_point(s, 80), tea_point(s + 256, 16), tea_point(s+256, 144));

    tea_draw_mode(ctx, TEA_LINE);
    tea_draw_rect(ctx, 0, 0, 32, 32);
    tea_draw_circle(ctx, (te_Point){320, 190}, 32);

    
    // Draw texture

    t += 0.08f;
    if (t >= 1) {
      t = 0;
      if (frame++ >= 4) frame = 0;
    }

    te_Rect r = tea_rect(16*frame, 0, 16, 16);
    // tea_render_texture(ctx, tex, &r, NULL);
    tea_draw_texture(ctx, tex, &r, tea_point(s+272, 64));

    tea_end_render(ctx);
    // printf("teste\n");
  }

  // SDL_Delay(1000);

  tea_terminate(NULL);

  return 0;
}
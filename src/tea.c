#include "tea.h"

static Tea _ctx;
static te_Config _conf;

int tea_config_init(te_Config *conf, const char *title, int width, int height) {
  if (!conf) conf = &_conf;
  title = title ? title : CAT("tea ", TEA_VERSION);

  if (title) strcpy(conf->title, title);
  conf->width = width;
  conf->height = height;
  conf->flags = SDL_INIT_VIDEO;
  conf->window_flags = SDL_WINDOW_SHOWN;
  conf->render_flags = SDL_RENDERER_ACCELERATED;

  return 1;
}

Tea* tea_init(te_Config *c) {
  // if (!c) c = &_conf;
  TE_ASSERT(c != NULL, "te_Config cannot be NULL");

  SDL_Init(c->flags);
  Tea *ctx = tea_context();
  memset(ctx, 0, sizeof(*ctx));

  ctx->window = tea_window_create(c->title, c->width, c->height, c->window_flags);
  ctx->render = tea_render_create(ctx->window, TEA_SOFTWARE_RENDER);

  ctx->draw.point = tea_render_point;
  ctx->draw.line = tea_render_line;

  ctx->draw.rect[TEA_LINE] = tea_render_rect_line;
  ctx->draw.rect[TEA_FILL] = tea_render_rect_fill;

  ctx->draw.circle[TEA_LINE] = tea_render_circle_line;
  ctx->draw.circle[TEA_FILL] = tea_render_circle_fill;

  ctx->draw.triangle[TEA_LINE] = tea_render_triangle_line;
  ctx->draw.triangle[TEA_FILL] = tea_render_triangle_fill;

  return ctx;
}

Tea* tea_context() {
  return &_ctx;
}

void tea_terminate(Tea *ctx) {
  ctx = ctx ? ctx : &_ctx;

  tea_window_destroy(ctx->window);
  tea_render_destroy(ctx->render);

  SDL_Quit();
}

void tea_push(Tea *ctx, te_Command cmd) {
  ctx->commands[ctx->cp++] = cmd;
}
te_Command* tea_pop(Tea *ctx) {
  return &ctx->commands[--ctx->cp];
}
te_Command* tea_top(Tea *ctx) {
  return &ctx->commands[ctx->cp-1];
}
void tea_repeat(Tea *ctx, int index) {
  tea_push(ctx, ctx->commands[ctx->cp - index]);
}

te_Command tea_command(Tea *ctx, TEA_COMMAND_ type) {
  te_Command cmd = {0};
  cmd.type = type;

  return cmd;
}

// te_Command tea_command_draw(Tea *ctx, TEA_DRAW_COMMAND_ type) {
//   te_Command cmd = {0};
//   cmd.type = TEA_COMMAND_DRAW;

//   cmd.draw.type = type;

//   return cmd;
// }
// te_Command tea_command_stack(Tea *ctx, TEA_STACK_COMMAND_ type) {
//   te_Command cmd = {0};
//   cmd.type = TEA_COMMAND_STACK;

//   cmd.stack.type = type;

//   return cmd;
// }

int tea_should_close(Tea *ctx) {
  return ctx->event.type == SDL_QUIT;
}

void tea_begin_render(Tea *ctx) {
  TE_ASSERT(ctx != NULL, "Tea cannot be NULL");

  ctx->cp = 0;
  SDL_PollEvent(&ctx->event);
  tea_render_clear(ctx, (te_Color){0, 0, 0, 255});
}

static void tea_render_circle(Tea *ctx, te_Command *cmd) {
  ctx->draw.circle[cmd->draw.fill](ctx, cmd->draw.circle.p, cmd->draw.circle.radius);
}

void tea_end_render(Tea *ctx) {
  TE_ASSERT(ctx != NULL, "Tea cannot be NULL");

  // printf("%d\n", ctx->cp);
  for (int i = 0; i < ctx->cp; i++) {
    te_Command *cmd = &ctx->commands[i];
    // if (cmd->type == TEA_COMMAND_DRAW) {
    tea_render_draw_color(ctx, cmd->draw.color);
    // if (cmd->type == TEA_DRAW_TRIANGLE) printf("ok\n");
    switch(cmd->type) {
      case TEA_COMMAND_NONE: break;
      case TEA_DRAW_POINT: tea_render_point(ctx, cmd->draw.point); break;
      case TEA_DRAW_LINE: tea_render_line(ctx, cmd->draw.line.p0, cmd->draw.line.p1); break;
      case TEA_DRAW_CIRCLE: tea_render_circle(ctx, cmd); break;
      case TEA_DRAW_RECT: ctx->draw.rect[cmd->draw.fill](ctx, cmd->draw.rect); break;
      case TEA_DRAW_TRIANGLE: ctx->draw.triangle[cmd->draw.fill](ctx, cmd->draw.triang.p0, cmd->draw.triang.p1, cmd->draw.triang.p2); break;
    }
    // }
  }

  // SDL_RenderPresent(ctx->render->handle);
  tea_render_swap(ctx);
}

void tea_draw_color(Tea *ctx, te_Color color) {
  ctx->_color = color;
}
void tea_draw_mode(Tea *ctx, TEA_DRAW_MODE mode) {
  ctx->_mode = mode;
}

void tea_draw_point(Tea *ctx, te_Point p) {
  // tea_render_draw_color(ctx->render, color);
  // tea_render_point(ctx->render, p);
  // te_Command cmd = tea_command_draw(ctx, DRAW_POINT);
  te_Command cmd = tea_command(ctx, TEA_DRAW_POINT);
  // memcpy(cmd.draw.point, p, sizeof(te_Point));
  cmd.draw.point = p;
  cmd.draw.color = ctx->_color;
  cmd.draw.fill = 0;

  tea_push(ctx, cmd);
}

void tea_draw_line(Tea *ctx, te_Point p0, te_Point p1) {
  // tea_render_draw_color(ctx->render, color);
  // // SDL_RenderDrawLine(ctx->render->handle, p0[0], p0[1], p1[0], p1[1]);
  // tea_render_line(ctx->render, p0, p1);
  // te_Command cmd = tea_command_draw(ctx, DRAW_LINE);
  te_Command cmd = tea_command(ctx, TEA_DRAW_LINE);
  // memcpy(cmd.draw.line.p0, p0, sizeof(te_Point));
  // memcpy(cmd.draw.line.p1, p1, sizeof(te_Point));
  cmd.draw.line.p0 = p0;
  cmd.draw.line.p1 = p1;
  cmd.draw.fill = 0;
  cmd.draw.color = ctx->_color;

  tea_push(ctx, cmd);
}

void tea_draw_rect(Tea *ctx, TEA_VALUE x, TEA_VALUE y, TEA_VALUE w, TEA_VALUE h) {
  // tea_render_draw_color(ctx->render, color);
  // te_Rect r;
  // r.x = x;
  // r.y = y;
  // r.w = w;
  // r.h = h;
  // // SDL_Rect r;
  // // r.x = x;
  // // r.y = y;
  // // r.w = w;
  // // r.h = h;
  // // SDL_RenderDrawRect(ctx->render->handle, &r);
  // switch(mode) {
  //   case TEA_FILL: tea_render_rect_fill(ctx->render, r); break;
  //   case TEA_LINE: tea_render_rect_line(ctx->render, r); break;
  // }
  // te_Command cmd = tea_command_draw(ctx, DRAW_RECT);
  te_Command cmd = tea_command(ctx, TEA_DRAW_RECT);
  cmd.draw.fill = ctx->_mode;
  cmd.draw.color = ctx->_color;
  cmd.draw.rect = tea_rect(x, y, w, h);

  tea_push(ctx, cmd);
}

void tea_draw_circle(Tea *ctx, te_Point p, TEA_VALUE radius) {
//   tea_render_draw_color(ctx->render, color);

//   switch (mode)
//   {
//     case TEA_FILL: tea_render_circle_fill(ctx->render, p, radius); break;
//     case TEA_LINE: tea_render_circle_line(ctx->render, p, radius); break;
  // }
  // te_Command cmd = tea_command_draw(ctx, DRAW_CIRCLE);
  te_Command cmd = tea_command(ctx, TEA_DRAW_CIRCLE);
  cmd.draw.fill = ctx->_mode;
  cmd.draw.color = ctx->_color;
  // memcpy(cmd.draw.circle.p, p, sizeof(te_Point));
  cmd.draw.circle.p = p;
  cmd.draw.circle.radius = radius;

  tea_push(ctx, cmd);
}

void tea_draw_triangle(Tea *ctx, te_Point p0, te_Point p1, te_Point p2) {
  // te_Command cmd = tea_command_draw(ctx, DRAW_TRIANGLE);
  te_Command cmd = tea_command(ctx, TEA_DRAW_TRIANGLE);
  cmd.draw.fill = ctx->_mode;
  cmd.draw.color = ctx->_color;
  cmd.draw.triang.p0 = p0;
  cmd.draw.triang.p1 = p1;
  cmd.draw.triang.p2 = p2;
  // memcpy(cmd.draw.triang.p0, p0, sizeof(te_Point));
  // memcpy(cmd.draw.triang.p1, p1, sizeof(te_Point));
  // memcpy(cmd.draw.triang.p2, p2, sizeof(te_Point));

  tea_push(ctx, cmd);
}

te_Window* tea_window_create(const char *title, int width, int height, int flags) {
  // te_Window *w = (te_Window*)malloc(sizeof(*w));
  // TE_ASSERT(w != NULL, "failed to create window");

  // w->handle = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);

  // w->width = width;
  // w->height = height;
  te_Window *w = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);

  return w;
}

void tea_window_destroy(te_Window *window) {
  if (!window) return;

  SDL_DestroyWindow(window);
}

te_Render* tea_render_create(te_Window *window, TEA_RENDER_FLAGS flags) {
  TE_ASSERT(window != NULL, "te_Window cannot be NULL");
  te_Render *r = SDL_CreateRenderer(window, -1, flags);

  // te_Render *r = (te_Render*)malloc(sizeof(*r));
  // TE_ASSERT(r != NULL, "failed to create render");

  // r->handle = SDL_CreateRenderer(window->handle, -1, flags);

  // r->draw.point = tea_render_point;
  // r->draw.line = tea_render_line;

  // r->draw.rect[TEA_LINE] = tea_render_rect_line;
  // r->draw.rect[TEA_FILL] = tea_render_rect_fill;

  // r->draw.circle[TEA_LINE] = tea_render_circle_line;
  // r->draw.circle[TEA_FILL] = tea_render_circle_fill;

  // r->draw.triangle[TEA_LINE] = tea_render_triangle_line;
  // r->draw.triangle[TEA_FILL] = tea_render_triangle_line;

  return r;
}

void tea_render_destroy(te_Render *render) {
  if (!render) return;

  SDL_DestroyRenderer(render);
  // free(render);
}

void tea_render_draw_color(Tea *t, te_Color color) {
  SDL_SetRenderDrawColor(t->render, color.r, color.g, color.b, color.a);
}

void tea_render_clear(Tea *t, te_Color color) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");

  SDL_SetRenderDrawColor(t->render, color.r, color.g, color.b, color.a);
  SDL_RenderClear(t->render);
}

void tea_render_swap(Tea *t) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_RenderPresent(t->render);
}

void tea_render_point(Tea *t, te_Point p) {
  // tea_render_draw_color(render, color);
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_RenderDrawPoint(t->render, p.x, p.y);
}
void tea_render_line(Tea *t, te_Point p0, te_Point p1) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_RenderDrawLine(t->render, p0.x, p0.y, p1.x, p1.y);
}

void tea_render_rect_fill(RECT_ARGS) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_Rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  SDL_RenderFillRect(t->render, &r);
}
void tea_render_rect_line(RECT_ARGS) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_Rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  SDL_RenderDrawRect(t->render, &r);
}

void tea_render_circle_fill(CIRC_ARGS) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");

  int x = 0;
	int y = radius;

	int P = 1 - radius;

	if (radius > 0) SDL_RenderDrawLine(t->render, p.x + radius, p.y, p.x - radius, p.y);

	while (x <= y) {
		if (P < 0) P += 2*x + 3;
		else {
			P += (2*(x-y))+5;
			y--;
		}
		x++;

		if (x > y) break;

		SDL_RenderDrawLine(t->render, p.x - x, p.y + y, p.x + x, p.y + y);
		SDL_RenderDrawLine(t->render, p.x + x, p.y - y, p.x - x, p.y - y);

		if (x != y) {
			SDL_RenderDrawLine(t->render, p.x - y, p.y + x, p.x + y, p.y + x);
			SDL_RenderDrawLine(t->render, p.x + y, p.y - x, p.x - y, p.y - x);
		}
	}
}

void tea_render_circle_line(CIRC_ARGS) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");

  int x = -radius;
	int y = 0;
	int r = radius;
	int err = 2 - 2*r;

	do {
		SDL_RenderDrawPoint(t->render, p.x - x, p.y + y);
		SDL_RenderDrawPoint(t->render, p.x - y, p.y - x);
		SDL_RenderDrawPoint(t->render, p.x + x, p.y - y);
		SDL_RenderDrawPoint(t->render, p.x + y, p.y + x);
		r = err;
		if (r <= y) err += ++y*2+1;
		if (r > x || err > y) err += ++x*2+1;
	} while (x < 0);
}

static int cross_prod(te_Point v0, te_Point v1) {
  return v0.x*v1.y - v0.y*v1.x;
}

static int get_intersection(te_Point p0, te_Point p1, te_Point p2, te_Point p3, te_Point *out) {
  if (!out) return -1;
  te_Point d0 = tea_point(p0.x-p1.x, p0.y-p1.y);
  te_Point d1 = tea_point(p2.x-p3.x, p2.y-p3.y);
  int cross = cross_prod(d0, d1);

  if (cross == 0) return 0;

  float t = cross_prod(tea_point(p0.x-p2.x, p0.y-p2.y), tea_point(p2.x-p3.x, p2.y-p3.y)) / (float)cross;
  float u = cross_prod(tea_point(p0.x-p1.x, p0.y-p1.y), tea_point(p0.x-p2.x, p0.y-p2.y)) / (float)cross;
  te_Point o;

  if (t <= 0 && t <= 1) o = tea_point(p0.x+t*(p1.x-p0.x), p0.y+t*(p1.y-p0.y));
  else if (u <= 0 && u <= 1) o = tea_point(p2.x+u*(p3.x-p2.x), p2.y+u*(p3.y-p2.y));
  else return 0;

  // printf("%f %f\n", out->x, out->y);
  if (out) *out = o;

  return 1;
}

void tea_render_triangle_fill(TRIANG_ARGS) {
  // tea_render_triangle_line(t, p0, p1, p2);
  Tea *tea = t;
  int minX = tea_min(p0.x, tea_min(p1.x, p2.x));
  int maxX = tea_max(p0.x, tea_max(p1.x, p2.x));

  int minY = tea_min(p0.y, tea_min(p1.y, p2.y));
  int maxY = tea_max(p0.y, tea_max(p1.y, p2.y));

  int x, y;
  for (y = minY; y <= maxY; y++) {
    // for (x = minX; x <= maxX; x++) {

      // te_Point q = tea_point(p0.x, y - p0.y);

      te_Point sp0 = tea_point(minX, y);
      te_Point sp1 = tea_point(maxX, y);
      int intersect = 0;
      te_Point inter_points[2];

      // SDL_RenderDrawLine(tea->render, sp0.x, sp0.y, pp1.x, pp1.y);


      // int cross = cross_prod(tea_point(p0.x-p1.x, pp0.y-pp1.y), tea_point(p0.y-p1.y, pp0.x-pp1.x));
      // int cross = get_intersection(pp0, pp1, p0, p1, &inter_points[intersect]);
      if (get_intersection(p1, p0, sp0, sp1, &inter_points[intersect])) intersect++;
      // printf("%d\n", intersect);
      if (get_intersection(p2, p1, sp0, sp1, &inter_points[intersect])) intersect++;
      // printf("%d\n", intersect);
      if (get_intersection(p0, p2, sp0, sp1, &inter_points[intersect])) intersect++;
      // printf("%d\n", intersect);

      // printf("%d\n", intersect);


      for (int i = 0; i < intersect; i++) {
        printf("p%d: %f %f\n", i, inter_points[i].x, inter_points[i].y);
      }
      // if (intersect > 0) 
        // SDL_RenderDrawLine(tea->render, inter_points[0].x, inter_points[0].y, inter_points[1].x, inter_points[1].y);
      switch (intersect) {
        case 0: break;
        case 1: SDL_RenderDrawPoint(tea->render, inter_points[0].x, inter_points[0].y); break;
        case 2: SDL_RenderDrawLine(tea->render, inter_points[0].x+250, inter_points[0].y, inter_points[1].x+250, inter_points[1].y); break;
      }

      // float s = (float)cross_prod(q, vs1) / c;
      // float tt = (float)cross_prod(vs0, q) / c;

      // if (s >= 0 && tt >= 0 && (s + tt <= 1)) SDL_RenderDrawPoint(t->render, x, y);

      // if ((p0.x - p1.x) * (y - p0.y) - (p0.y - p1.y) * (x - p0.x) > 0 &&
      //     (p1.x - p2.x) * (y - p1.y) - (p1.y - p2.y) * (x - p1.x) > 0 &&
      //     (p2.x - p0.x) * (y - p2.y) - (p2.y - p0.y) * (x - p2.x) > 0){
      //       printf("porra eh, entra aqui n?\n");
      //       SDL_RenderDrawPoint(t->render, x, y);
      // }
    // }
  }
}
void tea_render_triangle_line(TRIANG_ARGS) {
  // printf("qqqq\n");
  tea_render_line(t, p0, p1);
  tea_render_line(t, p1, p2);
  tea_render_line(t, p2, p0);
}
void tea_render_texture(Tea *t);
void tea_render_texture_ex(Tea *t);

/* Debug */

void tea_error(const char *msg) {
  fprintf(stderr, "[tea] error: %s\n", msg);
  exit(1);
}
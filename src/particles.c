#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

/*
 * TODO: If x,y,dx,dy are doubles we can have more control
 * over how the particles move.
 */
struct particle {
	float x, y;
	float dx, dy;
	int life;
	int color;
	
	struct particle *next;
};

static struct particle *particles = NULL;

void add_particle(float x, float y, float dx, float dy, int life, int color) {
	struct particle *p = malloc(sizeof *p);
	
	p->x = x;
	p->y = y;
	p->dx = dx;
	p->dy = dy;
	
	if(life <= 0) life = 1;
	p->life = life;
	p->color = color;
	
	p->next = particles;	
	particles = p;
}

void update_particles(struct bitmap *b, int scroll_x, int scroll_y) {
	struct particle *p = particles, *prev = NULL;
	while(p) {
		p->x += p->dx;
		p->y += p->dy;
		
		// FIXME: What about scroll_x and scroll_y
		if(p->x >= 0 && p->x < b->w && p->y >= 0 && p->y < b->h) {
			bm_set_color_i(b, p->color);
			bm_putpixel(b, p->x, p->y);
		}
		
		if(--p->life > 0) {
			prev = p;
			p = p->next;
		} else {
			struct particle *q = p->next;
			if(prev) {
				prev->next = p->next;				
			} else {
				particles = p->next;
			}
			free(p);
			p = q;
		}
	}
}

void clear_particles() {
	while(particles) {
		struct particle *p = particles;
		particles = particles->next;
		free(p);
	}
}

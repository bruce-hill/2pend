// A double pendulum simulator by Bruce Hill
// Released under the MIT license, see LICENSE for details.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termbox.h>
#include <unistd.h>
#include <math.h>

#define HISTORY_LEN 8

typedef struct {
    double a, p;
} pendulum_t;

typedef struct {
    pendulum_t p1, p2;
} state_t;

static const double M1 = 1.0, M2 = 1.0, G = 9.80, L1 = .5, L2 = .5;

const int color_ramp[] = {16,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,
    247,248,249,250,251,252,253,254,255,231,231,231,231,231};

static state_t get_derivative(state_t state)
{
    double a1 = state.p1.a, a2 = state.p2.a, p1 = state.p1.p, p2 = state.p2.p;
    double da1, da2, dp1, dp2;
    da1 = (6/(M1*L1*L1)) * (2*p1 - 3*cos(a1-a2)*p2)/(16-9*cos(a1-a2)*cos(a1-a2));
    da2 = (6/(M2*L2*L2)) * (8*p2 - 3*cos(a1-a2)*p1)/(16-9*cos(a1-a2)*cos(a1-a2));
    dp1 = -.5 * M1*L1*L1 * (da1*da2*sin(a1-a2) + 3*G/L1*sin(a1));
    dp2 = -.5 * M2*L2*L2 * (-da1*da2*sin(a1-a2) + G/L1*sin(a2));
    state_t derivative = {{da1, dp1}, {da2, dp2}};
    return derivative;
}

static state_t step(state_t state, state_t derivative, double dt)
{
    state_t next = {
        {state.p1.a+dt*derivative.p1.a, state.p1.p+dt*derivative.p1.p},
        {state.p2.a+dt*derivative.p2.a, state.p2.p+dt*derivative.p2.p}
    };
    return next;
}

static void draw(state_t state, int color)
{
    // TODO: properly walk along cells
    int W = tb_width(), H = tb_height();
    double *brightness = calloc(W*H, sizeof(double));
    double len = W/2 > H ? H/2 : (W/2)/2;
    double x = W/2, y = H/2;
    double step = 0.25;
    for (double p = 0; p <= len*L1; p += step) {
        double b = (sqrt(2*((x-floor(x+.5))*(x-floor(x+.5)) + (y-floor(y+.5))*(y-floor(y+.5)))));
        if (b > brightness[(int)y*W+(int)x]) brightness[(int)y*W+(int)x] = b;
        b = (sqrt(2*((x+1.-floor(x+1.5))*(x+1.-floor(x+1.5)) + (y-floor(y+.5))*(y-floor(y+.5)))));
        if (b > brightness[(int)y*W+(int)x+1]) brightness[(int)y*W+(int)x+1] = b;
        //tb_change_cell((int)x, (int)y, ' ', 0, color);
        x += step*cos(state.p1.a + M_PI/2)*2;
        y += step*sin(state.p1.a + M_PI/2);
    }
    for (double p = 0; p <= len*L2; p += step) {
        double b = (sqrt(2*((x-floor(x+.5))*(x-floor(x+.5)) + (y-floor(y+.5))*(y-floor(y+.5)))));
        if (b > brightness[(int)y*W+(int)x]) brightness[(int)y*W+(int)x] = b;
        b = (sqrt(2*((x+1.-floor(x+1.5))*(x+1.-floor(x+1.5)) + (y-floor(y+.5))*(y-floor(y+.5)))));
        if (b > brightness[(int)y*W+(int)x+1]) brightness[(int)y*W+(int)x+1] = b;
        //tb_change_cell((int)x, (int)y, ' ', 0, color);
        x += step*cos(state.p2.a + M_PI/2)*2;
        y += step*sin(state.p2.a + M_PI/2);
    }

    for (int y=0; y<H; y++) for (int x=0; x<W; x++) {
        double b = brightness[y*W+x];
        if (b > 0) {
            color = color_ramp[(int)(sizeof(color_ramp)/sizeof(int) * b)];
            tb_change_cell((int)x, (int)y, ' ', 0, color);
        }
    }
    free(brightness);
}

int main(int argc, char **argv) {
    int ret = tb_init();
    if (ret) {
        fprintf(stderr, "tb_init() failed with error code %d\n", ret);
        return 1;
    }
    tb_select_input_mode(TB_INPUT_MOUSE);
    tb_select_output_mode(TB_OUTPUT_256);

    state_t state;
    state_t history[HISTORY_LEN];

  randomize:
    state.p1.a = M_PI/2 + M_PI*(double)rand()/RAND_MAX;
    state.p2.a = M_PI/2 + M_PI*(double)rand()/RAND_MAX;
    state.p1.p = 0;
    state.p2.p = 0;
    for (int i=0; i<HISTORY_LEN; i++) {
        history[i] = state;
    }

    while (1) {
        tb_clear();
        const int steps = 5;
        double dt = (1./60.) / (double)steps;
        for (int i = 0; i < steps; i++) {
            // RK4 integration
            state_t k1 = get_derivative(state);
            state_t k2 = get_derivative(step(state, k1, dt/2.));
            state_t k3 = get_derivative(step(state, k2, dt/2.));
            state_t k4 = get_derivative(step(state, k3, dt));
            state.p1.a += dt/6.*(k1.p1.a + 2.*k2.p1.a + 2.*k3.p1.a + k4.p1.a);
            state.p2.a += dt/6.*(k1.p2.a + 2.*k2.p2.a + 2.*k3.p2.a + k4.p2.a);
            state.p1.p += dt/6.*(k1.p1.p + 2.*k2.p1.p + 2.*k3.p1.p + k4.p1.p);
            state.p2.p += dt/6.*(k1.p2.p + 2.*k2.p2.p + 2.*k3.p2.p + k4.p2.p);
        }
        for (int i=0; i<HISTORY_LEN-1; i++) {
            history[i] = history[i+1];
        }
        history[HISTORY_LEN-1] = state;
        /*
        for (int i=0; i<HISTORY_LEN; i++) {
            //draw(history[i], color_ramp[((i+1)*(sizeof(color_ramp)/sizeof(int)))/(HISTORY_LEN+1)]);
            draw(history[i], color_ramp[((i+1)*(sizeof(color_ramp)/sizeof(int)/2))/(HISTORY_LEN+1)]);
        }
        */
        draw(state, 231);
        //draw(state, 7);
        tb_present();
        usleep(60000);
        struct tb_event ev;
        while (tb_peek_event(&ev, 0)) {
            switch (ev.type) {
                case TB_EVENT_KEY:
                    if (ev.key == TB_KEY_ESC && tb_peek_event(&ev, 50) == 0)
                        goto done;
                    else if (ev.ch == 'q')
                        goto done;
                    else if (ev.ch == 'r')
                        goto randomize;
                    break;
            }
        }
    }
  done:
    tb_shutdown();
    return 0;
}

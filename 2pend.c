// A double pendulum simulator by Bruce Hill
// Released under the MIT license, see LICENSE for details.
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "bterm.h"

typedef struct {
    double a, p;
} pendulum_t;

typedef struct {
    pendulum_t p1, p2;
} state_t;

static const double M1 = 1.0, M2 = 1.0, G = 9.80, L1 = .5, L2 = .5;
struct winsize wsize = {0};

const int color_ramp[] = {16,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,
    247,248,249,250,251,252,253,254,255,231,231,231,231,231};

static FILE *tty_out = NULL;
int ttyin_fd = 0;

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

static inline void draw_pix(double x, double y, int W, int H, double *prev)
{
    double b = sqrt(2*((x-floor(x+.5))*(x-floor(x+.5)) + (y-floor(y+.5))*(y-floor(y+.5))));
    int ix = (int)x, iy = (int)y;
    if (b > prev[iy*W+ix] && b > .1) {
        prev[iy*W+ix] = b;
        int color = color_ramp[(int)(sizeof(color_ramp)/sizeof(int) * b)];
        fprintf(tty_out, "\033[%d;%dH\033[48;5;%dm \033[0m", iy+1, ix+1, color);
    }
}

static void draw(state_t state)
{
    // TODO: properly walk along cells
    int W = wsize.ws_col, H = wsize.ws_row;
    double *brightness = calloc(W*H, sizeof(double));
    double len = W/2 > H ? H/2 : (W/2)/2;
    double x = W/2, y = H/2;
    double step = 0.25;
    for (double p = 0; p <= len*L1; p += step) {
        draw_pix(x, y, W, H, brightness);
        draw_pix(x+1.0, y, W, H, brightness);
        x += step*cos(state.p1.a + M_PI/2)*2;
        y += step*sin(state.p1.a + M_PI/2);
    }
    for (double p = 0; p <= len*L2; p += step) {
        draw_pix(x, y, W, H, brightness);
        draw_pix(x+1.0, y, W, H, brightness);
        x += step*cos(state.p2.a + M_PI/2)*2;
        y += step*sin(state.p2.a + M_PI/2);
    }
    free(brightness);
}

int main(int argc, char **argv)
{
    int ret = 0;
    ttyin_fd = open("/dev/tty", O_RDONLY | O_NONBLOCK);
    tty_out = fopen("/dev/tty", "w");

    struct termios orig_termios, termios;
    tcgetattr(fileno(tty_out), &orig_termios);
    memcpy(&termios, &orig_termios, sizeof(termios));
    cfmakeraw(&termios);
    termios.c_cc[VMIN] = 10;
    termios.c_cc[VTIME] = 1;
    if (tcsetattr(fileno(tty_out), TCSAFLUSH, &termios) == -1)
        return 1;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);
    fputs(T_OFF(T_SHOW_CURSOR) T_ON(T_ALT_SCREEN), tty_out);

    state_t state;

  randomize:
    state.p1.a = M_PI*7/8 + M_PI/4*(double)rand()/RAND_MAX;
    state.p2.a = M_PI*7/8 + M_PI/4*(double)rand()/RAND_MAX;
    state.p1.p = 0;
    state.p2.p = 0;

    while (1) {
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
        fputs("\033[2J", tty_out);
        draw(state);
        fflush(tty_out);
        usleep(60000);
        int key;
        key = bgetkey(ttyin_fd, NULL, NULL);
        switch (key) {
            case KEY_CTRL_C: ret = 1; goto done;
            case KEY_ESC: case 'q': goto done;
            case 'r': goto randomize;
        }
    }
  done:
    tcsetattr(fileno(tty_out), TCSAFLUSH, &orig_termios);
    fputs(T_ON(T_SHOW_CURSOR) T_OFF(T_ALT_SCREEN), tty_out);
    fclose(tty_out);
    close(ttyin_fd);
    return ret;
}

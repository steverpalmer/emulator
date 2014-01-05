/*
 * screen.c
 *
 *  Created on: 4 Apr 2012
 *      Author: steve
 */

#include <stdlib.h>
#include <math.h>
#include <SDL.h>

#include "screen.h"

#include "assert.h"

#define SCR_FNAME "mc6847.bmp"

static log4c_category_t *ctracelog;

#define SCR_CTRACE_LOG(...) CTRACE_LOG(ctracelog, __VA_ARGS__)

static int ScrRefreshRate_ms = 8;

struct My_Scr {
        SDL_Surface *screen;
        Uint32      colour[4];
        ScreenMode  mode;
        byte      ram[0x1800];
        byte      rendered[0x1800];
        SDL_TimerID timer;
        int         callback_count;
        /* Mode 0 Stuff */
        int         character_w;
        int         character_h;
        SDL_Surface *glyph[256];
        /* Mode 1-4 Stuff */
        int map_x[257];
        int map_y[193];
        struct Resolution {
                int     max_x;
                int     max_y;
                int     bits_per_pixel;
                word mem_used;
                int     *map_x;
                int     *map_y;
                int     pixels_per_byte;
                byte  bit_mask;
        } graphics[SCR_LAST];
};


/******************************************************************************/
static SDL_Surface *scale_and_convert_surface(Scr scr, SDL_Surface *src)
/******************************************************************************/
{
        SCR_CTRACE_LOG("scale_and_convert_surface");
        int rv;
        SDL_Surface *result = 0;
        assert (src->format->BitsPerPixel == 8);
        SDL_Surface *scaled_glyph = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                                                     scr->character_w,
                                                                     scr->character_h,
                                                                     src->format->BitsPerPixel,
                                                                     src->format->Rmask,
                                                                     src->format->Gmask,
                                                                     src->format->Bmask,
                                                                     src->format->Amask);
        assert (scaled_glyph);
        if (src->format->BitsPerPixel == 8)
                SDL_SetPalette(scaled_glyph,
                                SDL_LOGPAL,
                                src->format->palette->colors,
                                0,
                                src->format->palette->ncolors);
        rv = SDL_LockSurface(scaled_glyph);
        assert (!rv);
        byte *scaled_pixels;
        int x, y;
        for (scaled_pixels=scaled_glyph->pixels, y = 0; y < scaled_glyph->h; y++) {
                const int src_y = (src->h * y) / scaled_glyph->h;
                const byte *src_pixels_offset = ((byte *)src->pixels) + (src_y * src->pitch);
                for (x = 0; x < scaled_glyph->w; x++, scaled_pixels++) {
                        const int src_x = (src->w * x) / scaled_glyph->w;
                        *scaled_pixels = src_pixels_offset[src_x];
                }
        }
        SDL_UnlockSurface(scaled_glyph);
        result = SDL_DisplayFormat(scaled_glyph);
        SDL_FreeSurface(scaled_glyph);
        return result;
}

/******************************************************************************/
static Uint32 callback(Uint32 interval, void *param)
/******************************************************************************/
{
        /* SCR_CTRACE_LOG("callback"); can't afford the time to log this */
        Scr scr = param;
        if (++scr->callback_count == 2) {
                scr->callback_count = 0;
                static SDL_UserEvent timer_event = {SDL_USEREVENT, 0, 0, 0 };
                int rv = SDL_PushEvent((SDL_Event *)&timer_event);
                assert (!rv);
        }
        return interval;
}

/******************************************************************************/
static void render_mode0 ( Scr scr )
/******************************************************************************/
{
        SCR_CTRACE_LOG("render_mode0");
        SDL_Rect pos;
        int row, col;
        pos.w = scr->character_w;
        pos.h = scr->character_h;
        pos.y = 0;
        pos.x = 0;
        int addr;
        for (addr = 0, row = 0, pos.y = 0; row < 16; row++, pos.y += pos.h)
                for (col = 0, pos.x = 0; col < 32; col++, addr++, pos.x += pos.w) {
                        const byte ch = scr->ram[addr];
                        if (ch != scr->rendered[addr])
                        {
                                SDL_Surface * glyph = scr->glyph[ch];
                                assert (glyph);
                                SDL_BlitSurface(glyph, 0, scr->screen, &pos);
                                scr->rendered[addr] = ch;
                                SDL_UpdateRect(scr->screen, pos.x, pos.y, pos.w, pos.h);
                        }
                }
}

/******************************************************************************/
void graphics_precalcs(struct Resolution *p, int max_x, int max_y, int bits_per_pixel)
/******************************************************************************/
{
        p->max_x = max_x;
        p->max_y = max_y;
        p->bits_per_pixel = bits_per_pixel;
        p->mem_used = (max_x * max_y * bits_per_pixel) / 8;
        /* map max_x, max_y to 256,192 coordinates */
        p->map_x = calloc(max_x + 1, sizeof(*p->map_x));
        int x;
        for (x = 0; x <= max_x; x++)
                p->map_x[x] = (256 * x) / max_x;
        p->map_y = calloc(max_y + 1, sizeof(*p->map_y));
        int y;
        for (y = 0; y <= max_y; y++)
                p->map_y[y] = (192 * y) / max_y;
        p->pixels_per_byte = 8 / bits_per_pixel;
        p->bit_mask = 0xFF << (8-bits_per_pixel) & 0xFF;
}

/******************************************************************************/
static void render_graphics ( Scr scr )
/******************************************************************************/
{
        SCR_CTRACE_LOG("render_graphics");
        assert (scr->mode != SCR_MODE0 );
        SDL_Rect pos;
        struct Resolution *res_p = &scr->graphics[scr->mode];
        int row = 0, col = 0;
        int addr;
        for (addr = 0; addr < res_p->mem_used; addr++) /* For each byte in memory */
        {
                word byte = scr->ram[addr];
                word changes = byte ^ scr->rendered[addr];
                if (changes) /* If the byte has changed */
                {
                        scr->rendered[addr] = byte;
                        pos.y = scr->map_y[res_p->map_y[row]];
                        pos.h = scr->map_y[res_p->map_y[row+1]] - pos.y;
                        int bit_cnt;
                        for (bit_cnt = 0; bit_cnt < res_p->pixels_per_byte; bit_cnt++, col++) /* For each bit */
                        {
                                byte    <<= res_p->bits_per_pixel;
                                changes <<= res_p->bits_per_pixel;
                                if (changes >= 256) /* If the bit(s) have changed */
                                {
                                        pos.x = scr->map_x[res_p->map_x[col]];
                                        pos.w = scr->map_x[res_p->map_x[col+1]] - pos.x;
                                        const int rv = SDL_FillRect(scr->screen, &pos, scr->colour[byte >> 8]);
                                        assert (!rv);
                                        SDL_UpdateRect(scr->screen, pos.x, pos.y, pos.w, pos.h);
                                        changes &= 0xFF;
                                }
                                byte &= 0xFF;
                        }
                }
                else
                        col += res_p->pixels_per_byte;
                if (col == res_p->max_x) {
                        col = 0;
                        row += 1;
                }
        }
}

/******************************************************************************/
void scr_init(void)
/******************************************************************************/
{
        ctracelog = log4c_category_get(LOGNAME_CTRACE("screen"));
        SCR_CTRACE_LOG("scr_init");
}

/******************************************************************************/
Scr scr_new ( float scale )
/******************************************************************************/
/*
 * TODO: It would be better to allow the screen to be resized on the fly (#5)
 */
{
        SCR_CTRACE_LOG("scr_new(%f)", scale);
        if (!scale)
                scale = 2.0;
        assert (scale >= 1.0);
    Scr result = 0;
    result = malloc(sizeof(*result));
    if (result) {
        result->character_w = floor(8 * scale);
        result->character_h = floor(12 * scale);
        result->screen = SDL_SetVideoMode( result->character_w * 32,
                                                   result->character_h * 16,
                                                   0,
                                                   SDL_SWSURFACE | SDL_ANYFORMAT );
        assert (result->screen);
        SDL_WM_SetCaption("Acorn Atom", "Acorn Atom");
        result->colour[0] = SDL_MapRGB(result->screen->format,   0,   0,   0); /* Black */
        result->colour[1] = SDL_MapRGB(result->screen->format, 255, 255, 255); /* White */
        result->colour[2] = SDL_MapRGB(result->screen->format, 255,   0,   0); /* Red */
        result->colour[1] = SDL_MapRGB(result->screen->format,   0, 255,   0); /* Blue */
        { /* Construct Characters for Mode 0 */
                SDL_Surface *BaseFont = SDL_LoadBMP(SCR_FNAME);
                assert (BaseFont);
                int rv;
                /* TODO: Fix Colour (#15) */
                SDL_Rect src_pos;
                src_pos.x = 0;
                src_pos.y = 0;
                src_pos.h = 12;
                src_pos.w = 8;
                SDL_Surface *original_glyph = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                                                               src_pos.w,
                                                                               src_pos.h,
                                                                               BaseFont->format->BitsPerPixel,
                                                                               BaseFont->format->Rmask,
                                                                               BaseFont->format->Gmask,
                                                                               BaseFont->format->Bmask,
                                                                               BaseFont->format->Amask);
                assert (original_glyph);
                if (BaseFont->format->BitsPerPixel == 8)
                        SDL_SetPalette(original_glyph,
                                                   SDL_LOGPAL,
                                                   BaseFont->format->palette->colors,
                                                   0,
                                                   BaseFont->format->palette->ncolors);
                int ch;
                for (ch = 0; ch < 256; ch++) {
                        rv = SDL_BlitSurface(BaseFont, &src_pos, original_glyph, 0);
                        assert (!rv);
                        result->glyph[ch] = scale_and_convert_surface(result, original_glyph);
                        assert (result->glyph[ch]);
                        src_pos.x += src_pos.w;
                        if (src_pos.x >= BaseFont->w) {
                                src_pos.x = 0;
                                src_pos.y += src_pos.h;
                        }
                }
                SDL_FreeSurface(original_glyph);
        }
        { /* Construct Pixel mapping for Graphics Modes */
                /* Map a 256,192 screen to the SDL surface coordinates */
                int x, y;
                for (x=0; x<=256; x++)
                        result->map_x[x] = (result->character_w * 32 * x) / 256;
                for (y=0; y<=192; y++)
                        result->map_y[y] = (result->character_h * 16 * y) / 192;
                graphics_precalcs(&result->graphics[SCR_MODE4],  256, 192, 1);
                graphics_precalcs(&result->graphics[SCR_MODE4A], 128, 192, 2);
                graphics_precalcs(&result->graphics[SCR_MODE3],  128, 192, 1);
                graphics_precalcs(&result->graphics[SCR_MODE3A], 128,  96, 2);
                graphics_precalcs(&result->graphics[SCR_MODE2],  128,  96, 1);
                graphics_precalcs(&result->graphics[SCR_MODE2A], 128,  64, 2);
                graphics_precalcs(&result->graphics[SCR_MODE1],  128,  64, 1);
                graphics_precalcs(&result->graphics[SCR_MODE1A],  64,  64, 2);
                graphics_precalcs(&result->graphics[SCR_MODE0],   64,  48, 4);
        }
        { /* Initialise the memory */
                int addr;
                for (addr = 0; addr < 0x1800; addr++) {
                        result->ram[addr] = 0;
                        result->rendered[addr] = -1;
                }
        }
        result->mode = SCR_MODE0;
        result->callback_count = 0;
        result->timer = SDL_AddTimer(ScrRefreshRate_ms, callback, (void *)result);
        assert (result->timer);
    }
    return result;
}

/******************************************************************************/
bool scr_del ( Scr scr )
/******************************************************************************/
{
        SCR_CTRACE_LOG("scr_del");
    bool result = false;
    if (scr) {
        int i;
        for (i = SCR_MODE0; i < SCR_LAST; i++) {
                free (scr->graphics[i].map_y);
                free (scr->graphics[i].map_x);
        }
        SDL_RemoveTimer(scr->timer);
        for (i = 0; i < 256; i++)
                if (scr->glyph[i])
                        SDL_FreeSurface(scr->glyph[i]);
        SDL_FreeSurface(scr->screen);
        free(scr);
        result = true;
    }
    return result;
}

/******************************************************************************/
void scr_deinit(void)
/******************************************************************************/
{
        SCR_CTRACE_LOG("scr_deinit");
}

/******************************************************************************/
void scr_dump(log4c_category_t *log, Scr scr)
/******************************************************************************/
{
        SCR_CTRACE_LOG("scr_dump");
}

/******************************************************************************/
byte *scr_ram(Scr scr)
/******************************************************************************/
{
        SCR_CTRACE_LOG("scr_ram");
        return scr->ram;
}

/******************************************************************************/
void scr_set_mode(Scr scr, ScreenMode mode)
/******************************************************************************/
{
        SCR_CTRACE_LOG("scr_set_mode(%d)", mode);
        if (scr->mode != mode){
                /* Invalidate the cache */
                int i;
                for (i=0; i<0x1800; i++)
                        scr->rendered[i] = ~scr->ram[i];
                scr->mode = mode;
        }
}

/******************************************************************************/
void scr_handle(Scr scr, int code)
/******************************************************************************/
{
        SCR_CTRACE_LOG("scr_handle(%d)", code);
        switch(code)
        {
        case 0: /* Render the screen */
                switch(scr->mode)
                {
                case SCR_MODE0 : /* Text Mode */
                        render_mode0(scr);
                        break;
                case SCR_MODE1 :
                case SCR_MODE2 :
                case SCR_MODE3 :
                case SCR_MODE4 :
                        render_graphics(scr);
                        break;
                default:
                        /* TODO: render graphics screen (#6) */
                        CTRACE_ERR(ctracelog, "Unmodelled screen mode %d", scr->mode);
                        assert (false);
                }
        }
}

/******************************************************************************/
bool scr_is_refresh(Scr scr)
/******************************************************************************/
{
        SCR_CTRACE_LOG("scr_is_refresh");
#if 0
        static bool result = false;
        result = !result;
        return result;
#else
        return scr->callback_count == 0;
#endif
}


#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <assert.h>
#include <termios.h>
#include "fb.h"
#include "fbset.h"

#define DEFAULT_FRAMEBUFFER     "/dev/fb0"
#define ASSERT(x) assert(x)

int OpenFrameBuffer(const char *name); // from fbset
void CloseFrameBuffer(int fh);
void GetVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void SetVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void GetFixScreenInfo(int fh, struct fb_fix_screeninfo *fix);
void ConvertFromVideoMode(const struct VideoMode *vmode,
                                 struct fb_var_screeninfo *var);
void ConvertToVideoMode(const struct fb_var_screeninfo *var,
                               struct VideoMode *vmode);
int atoboolean(const char *var);
void ReadModeDB(void);
struct VideoMode *FindVideoMode(const char *name);
void ModifyVideoMode(struct VideoMode *vmode);
void DisplayVModeInfo(struct VideoMode *vmode);
void DisplayFBInfo(struct fb_fix_screeninfo *fix);
int FillScanRates(struct VideoMode *vmode);
/*static void ConvertFromVideoMode(const struct VideoMode *vmode,
                                 struct fb_var_screeninfo *var);
static void ConvertToVideoMode(const struct fb_var_screeninfo *var,
                               struct VideoMode *vmode);
static int atoboolean(const char *var);
static void ReadModeDB(void);
static struct VideoMode *FindVideoMode(const char *name);
static void ModifyVideoMode(struct VideoMode *vmode);
static void DisplayVModeInfo(struct VideoMode *vmode);
static void DisplayFBInfo(struct fb_fix_screeninfo *fix);
static int FillScanRates(struct VideoMode *vmode);
static void Usage(void) __attribute__ ((noreturn));*/
int main(int argc, char *argv[]);

static const char *Opt_fb = NULL; // from fbset
//const char *Opt_modedb = DEFAULT_MODEDBFILE;
static const char *Opt_xres = NULL;
static const char *Opt_yres = NULL;
static const char *Opt_vxres = NULL;
static const char *Opt_vyres = NULL;
static const char *Opt_depth = NULL;
static const char *Opt_pixclock = NULL;
static const char *Opt_left = NULL;
static const char *Opt_right = NULL;
static const char *Opt_upper = NULL;
static const char *Opt_lower = NULL;
static const char *Opt_hslen = NULL;
static const char *Opt_vslen = NULL;
static const char *Opt_accel = NULL;
static const char *Opt_hsync = NULL;
static const char *Opt_vsync = NULL;
static const char *Opt_csync = NULL;
static const char *Opt_gsync = NULL;
static const char *Opt_extsync = NULL;
static const char *Opt_sync = NULL;
static const char *Opt_bcast = NULL;
static const char *Opt_laced = NULL;
static const char *Opt_double = NULL;
static const char *Opt_move = NULL;
static const char *Opt_step = NULL;
static const char *Opt_modename = NULL;
static const char *Opt_rgba = NULL;
static const char *Opt_nonstd = NULL;
static const char *Opt_grayscale = NULL;
static const char *Opt_matchyres = NULL;

static struct fb_fix_screeninfo fix_info;    // read-only video mode info, from allegro4.4
static struct fb_var_screeninfo orig_mode;   // original video mode info
//static struct fb_var_screeninfo my_mode;     // my video mode info
extern struct VideoMode Current;

/* __al_linux_leave_console:
 *   Close Linux console if no driver uses it any more.
 */
int __al_linux_leave_console(void)
{
   //ASSERT (console_users > 0);
   //console_users--;
   //if (console_users > 0) return 0;

   /* shut down the console switching system */
   //if (__al_linux_done_vtswitch()) return 1;
   //if (done_console()) return 1;
   
   return 0;
}

/* __al_linux_use_console:
 *   Init Linux console if not initialized yet.
 */
int __al_linux_use_console(void)
{
   //console_users++;
   //if (console_users > 1) return 0;

   //if (init_console()) {
   //   console_users--;
   //   return 1;
   //}

   /* Initialise the console switching system */
   //set_display_switch_mode (SWITCH_PAUSE);
   return 0;//__al_linux_init_vtswitch();
}

int __al_linux_console_fd = -1;
int __al_linux_vt = -1;

/* __al_linux_wait_for_display:
 *  Waits until we have the display.
 */
int __al_linux_wait_for_display (void)
{
   int x;
   do {
      x = ioctl (__al_linux_console_fd, VT_WAITACTIVE, __al_linux_vt);
   } while (x && errno != EINTR);
   return x;
}

static int graphics_mode = 0;

/* __al_linux_console_graphics:
 *   Puts the Linux console into graphics mode.
 */
int __al_linux_console_graphics (void)
{
   if (__al_linux_use_console()) return 1;

   if (graphics_mode) return 0;  /* shouldn't happen */

   //__al_linux_display_switch_lock(TRUE, TRUE);
   ioctl(__al_linux_console_fd, KDSETMODE, KD_GRAPHICS);
//   __al_linux_wait_for_display();
//   sleep( 1 ); // it's just allocating a vt here

   graphics_mode = 1;

   return 0;
}

/* __al_linux_display_switch_lock:
 *  System driver routine for locking the display around crucial bits of 
 *  code, for example when changing video modes.
 */
void __al_linux_display_switch_lock(int lock, int foreground)
{
        if (__al_linux_console_fd == -1) {
                return;
        }
/*
        if (foreground) {
                __al_linux_wait_for_display();
        }

        if (lock) {
                __al_linux_switching_blocked++;
        }
        else {
                __al_linux_switching_blocked--;
                poll_console_switch();
        }
*/
}

/* __al_linux_console_text:
 *  Returns the console to text mode.
 */
int __al_linux_console_text (void)
{
   int ret;

   if (!graphics_mode)
      return 0;  /* shouldn't happen */

   ioctl(__al_linux_console_fd, KDSETMODE, KD_TEXT);
/*
   do {
      ret = write(__al_linux_console_fd, "\e[H\e[J\e[0m", 10);
      if ((ret < 0) && (errno != EINTR))
         break;
   } while (0);//ret < 10);
*/
   graphics_mode = 0;
   
//   __al_linux_display_switch_lock(FALSE, FALSE);
//   __al_linux_leave_console();

   return 0;
}

void getcursorpos( int* cx, int* cy )
{
   *cx = 0, *cy = 0;
   struct termios save,raw;
   tcgetattr(0,&save);
   cfmakeraw(&raw); tcsetattr(0,TCSANOW,&raw);
   int fd = __al_linux_console_fd; // "/dev/tty"
   write( fd, "\e[6n", 4 );
   short buf = 0;
   read( 0, &buf, 1 ); // '\e'
   read( 0, &buf, 1 ); // '['
   while( buf != ';' )
   {
      *cy = *cy * 10 + ( strtol( ( char* )&buf, NULL, 10 ) );
      read( 0, &buf, 1 );
   }
   while( buf != 'R' )
   {
      *cx = *cx * 10 + ( strtol( ( char* )&buf, NULL, 10 ) );
      read( 0, &buf, 1 );
   }
   tcsetattr(0,TCSANOW,&save);
}

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <stdbool.h>

struct modeset_buf {
        uint32_t width;
        uint32_t height;
        uint32_t stride;
        uint32_t size;
        uint32_t handle;
        uint8_t *map;
        uint32_t fb;
};

struct drm_object {
        drmModeObjectProperties *props;
        drmModePropertyRes **props_info;
        uint32_t id;
};

struct modeset_output {
        struct modeset_output *next;

        unsigned int front_buf;
        struct modeset_buf bufs[2];

        struct drm_object connector;
        struct drm_object crtc;
        struct drm_object plane;

        drmModeModeInfo mode;
        uint32_t mode_blob_id;
        uint32_t crtc_index;

        bool pflip_pending;
        bool cleanup;

        uint8_t r, g, b;
        bool r_up, g_up, b_up;
};

void modeset_paint_framebuffer(struct modeset_output *out);
 
int main2( void* screen, void* buffer, int size, struct modeset_output boom )
{
   int sis = 0;
   for(; sis < 960; ++sis )
   {
      //( ( char* )screen )[ sis ] = ( ( char* )buffer )[ sis ];
      modeset_paint_framebuffer( &boom );
      sleep( 0.01 );
   }
   return 1;
}

static unsigned short *orig_cmap_data;      /* original palette data */

/* fb_do_cmap:
 *  Helper for fb_{save|restore}_cmap.
 */
static void fb_do_cmap( int ioctlno, int fbfd )
{
        struct fb_cmap cmap;
        cmap.start = 0;
        cmap.len = 256;
        cmap.red = orig_cmap_data;
        cmap.green = orig_cmap_data+256;
        cmap.blue = orig_cmap_data+512;
        cmap.transp = NULL;
        ioctl( fbfd, ioctlno, &cmap );
}

/* fb_{save|restore}_cmap:
 *  Routines to save and restore the whole palette.
 */
static void fb_save_cmap( int fbfd )
{
        if (orig_cmap_data) //_AL_FREE
           free(orig_cmap_data);   /* can't happen */
        orig_cmap_data = //_AL_MALLOC_ATOMIC
           malloc(sizeof *orig_cmap_data* 768);
        if (orig_cmap_data)
                fb_do_cmap( FBIOGETCMAP, fbfd );
}

static void fb_restore_cmap( int fbfd )
{
        if (orig_cmap_data) {
                fb_do_cmap( FBIOPUTCMAP, fbfd );
                //_AL_FREE 
                free(orig_cmap_data);
                orig_cmap_data = NULL;
        }
}

//#define EMPTY_STRING ""
//#define ALLEGRO_ERROR_SIZE 0
//char allegro_error[ALLEGRO_ERROR_SIZE] = EMPTY_STRING;
//#define get_config_text(x) (x)
//#define PREFIX_E
//#define TRACE printf
#define BYTES_PER_PIXEL(x) ( (x) / 4 )
static int fb_approx;                        /* emulate small resolution */

int modeset_drm_kms( int argc, char* argv[] );

uint8_t next_color(bool *up, uint8_t cur, unsigned int mod);

//way to assume 32-bit color!
/*void modeset_paint_framebuffer(struct modeset_output *out)
{
        struct modeset_buf *buf;
        unsigned int j, k, off;

        // draw on back framebuffer
        out->r = next_color(&out->r_up, out->r, 5);
        out->g = next_color(&out->g_up, out->g, 5);
        out->b = next_color(&out->b_up, out->b, 5);
        buf = &out->bufs[out->front_buf];// ^ 1];
        for (j = 0; j < buf->height; ++j) {
                for (k = 0; k < buf->width; ++k) {
                        off = buf->stride * j + k * 4;
                        *(uint32_t*)&buf->map[off] =
                                     (out->r << 16) | (out->g << 8) | out->b;
                }
        }

}*/

#include <time.h>

int main( int argc, char* argv[] )
{
   if( getenv( "DISPLAY" ) != NULL )
   {
      printf( "please don't try to run this program from X windows.\n" );
      return EXIT_FAILURE;
   }
   int re = 1;
   char* argv_[] = { "./main", NULL };
   if( argc == 1 )
      re = modeset_drm_kms( argc, argv );
   else if( 0 != strcmp( argv[ 1 ], "-fb" ) )
      re = modeset_drm_kms( 1, argv_ );
   if( re == 0 )
      return EXIT_SUCCESS;
   struct modeset_output enough;
   enough.next = NULL;
   enough.front_buf = 0;
   time_t start;
   srand( time( &start ) );
   enough.r = rand() % 0xff;
   enough.g = rand() % 0xff;
   enough.b = rand() % 0xff;
   enough.r_up = enough.g_up = enough.b_up = true;
   if (!Opt_fb) // from fbset
      Opt_fb = DEFAULT_FRAMEBUFFER;
   // from allegro4.4:
   __al_linux_console_fd = open( "/dev/tty", O_RDWR );
   if( __al_linux_console_fd < 0 )
   {
      perror( "opening /dev/tty" );
      return EXIT_FAILURE;
   }
   int fbfd = open( Opt_fb, O_RDWR );
   if( fbfd < 0 )
   {
      perror( "opening /dev/fb0 or DEFAULT_FRAMEBUFFER" );
      return EXIT_FAILURE;
   }
   int w = 0, h = 0, color_depth = 32;
   fb_approx = FALSE; // from allegro4.4

   struct VideoMode *vmode;
   struct fb_var_screeninfo var;
   struct fb_fix_screeninfo fix; // from fbset

   int cx = 0, cy = 0; // https://www.linuxquestions.org/questions/programming-9/get-cursor-position-in-c-947833/

   GetVarScreenInfo(fbfd, &var);
   ConvertToVideoMode(&var, &Current); // from fbset
   //DisplayVModeInfo(&Current); // fbset mode information
   w = Current.xres;
   h = Current.yres;
   color_depth = Current.depth;
   orig_mode = var;

   // get the fb_fix_screeninfo
   if( ioctl( fbfd, FBIOGET_FSCREENINFO, &fix_info ) != 0 ) // from allegro4.4
   {  close( __al_linux_console_fd );
      close( fbfd );
      printf( "Framebuffer ioctl() failed\n" );
      return EXIT_FAILURE;
   }
   void* buffer = malloc( fix_info.smem_len );
   read( fbfd, buffer, fix_info.smem_len );
   lseek( fbfd, 0, SEEK_SET );

   getcursorpos( &cx, &cy ); // linuxquestions link
   __al_linux_console_graphics(); // your cursor may stop blinking
   int stride = fix_info.line_length;
   void* fbaddr = NULL;
   // map the framebuffer, from allegro4.4
   //fbaddr = mmap( NULL, stride * 800 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0 );
   fbaddr = mmap( NULL, fix_info.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0 );
   if( fbaddr == MAP_FAILED )
   {  ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_mode);
      __al_linux_console_text();
      printf( "\e[%d;%dH", cy, cx );
      close( __al_linux_console_fd );
      close( fbfd );
      perror( "Can't map framebuffer" );
      printf( "Couldn't map framebuffer for %dx%d. Restored old "
            "resolution.\n", w, h);
      return EXIT_FAILURE;
   }

   int v_w = var.xres_virtual;
   int v_h = var.yres_virtual;
   enough.bufs[ 0 ].stride = stride;
   enough.bufs[ 0 ].map = fbaddr;
   enough.bufs[ 0 ].width = w;
   enough.bufs[ 0 ].height = h;
   enough.bufs[ 0 ].size = fix_info.smem_len;

   if (fb_approx) // from allegro
   {  v_w = w;
      v_h = h;
      //p += (my_mode.xres-w)/2 * BYTES_PER_PIXEL(color_depth) + 
      //     (my_mode.yres-h)/2 * stride;
   }
   //fb_save_cmap();    // only do this if you're testing 8bpp

   int success = main2( fbaddr, buffer, fix_info.smem_len, enough ); // returns 1;

   ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_mode);
   //fb_restore_cmap( fbfd ); // only do this if you're testing 8bpp

// safety check: (you may need sshd running if you remove this.)
   __al_linux_console_text();

   munmap( fbaddr, fix_info.smem_len );
   lseek( fbfd, 0, SEEK_SET );
   write( fbfd, buffer, fix_info.smem_len );
   printf( "\e[%d;%dH", cy, cx );
   free( buffer );
   close( __al_linux_console_fd );
   close( fbfd );
   return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

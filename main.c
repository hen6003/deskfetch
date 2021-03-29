#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/Xrandr.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int win_x = 0;
int win_y = 0;
int win_w = 800;
int win_h = 256;

/*! Destroy cairo Xlib surface and close X connection.
 *  */
void close_x11_win(cairo_surface_t *sfc)
{
   Display *dsp = cairo_xlib_surface_get_display(sfc);

   cairo_surface_destroy(sfc);
   XCloseDisplay(dsp);
}

int isUserWantsWindowToClose = 0;

int events_x11_win(cairo_surface_t *sfc)
{
   char keybuf[8];
   KeySym key;
   XEvent e;

	 Display* display = cairo_xlib_surface_get_display(sfc);

   for (;;)
   {
      if (XPending(display))
         XNextEvent(display, &e);
      else 
         return 0;

      switch (e.type)
      {
         case ButtonPress:
            return -e.xbutton.button;
         case KeyPress:
            XLookupString(&e.xkey, keybuf, sizeof(keybuf), &key, NULL);
            return key;
				case ClientMessage:
         		// check if the client message was send by window manager to indicate user wants to close the
         		if (  e.xclient.message_type  == XInternAtom( display, "WM_PROTOCOLS", 1)
         		      && e.xclient.data.l[0]  == XInternAtom( display, "WM_DELETE_WINDOW", 1)
         		      )
         		{
         		   isUserWantsWindowToClose = 1;
         		}
         		break;
         // default:
         //    fprintf(stderr, "Dropping unhandled XEevent.type = %d.\n", e.type);
      }
   }
}

cairo_surface_t *setup_x11_win()
{
   Display *display = XOpenDisplay( 0 );
   cairo_surface_t *sfc;
   
   if (display == 0)
      exit(1);

   // query Visual for "TrueColor" and 32 bits depth (RGBA)
   XVisualInfo visualinfo;
   XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &visualinfo);
   
   // create window
   Window   win;
   GC       gc;
   XSetWindowAttributes attr;
   attr.colormap   = XCreateColormap( display, DefaultRootWindow(display), visualinfo.visual, AllocNone);
   attr.event_mask = ExposureMask | KeyPressMask;
   attr.background_pixmap = None;
   attr.border_pixel      = 0;
   win = XCreateWindow(    display, DefaultRootWindow(display),
                           win_x, win_y, win_w, win_h, // x,y,width,height : are possibly opverwriteen by window manager
                           0,
                           visualinfo.depth,
                           InputOutput,
                           visualinfo.visual,
                           CWColormap|CWEventMask|CWBackPixmap|CWBorderPixel,
                           &attr
                           );
   gc = XCreateGC( display, win, 0, 0);

   // set title bar name of window
   XStoreName( display, win, "Deskfetch" );
   
   // set class of window
   XClassHint classhint = { "deskfetch", "Deskfetch" };
   XSetClassHint(display, win, &classhint);

   // Switch On >> If user pressed close key let window manager only send notification >>
   Atom wm_delete_window = XInternAtom( display, "WM_DELETE_WINDOW", 0);
   XSetWMProtocols( display, win, &wm_delete_window, 1);

   Atom atoms[2] = { XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False), None };
   XChangeProperty(display, win, XInternAtom(display, "_NET_WM_WINDOW_TYPE", False),
                           XA_ATOM, 32, PropModeReplace, (unsigned char*) atoms, 1);

   sfc = cairo_xlib_surface_create(display, win, visualinfo.visual, win_w, win_h);

   // now let the window appear to the user
   XMapWindow(display, win);

   return sfc;
}

int render_text_line;

int cairo_render_text_line(cairo_t* ctx, char* str, int spacing)
{
   cairo_move_to(ctx, 280, render_text_line);
   cairo_show_text(ctx, str);

   render_text_line += spacing;
}

char* get_image_path()
{
   char *path = (char *) malloc(30);
   char home[10];
   strcpy(home, getenv("HOME"));

   sprintf(path, "%s/.logo.png", home);

   return path;
}

char* get_distro()
{
   FILE *fp;
   char *distro;
   char ch;

   distro = (char *) malloc(20);

   fp = fopen("/etc/os-release", "r");
   
   if (fp == NULL)
   {
      fprintf(stderr, "Couldn't get distro\n");
      strcpy(distro, "Unknown");
   }
   else
   {
      strcpy(distro, "");
      
      while (fgetc(fp) != '"') ;
      while ((ch = fgetc(fp)) != '"') 
         sprintf(distro, "%s%c", distro, ch);
      
      fclose(fp);
   }

   return distro;
}

char* get_kernel()
{
   FILE *fp;
   char *kernel;
   char ch;

   kernel = (char *) malloc(20);

   fp = fopen("/proc/version", "r");
   
   if (fp == NULL)
   {
      fprintf(stderr, "ERROR: Couldn't get kernel\n");
      strcpy(kernel, "Unknown");
   }
   else
   {
      strcpy(kernel, "");
      
      while (fgetc(fp) != ' ') ;
      while (fgetc(fp) != ' ') ;
      while ((ch = fgetc(fp)) != ' ') 
         sprintf(kernel, "%s%c", kernel, ch);
      
      fclose(fp);
   }

   return kernel;
}

char* get_username()
{
   FILE *fp;
   char *username;
   char ch;

   username = (char *) malloc(50);

   username = getenv("USER");

   return username;
}

char* get_hostname()
{
   FILE *fp;
   char *hostname;
   char ch;

   hostname = (char *) malloc(50);

   fp = fopen("/etc/hostname", "r");
   
   if (fp == NULL)
   {
      fprintf(stderr, "ERROR: Couldn't get hostname\n");
      strcpy(hostname, "Unknown");
   }
   else
   {
      strcpy(hostname, "");
      
      while ((ch = fgetc(fp)) != '\n')
         sprintf(hostname, "%s%c", hostname, ch);
      
      fclose(fp);
   }

   return hostname;
}

char* get_cpu()
{
   FILE *fp;
   char *cpu;
   char ch;

   cpu = (char *) malloc(50);

   fp = fopen("/proc/cpuinfo", "r");
   
   if (fp == NULL)
   {
      fprintf(stderr, "ERROR: Couldn't get cpu\n");
      strcpy(cpu, "Unknown");
   }
   else
   {
      strcpy(cpu, "");
      
      for (int i = 0; i < 5; i++)
         while (fgetc(fp) != ':') ;

      while ((ch = fgetc(fp)) != '\n') 
         sprintf(cpu, "%s%c", cpu, ch);
      
      fclose(fp);
   }

   return cpu;
}

char* get_mem()
{
   FILE *fp;
   char *mem;
   char mem_free[20]  = "";
   char mem_total[20] = "";
   int int_mem_free;
   int int_mem_total;
   char ch;

   mem = (char *) malloc(20);

   fp = fopen("/proc/meminfo", "r");
   
   if (fp == NULL)
   {
      fprintf(stderr, "ERROR: Couldn't get memory\n");
      strcpy(mem, "Unknown");
   }
   else
   {
      strcpy(mem, "");
      
      while (fgetc(fp) != ':') ;
      while ((ch = fgetc(fp)) != 'k') 
         sprintf(mem_total, "%s%c", mem_total, ch);

      int_mem_total = atoi(mem_total);
      
      while (fgetc(fp) != ':') ;
      while ((ch = fgetc(fp)) != 'k') 
         sprintf(mem_free, "%s%c", mem_free, ch);
      
      int_mem_free = atoi(mem_free);

      sprintf(mem, "%d / %d MB", int_mem_free/1000, int_mem_total/1000);
      
      fclose(fp);
   }

   return mem;
}

char* get_uptime()
{
   FILE *fp;
   char *uptime;
   char ch;
   int int_uptime;
   int buf;

   uptime = (char *) malloc(20);

   fp = fopen("/proc/uptime", "r");
   
   if (fp == NULL)
   {
      fprintf(stderr, "ERROR: Couldn't get uptime\n");
      strcpy(uptime, "Unknown");
   }
   else
   {
      strcpy(uptime, "");

      while ((ch = fgetc(fp)) != ' ') 
         sprintf(uptime, "%s%c", uptime, ch);

      // convert to int
      int_uptime = atoi(uptime);
      
      buf = int_uptime / 3600 / 24;
      if (buf == 1)
         sprintf(uptime, "%d Day, ", buf);
      if (buf > 1)
         sprintf(uptime, "%d Days, ", buf);
      else
         strcpy(uptime, "");
      
      buf = int_uptime / 3600 % 24;
      if (buf == 1)
         sprintf(uptime, "%s%d Hour, ", uptime, buf);
      else
         sprintf(uptime, "%s%d Hours, ", uptime, buf);

      buf = int_uptime / 60 % 60;
      if (buf == 1)
         sprintf(uptime, "%s%d Min", uptime, buf);
      else
         sprintf(uptime, "%s%d Mins", uptime, buf);
      
      fclose(fp);
   }

   return uptime;
}

char* get_wm(cairo_surface_t* sfc)
{
   char *buf = (char *) malloc(20);
   Display *dsp = cairo_xlib_surface_get_display(sfc);
   Window root = DefaultRootWindow(dsp);
   XClassHint *classhint = XAllocClassHint();
   Bool xerror = False;
   Status s;
   
   if(xerror)
   {
      fprintf(stderr, "ERROR: Couldn't allocate classhint\n");
   }

   s = XGetClassHint(dsp, root, classhint);

   buf = classhint->res_name;

   if(xerror || s)
      printf("\tname: %s\n\tclass: %s\n", classhint->res_name, classhint->res_class);
   else
      printf("ERROR: Couldn't get root window's class\n");

   XFree(classhint->res_name);
   XFree(classhint->res_class);

   return buf;
}

char* get_screen_info(cairo_surface_t* sfc)
{
   char *buf = (char *) malloc(20);
   Display *dsp = cairo_xlib_surface_get_display(sfc);
   Window root = DefaultRootWindow(dsp);
	 int num_sizes;
   Rotation current_rotation;

   XRRScreenSize *xrrs = XRRSizes(dsp, 0, &num_sizes);
   XRRScreenConfiguration *conf = XRRGetScreenInfo(dsp, root);
   short refresh_rate = XRRConfigCurrentRate(conf);
   SizeID current_size_id = XRRConfigCurrentConfiguration(conf, &current_rotation);

   int width = xrrs[current_size_id].width;
   int height = xrrs[current_size_id].height;

   sprintf(buf, "%d x %d @ %dHz", width, height, refresh_rate);

   return buf;
}

int main(int argc, char* argv[])
{
   struct timespec ts = {0, 5000000};
   cairo_surface_t *sfc;
   cairo_t *ctx;
   char opt;
   char *font = (char *) malloc(20);
   int font_size = 20;
   int borders = 0;
   int transparency = 0;
   unsigned int interval = 200;

   // parse args
   while ((opt = getopt(argc, argv, "fsbBtxy")) != -1)
      switch (opt)
      {
         case 'f': 
            strcpy(font, argv[optind]);
            break;
         case 's': 
            font_size = atoi(argv[optind]);
            break;
         case 't': 
            transparency = atoi(argv[optind]);
            break;
         case 'x': 
            win_x = atoi(argv[optind]);
            break;
         case 'y': 
            win_y = atoi(argv[optind]);
            break;
         case 'b':
            borders = 1;
            break;
         case 'B':
            borders = 0;
            break;
      }
   
   // setup window	
   sfc = setup_x11_win();
   ctx = cairo_create(sfc);

   // set font
   cairo_select_font_face (ctx,
         font,
         CAIRO_FONT_SLANT_NORMAL,
         CAIRO_FONT_WEIGHT_NORMAL);

   free(font);

   // get distro image
   cairo_surface_t *image_sfc;
   cairo_pattern_t *image;
   cairo_matrix_t matrix;
   int image_w, image_h;
   char *image_path = (char *) malloc(30);
   // strcpy(image_path, "logo.png");
   strcpy(image_path, get_image_path());

   image_sfc = cairo_image_surface_create_from_png(image_path);
   image_w = cairo_image_surface_get_width(image_sfc);
   image_h = cairo_image_surface_get_height(image_sfc);
   image = cairo_pattern_create_for_surface(image_sfc);
   cairo_matrix_init_scale(&matrix, image_w/256.0, image_h/256.0);
   cairo_pattern_set_matrix(image, &matrix);

   free(image_path);

   // get distros name
   char *distro = (char *) malloc(30);
   sprintf(distro, "Distro: %s", get_distro());
   
   // get wm name
   char *wm = (char *) malloc(30);
   sprintf(wm,     "WM:     %s", get_wm(sfc));

   // get kernel
   char *kernel = (char *) malloc(30);
   sprintf(kernel, "Kernel: %s", get_kernel());

   // get resoulution
   char *screen_info = (char *) malloc(30);
   sprintf(screen_info,    "Screen: %s", get_screen_info(sfc));
   
   // get cpu
   char *cpu = (char *) malloc(60);
   sprintf(cpu,    "Cpu:   %s", get_cpu());
   
   // get hostname and username
   char *name = (char *) malloc(60);
   sprintf(name,   "%s@%s", get_username(), get_hostname());

   char *seperator = (char *) malloc(strlen(name)+1);
   strcpy(seperator, "");
   for (int i = 0; i < strlen(name); i++)
      sprintf(seperator, "%s=", seperator);

   char *uptime, *mem;
   while( !isUserWantsWindowToClose )
   {
      render_text_line = 30; // start of text rendering
      interval++;
     
      // needs to be done constantly
      if (interval > 200)
      {
         // get uptime
         uptime = (char *) malloc(30);
         sprintf(uptime, "Uptime: %s", get_uptime());
    
         // get memory
         mem = (char *) malloc(30);
         sprintf(mem,    "Mem:    %s", get_mem());

         interval = 0;
      }

      switch(events_x11_win(sfc))
      {
         case XK_Escape:
            isUserWantsWindowToClose = 1;
            break;
      }

      /* Set surface to translucent color (r, g, b, a) without disturbing graphics state. */
      cairo_save(ctx);
      cairo_set_source_rgba(ctx, 0, 0, 0, transparency / 256.0);
      cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
      cairo_paint(ctx);
      cairo_restore(ctx);
      
      // begin drawing
      cairo_push_group(ctx);

      cairo_set_source(ctx, image);
      cairo_paint(ctx);
      cairo_stroke(ctx);
      
      cairo_set_source_rgba(ctx, 1, 1, 1, 1);

      if (borders)
      {
         cairo_move_to(ctx, 0, 0);
         cairo_line_to(ctx, 0, win_h);
         cairo_line_to(ctx, win_w, win_h);
         cairo_line_to(ctx, win_w, 0);
         cairo_line_to(ctx, 0, 0);
         cairo_set_line_width(ctx, 1);
         cairo_stroke(ctx);
      }
      
      cairo_set_font_size(ctx, font_size);

      cairo_render_text_line(ctx, name, font_size);
      cairo_render_text_line(ctx, seperator, font_size);
      cairo_render_text_line(ctx, distro, font_size);
      cairo_render_text_line(ctx, kernel, font_size);
      cairo_render_text_line(ctx, uptime, font_size);
      cairo_render_text_line(ctx, cpu, font_size);
      cairo_render_text_line(ctx, mem, font_size);
      cairo_render_text_line(ctx, wm, font_size);
      cairo_render_text_line(ctx, screen_info, font_size);

      cairo_pop_group_to_source(ctx);
      cairo_paint(ctx);
      cairo_surface_flush(sfc);

      // sleep
      nanosleep(&ts, NULL);
   }

   cairo_destroy(ctx);
   cairo_surface_destroy(image_sfc);
   close_x11_win(sfc);
   free(distro);
   free(kernel);
   free(cpu);
   free(wm);
   free(screen_info);
   free(uptime);
   free(mem);
   free(seperator);

   return 0;
}

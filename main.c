#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/Xrandr.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <pango/pangocairo.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

struct color
{
   double r,g,b,a;
};

int win_x = 0;
int win_y = 0;
int win_w = 800;
int win_h = 256;
int needs_redraw = 0;
int isUserWantsWindowToClose = 0;

// Destroy cairo Xlib surface and close X connection.
void close_x11_win(cairo_surface_t *sfc)
{
   Display *dsp = cairo_xlib_surface_get_display(sfc);

   cairo_surface_destroy(sfc);
   XCloseDisplay(dsp);
}

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
         case Expose:
            needs_redraw = 1;
            return 0;
				 case ClientMessage:
         		// check if the client message was send by window manager to indicate user wants to close the
         		if (  e.xclient.message_type  == XInternAtom( display, "WM_PROTOCOLS", 1)
         		      && e.xclient.data.l[0]  == XInternAtom( display, "WM_DELETE_WINDOW", 1)
         		      )
         		   isUserWantsWindowToClose = 1;
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
   XSetWindowAttributes attr;
   attr.colormap   = XCreateColormap( display, DefaultRootWindow(display), visualinfo.visual, AllocNone);
   attr.event_mask = ExposureMask;
   attr.background_pixmap = None;
   attr.border_pixel      = 0;
   win = XCreateWindow(    display, DefaultRootWindow(display),
                           win_x, win_y, win_w, win_h, 0,
                           visualinfo.depth,
                           InputOutput,
                           visualinfo.visual,
                           CWColormap|CWEventMask|CWBackPixmap|CWBorderPixel,
                           &attr
                           );

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

int pango_render_text_line(cairo_t* ctx, PangoLayout *layout, int spacing, int offset, char* str)
{
   cairo_move_to(ctx, offset, render_text_line);
   pango_layout_set_text(layout, str, -1);
   pango_cairo_show_layout(ctx, layout);

   render_text_line += spacing;
}

int get_image_path(char* path)
{
   while (1)
   {
      if (!access(path, R_OK))
         return 0;
      else if (path[0] != 0x0)
      {
         fprintf(stderr, "Couldn't find image: '%s'\n", path);
         exit(2);
      }

      char home[10];
      strcpy(home, getenv("HOME"));
      sprintf(path, "%s/.logo.png", home);
   }
}

void get_distro(char* buf)
{
   FILE *fp;
   char *distro = buf;
   char ch;

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
}

void get_kernel(char* buf)
{
   FILE *fp;
   char *kernel = buf;
   char ch;

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
}

char* get_username(char* buf)
{
   char *username = buf;

   username = getenv("USER");

   return username;
}

char* get_hostname(char* buf)
{
   FILE *fp;
   char *hostname = buf;
   char ch;

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

void get_device(char* buf)
{
   FILE *fp;
   char *device = buf;
   char ch;

   fp = fopen("/sys/devices/virtual/dmi/id/product_version", "r");
   
   if (fp == NULL)
   {
      fprintf(stderr, "ERROR: Couldn't get device info\n");
      strcpy(device, "Unknown");
   }
   else
   {
      strcpy(device, "");
      
      while ((ch = fgetc(fp)) != '\n')
         sprintf(device, "%s%c", device, ch);
      
      fclose(fp);
   }
}

void get_cpu(char* buf)
{
   FILE *fp;
   char *cpu = buf;
   char ch;

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
}

char* get_mem(char* buf)
{
   FILE *fp;
   char *mem = buf;
   char mem_available[20]  = "";
   char mem_total[20] = "";
   int int_mem_available;
   int int_mem_total;
   char ch;

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
      while (fgetc(fp) != ':') ;
      while ((ch = fgetc(fp)) != 'k') 
         sprintf(mem_available, "%s%c", mem_available, ch);
      
      int_mem_available = atoi(mem_available);

      int_mem_available = int_mem_total - int_mem_available;

      sprintf(mem, "%d / %d MiB", int_mem_available/1024, int_mem_total/1024);
      
      fclose(fp);
   }

   return buf;
}

char* get_uptime(char* char_buf)
{
   FILE *fp;
   char *uptime = char_buf;
   char ch;
   int int_uptime;
   int buf;

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
      else if (buf > 1)
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

   return char_buf;
}

void get_wm(cairo_surface_t* sfc, char* buf)
{
   Display *dsp = cairo_xlib_surface_get_display(sfc);
   Window root = DefaultRootWindow(dsp);
   Window win;
   XClassHint *classhint = XAllocClassHint();
   Bool xerror = False;
   Status s;

   if(xerror)
   {
      fprintf(stderr, "ERROR: Couldn't allocate classhint\n");
      strcpy(buf, "Unknown");
      return;
   }

   int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;
	Atom req = XA_WINDOW;

   Atom prop = XInternAtom(dsp, "_NET_SUPPORTING_WM_CHECK", 0);

	 if (XGetWindowProperty(dsp, root, prop, 0L, sizeof atom, False, req,
	   &da, &di, &dl, &dl, &p) == Success && p) {
	   atom = *(Atom *)p;

	   XFree(p);
	} 

   win = atom;

   s = XGetClassHint(dsp, win, classhint);

   if(!xerror && !s)
   {
      printf("ERROR: Couldn't get root window's class\n");

      XTextProperty text;
      char **list = NULL;
	   int n;

      XGetTextProperty(dsp, win, &text, XInternAtom(dsp, "_NET_WM_NAME", 0));

      if (XmbTextPropertyToTextList(dsp, &text, &list, &n) >= Success && n > 0 && *list) {
			strncpy(buf, *list, strlen(buf));
			XFreeStringList(list);
		}

      if(xerror && !s)
         strcpy(buf, "Unknown");
      else
         buf[strlen(buf)] = 0x0;

      XFree(text.value);
   }
   else
      strcpy(buf, classhint->res_class);

   XFree(classhint->res_name);
   XFree(classhint->res_class);
}

void get_screen_info(cairo_surface_t* sfc, char* buf)
{
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
}

int main(int argc, char* argv[])
{
   struct timespec ts = {0, 5000000};
   cairo_surface_t *sfc;
   cairo_t *ctx;
   int font_size = 0;
   int borders = 0;
   int offset;
   int font_color_hex = 0xffffff;
   unsigned int interval = 200;

   struct color bg_color;
   struct color font_color;
   bg_color.r = bg_color.g = bg_color.b = bg_color.a = 0;
   font_color.r = font_color.g = font_color.b = font_color.a = 1;

   char opt;
   char *font       = (char *) malloc(20);
   char *image_path = (char *) malloc(30);
   char *char_buf   = (char *) malloc(60);

   strcpy(image_path, "");
   strcpy(font, "monospace 14");

   // parse args
   while ((opt = getopt(argc, argv, "fbBtxyihc")) != -1)
      switch (opt)
      {
         case 'f': 
            strcpy(font, argv[optind]);
            break;
         case 'i': 
            strcpy(image_path, argv[optind]);
            break;
         case 't': 
            bg_color.a = atoi(argv[optind]) / 100.0;
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
         case 'c':
            if (argv[optind])
            {
               if (strlen(argv[optind]) != 6)
                  fprintf(stderr, "ERROR: invalid color\n");
               else
               {
                  int hex = (int)strtol(argv[optind], NULL, 16);
                  // convert hex to 3 doubles
                  font_color.r = ((hex >> 16) & 0xFF) / 255.0;
                  font_color.g = ((hex >> 8)  & 0xFF) / 255.0;
                  font_color.b = (hex         & 0xFF) / 255.0;
               }
            }
            else 
               fprintf(stderr, "ERROR: invalid color\n");
            break;
         default:
            printf("%s [OPTION]...\n"
                   "a neofetch that shows on the desktop\n\n"
                   "  -f   font to use (monospace 14)\n"
                   "  -i   alternative image path (~/.logo.png)\n"
                   "  -t   background transparency\n"
                   "  -x/y window coordinates\n"
                   "  -b   enable borders\n"
                   "  -B   disable borders\n", argv[0]);
            free(font);
            free(char_buf);
            exit(1);
      }
   
   // setup window	
   sfc = setup_x11_win();
   ctx = cairo_create(sfc);

   // setup font
   PangoLayout *layout;
   PangoFontDescription *font_description;

   font_description = pango_font_description_from_string(font);
   
   layout = pango_cairo_create_layout(ctx);
   pango_layout_set_font_description (layout, font_description);
   pango_font_description_free(font_description);

   pango_layout_get_size(layout, NULL, &font_size);

   font_size = font_size / PANGO_SCALE;

   // get distro image
   cairo_surface_t *image_sfc;
   cairo_pattern_t *image;
   cairo_matrix_t matrix;
   int image_w, image_h;
   
   if (get_image_path(image_path))
      fprintf(stderr, "ERROR: Couldn't 'find image");

   image_sfc = cairo_image_surface_create_from_png(image_path);
   image_w = cairo_image_surface_get_width(image_sfc);
   image_h = cairo_image_surface_get_height(image_sfc);
   image = cairo_pattern_create_for_surface(image_sfc);

   cairo_matrix_init_scale(&matrix, image_h/256.0, image_h/256.0);
   offset = 256.0 / ((double) image_h / (double) image_w) + font_size;

   cairo_pattern_set_matrix(image, &matrix);

   free(image_path);

   // get distros name
   get_distro(char_buf);
   char *distro = (char *) malloc(strlen(char_buf) + 9);
   sprintf(distro, "Distro: %s", char_buf);
   
   // get wm name
   get_wm(sfc, char_buf);
   char *wm = (char *) malloc(strlen(char_buf) + 9);
   sprintf(wm,     "WM:     %s", char_buf);

   // get kernel
   get_kernel(char_buf);
   char *kernel = (char *) malloc(strlen(char_buf) + 9);
   sprintf(kernel, "Kernel: %s", char_buf);

   // get resoulution
   get_screen_info(sfc, char_buf);
   char *screen_info = (char *) malloc(strlen(char_buf) + 9);
   sprintf(screen_info,    "Screen: %s", char_buf);
   
   // get cpu
   get_cpu(char_buf);
   char *cpu = (char *) malloc(strlen(char_buf) + 9);
   sprintf(cpu,    "Cpu:   %s", char_buf);
   
   // get device
   get_device(char_buf);
   char *device = (char *) malloc(strlen(char_buf) + 9);
   sprintf(device, "Device: %s", char_buf);
   
   // get hostname and username
   char *name = (char *) malloc(60);
   sprintf(name,   "%s@", get_username(char_buf));
   strcat(name, get_hostname(char_buf));

   char *seperator = (char *) malloc(strlen(name)+1);
   strcpy(seperator, "");
   for (int i = 0; i < strlen(name); i++)
      sprintf(seperator, "%s=", seperator);

   char *uptime, *mem;
   uptime = (char *) malloc(30);
   mem = (char *) malloc(30);
   while(!isUserWantsWindowToClose)
   {
      render_text_line = 10; // start of text rendering
      interval++;
     
      // needs to be done constantly
      if (interval > 200)
      {
         // get uptime
         sprintf(uptime, "Uptime: %s", get_uptime(char_buf));
    
         // get memory
         sprintf(mem,    "Mem:    %s", get_mem(char_buf));

         interval = 0;
         needs_redraw = 1;
      }

      events_x11_win(sfc);

      if (needs_redraw)
      {
         needs_redraw = 0;

         /* Set surface to translucent color (r, g, b, a) without disturbing graphics state. */
         cairo_save(ctx);
         cairo_set_source_rgba(ctx, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
         cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
         cairo_paint(ctx);
         cairo_restore(ctx);
         
         // begin drawing
         cairo_push_group(ctx);

         cairo_set_source(ctx, image);
         cairo_paint(ctx);
         cairo_stroke(ctx);
         
         cairo_set_source_rgba(ctx, font_color.r, font_color.g, font_color.b, 1);

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

         pango_render_text_line(ctx, layout, font_size, offset, name);
         pango_render_text_line(ctx, layout, font_size, offset, seperator);
         pango_render_text_line(ctx, layout, font_size, offset, distro);
         pango_render_text_line(ctx, layout, font_size, offset, kernel);
         pango_render_text_line(ctx, layout, font_size, offset, uptime);
         pango_render_text_line(ctx, layout, font_size, offset, device);
         pango_render_text_line(ctx, layout, font_size, offset, cpu);
         pango_render_text_line(ctx, layout, font_size, offset, mem);
         pango_render_text_line(ctx, layout, font_size, offset, wm);
         pango_render_text_line(ctx, layout, font_size, offset, screen_info);

         cairo_pop_group_to_source(ctx);
         cairo_paint(ctx);
         cairo_surface_flush(sfc);
      }

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
   free(device);
   free(char_buf);

   return 0;
}

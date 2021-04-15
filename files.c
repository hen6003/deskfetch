#include "files.h"

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

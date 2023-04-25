// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Example of a clock. This is very similar to the text-example,
// except that it shows the time :)
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "graphics.h"


#include <algorithm>
#include <fstream>
#include <streambuf>

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <vector>
#include <string>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}


// modify to accomodate for line format and parse accordingly
static bool ReadLine(const char *filename, std::string *out){
  std::ifstream fs(filename, std::ifstream::in);
  std::string str((std::istreambuf_iterator<char>(fs)),
                  std::istreambuf_iterator<char>());
  std::replace(str.begin(), str.end(), '\n', ' ');
  *out =str ;
  return true ;
}

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads text from stdin and displays it. "
          "Empty string: clear screen\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "\t-d <time-format>  : Default '%%H:%%M'. See strftime()\n"
          "\t                    Can be provided multiple times for multiple "
          "lines\n"
          "\t-f <font-file>    : Use given font.\n"
          "\t-x <x-origin>     : X-Origin of displaying text (Default: 0)\n"
          "\t-y <y-origin>     : Y-Origin of displaying text (Default: 0)\n"
          "\t-s <line-spacing> : Extra spacing between lines when multiple -d given\n"
          "\t-S <spacing>      : Extra spacing between letters (Default: 0)\n"
          "\t-C <r,g,b>        : Color. Default 255,255,0\n"
          "\t-B <r,g,b>        : Background-Color. Default 0,0,0\n"
          "\t-O <r,g,b>        : Outline-Color, e.g. to increase contrast.\n"
          "\n"
          );
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

static bool parseColor(Color *c, const char *str) {
  return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

static bool FullSaturation(const Color &c) {
  return (c.r == 0 || c.r == 255)
    && (c.g == 0 || c.g == 255)
    && (c.b == 0 || c.b == 255);
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  // We accept multiple format lines

  std::vector<std::string> format_lines;
  Color color(255, 255, 0);
  Color cold(128,128,255) ; 
  Color good(128,255,128) ; 
  Color hot(255,50,128) ; 

  Color bg_color(0, 0, 0);
  Color outline_color(0,0,0);
  bool with_outline = false;

  const char *bdf_font_file = "../fonts/7x13B.bdf";
  int x_orig = 0;
  int y_orig = 0;
  int letter_spacing = 0;

  const char *input_file = NULL;
  std::string line;

  int opt;
  while ((opt = getopt(argc, argv, "x:y:f:C:B:O:s:S:d:i:")) != -1) {
    switch (opt) {
    case 'd': format_lines.push_back(optarg); break;
    case 'x': x_orig = atoi(optarg); break;
    case 'y': y_orig = atoi(optarg); break;
    case 'i': input_file = strdup(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    // case 's': line_spacing = atoi(optarg); break;
    case 'S': letter_spacing = atoi(optarg); break;
    case 'C':
      if (!parseColor(&color, optarg)) {
        fprintf(stderr, "Invalid color spec: %s\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 'B':
      if (!parseColor(&bg_color, optarg)) {
        fprintf(stderr, "Invalid background color spec: %s\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 'O':
      if (!parseColor(&outline_color, optarg)) {
        fprintf(stderr, "Invalid outline color spec: %s\n", optarg);
        return usage(argv[0]);
      }
      with_outline = true;
      break;
    default:
      return usage(argv[0]);
    }
  }

  if (input_file) {
    if (!ReadLine(input_file, &line)) {
      fprintf(stderr, "Couldn't read file '%s'\n", input_file);
      return usage(argv[0]);
    }
  }

  if (bdf_font_file == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return usage(argv[0]);
  }



  rgb_matrix::Font font;
  if (!font.LoadFont(bdf_font_file)) {
    fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
    return 1;
  }

  rgb_matrix::Font font_time;
  font_time.LoadFont("../fonts/4x6.bdf") ; 

  matrix_options.brightness       = 50 ; 
  matrix_options.cols             = 64 ; 
  matrix_options.hardware_mapping = "adafruit-hat" ; 
  
  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);


  if (matrix == NULL)
    return 1;


  // const int x = x_orig;
  int y = y_orig;

  FrameCanvas *offscreen = matrix->CreateFrameCanvas();

  char text_buffer[256];
  char date[256] ; 

  struct timespec next_time;
  struct tm tm;
  next_time.tv_sec = time(NULL);
  next_time.tv_nsec = 0;

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  float speed = 6;
  int lenght=0 ; 
  int x = 0 ; 

  while (!interrupt_received) {
    offscreen->Fill(bg_color.r, bg_color.g, bg_color.b);
    next_time.tv_sec = time(NULL);
    // next_time.tv_nsec = 0;
    localtime_r(&next_time.tv_sec, &tm);
    (ReadLine(input_file, &line)); 

    // int line_offset = 0;
    // for (const std::string &line : format_lines) {
      
      int offset_clock =  strftime(text_buffer, sizeof(text_buffer), "%H:%M", &tm);
                          strftime(date, sizeof(date), "%a %d %b %Y" , &tm);

      // upper_screen date 
      rgb_matrix::DrawText(offscreen, font_time,
                           0, y + font_time.baseline(),
                           color, NULL, date,
                           0);
      // left time 
      rgb_matrix::DrawText(offscreen, font,
                           0, y + font.baseline() + font_time.height(),
                           color, NULL, text_buffer,
                           letter_spacing);

      // current temp bottom screen 
      lenght = rgb_matrix::DrawText(offscreen, font_time,
                           x, y + font.baseline()+ font_time.height() + font.height(),
                           cold, NULL, line.c_str(),
                           letter_spacing);
    /*
    let' forget about this for now 
    must find a way to keep incrementing clock
    in the loop with precise nano 
    */
    // if (speed > 0 && --x + lenght < 0) {
    //   x = x_orig;
    // }
    
      // line_offset += font.height() + line_spacing;
    // }

    // Wait until we're ready to show it.
    // clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_time, NULL);


    // Atomic swap with double buffer
    offscreen = matrix->SwapOnVSync(offscreen);
    usleep(50000);

    // next_time.tv_sec += 1;
  }

  // Finished. Shut down the RGB matrix.
  delete matrix;

  write(STDOUT_FILENO, "\n", 1);  // Create a fresh new line after ^C on screen
  return 0;
}

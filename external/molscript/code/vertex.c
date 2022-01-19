#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#include "clib/str_utils.h"
#include "clib/dynstring.h"
#include "clib/quaternion.h"
#include "clib/extent3d.h"
#include "clib/ogl_body.h"

#include "lex.h"
#include "global.h"
#include "graphics.h"
#include "segment.h"
#include "state.h"
#include "select.h"


#define MAX_VERTEX 1000000


/*============================================================*/

/* Mimic opengl datatypes and primitives */

typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef void            GLvoid;
typedef signed char     GLbyte;   /* 1-byte signed */
typedef short           GLshort;  /* 2-byte signed */
typedef int             GLint;    /* 4-byte signed */
typedef unsigned char   GLubyte;  /* 1-byte unsigned */
typedef unsigned short  GLushort; /* 2-byte unsigned */
typedef unsigned int    GLuint;   /* 4-byte unsigned */
typedef int             GLsizei;  /* 4-byte signed */
typedef float           GLfloat;  /* single precision float */
typedef float           GLclampf; /* single precision float in [0,1] */
typedef double          GLdouble; /* double precision float */
typedef double          GLclampd; /* double precision float in [0,1] */


#define GL_POINTS         0x0000
#define GL_LINES          0x0001
#define GL_LINE_LOOP      0x0002
#define GL_LINE_STRIP     0x0003
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN   0x0006
#define GL_QUADS          0x0007
#define GL_QUAD_STRIP     0x0008
#define GL_POLYGON        0x0009



static colour current_rgb;
static GLdouble current_alpha;


struct fp
{
  short vert[3];
  short norm[3];
  unsigned char col[4];
};


struct cols
{
  unsigned char col[4];
};


struct compact_header
{
  int num_triangles;
  int num_strips;
};

#define OPTION_COLOR      1
#define OPTION_INTERLEAVE 2


static struct fp *store_   = NULL;
static struct fp *store_s_ = NULL;

static int out_mode_ = 0;

static int counter_   = 0;
static int counter_s_ = 0;

static int strip_counter_ = 0;
static int fan_counter_   = 0;

static float fan_start_[3]; // start point used to convert fan to individual triangles..
static float fan_prev_[3];  // previous point

static short fan_start_normal_[3];
static short fan_prev_normal_[3];

static short current_normal_[3];

static float current_colour_[4];
static short current_colour_s_[4];

static float res_ = 400.0; // fixme to calculate depending on heuristics for pdb values

static int printed_already_ = 0;

inline short to_short(float x)
{
  return x * (32768.0 / res_);
}


void vertex_free()
{
  free(store_);
  store_ = NULL;

  free(store_s_);
  store_s_ = NULL;
}


void vertex_clear()
{
  counter_   = 0;
  counter_s_ = 0;

  strip_counter_ = 0;

  printed_already_ = 0;
}


char* vertex_data()
{
  return (char*)&store_[0].vert[0];
}

char* vertex_strip_data()
{
  return (char*)&store_s_[0].vert[0];
}


int vertex_size()
{
  return counter_;
}

int vertex_strip_size()
{
  return counter_s_;
}


// compact the data, optionally include colours interleaved or seperate

int compact(char** cdata, int options)
{
  struct compact_header h;

  h.num_triangles = counter_;
  h.num_strips    = counter_s_;

  int total_size = sizeof(struct compact_header);

  if (options & OPTION_COLOR)
    total_size += (h.num_triangles + h.num_strips) * sizeof(struct fp);
  else
    total_size += (h.num_triangles + h.num_strips) * (sizeof(struct fp) - sizeof(struct cols));

  *cdata = malloc(total_size);

  if (*cdata == NULL)
  {
    printf("unable to allocate compact all\n");
    return 0;
  }

  char* p = *cdata;

  memcpy(p, &h, sizeof(struct compact_header));

  p += sizeof(struct compact_header);

  if ((options & OPTION_COLOR) && (options & OPTION_INTERLEAVE))
  {
    memcpy(p, store_, h.num_triangles * sizeof(struct fp));

    p +=  h.num_triangles * sizeof(struct fp);

    memcpy(p, store_s_, h.num_strips * sizeof(struct fp));

    return total_size;
  }

  // de-interleave

  struct fp* s = store_;

  for (int i = 0; i < h.num_triangles; ++i)
  {
    memcpy(p, s++, sizeof(struct fp) - sizeof(struct cols));
    p += sizeof(struct fp) - sizeof(struct cols);
  }

  s = store_s_;

  for (int i = 0; i < h.num_strips; ++i)
  {
    memcpy(p, s++, sizeof(struct fp) - sizeof(struct cols));
    p += sizeof(struct fp) - sizeof(struct cols);
  }

  if (options & OPTION_COLOR) // must also be non interleaved to reach here
  {
    s = store_;

    for (int i = 0; i < h.num_triangles; ++i)
    {
      memcpy(p, ((char*)s) + (sizeof(struct fp) - sizeof(struct cols)), sizeof(struct cols));
      p += sizeof(struct cols);
      ++s;
    }

    s = store_s_;

    for (int i = 0; i < h.num_strips; ++i)
    {
      memcpy(p, ((char*)s) + (sizeof(struct fp) - sizeof(struct cols)), sizeof(struct cols));
      p += sizeof(struct cols);
      ++s;
    }
  }

  return total_size;
}


void out_finished()
{
  // all done
}

void out_start()
{
  counter_   = 0;
  counter_s_ = 0;
}


void glNormal3d_s(short n0, short n1, short n2)
{
  current_normal_[0] = n0;
  current_normal_[1] = n1;
  current_normal_[2] = n2;
}

void glNormal3d(float n0, float n1, float n2)
{
  // normals are (should be) between -1 and 1 as direction only - could reduce these to char instead of short?

  current_normal_[0] = n0 * 32767;
  current_normal_[1] = n1 * 32767;
  current_normal_[2] = n2 * 32767;
}


void glColor4d(float n0, float n1, float n2, float n3)
{
  current_colour_[0] = n0;
  current_colour_[1] = n1;
  current_colour_[2] = n2;
  current_colour_[3] = n3;

  current_colour_s_[0] = current_colour_[0] * 255;
  current_colour_s_[1] = current_colour_[1] * 255;
  current_colour_s_[2] = current_colour_[2] * 255;
  current_colour_s_[3] = current_colour_[3] * 255;
}


void glBegin(int t)
{
  if (strip_counter_ == 1 || strip_counter_ == 2)
    printf("bad strip_counter: %d\n", strip_counter_);

  out_mode_ = t;
  strip_counter_ = 0;
  fan_counter_   = 0;
}


void glEnd()
{
  if (strip_counter_ == 1 || strip_counter_ == 2)
    printf("bad strip_counter: %d\n", strip_counter_);

  strip_counter_ = 0;
  fan_counter_   = 0;
}


void output_triangle(short n0, short n1, short n2)
{
  store_[counter_].vert[0] = n0;
  store_[counter_].vert[1] = n1;
  store_[counter_].vert[2] = n2;

  store_[counter_].norm[0] = current_normal_[0];
  store_[counter_].norm[1] = current_normal_[1];
  store_[counter_].norm[2] = current_normal_[2];

  store_[counter_].col[0] = current_colour_s_[0];
  store_[counter_].col[1] = current_colour_s_[1];
  store_[counter_].col[2] = current_colour_s_[2];
  store_[counter_].col[3] = current_colour_s_[3];

  counter_++;
}


void output_triangle_f(float n0, float n1, float n2)
{
  output_triangle(to_short(n0), to_short(n1), to_short(n2));
}


void output_strip(short n0, short n1, short n2)
{
  store_s_[counter_s_].vert[0] = n0;
  store_s_[counter_s_].vert[1] = n1;
  store_s_[counter_s_].vert[2] = n2;

  store_s_[counter_s_].norm[0] = current_normal_[0];
  store_s_[counter_s_].norm[1] = current_normal_[1];
  store_s_[counter_s_].norm[2] = current_normal_[2];

  store_s_[counter_s_].col[0] = current_colour_s_[0];
  store_s_[counter_s_].col[1] = current_colour_s_[1];
  store_s_[counter_s_].col[2] = current_colour_s_[2];
  store_s_[counter_s_].col[3] = current_colour_s_[3];

  counter_s_++;
}


void output_strip_f(float n0, float n1, float n2)
{
  output_strip(to_short(n0), to_short(n1), to_short(n2));
}


void print_out_of_space()
{
  if (printed_already_ != 0)
    return;

  printf("out of space\n");
  printed_already_ = 1;

  return;
}


void glVertex3d(float n0, float n1, float n2)
{
  if (store_ == NULL)
  {
    store_ = malloc(sizeof(struct fp) * MAX_VERTEX);

    if (store_ == NULL)
    {
      printf("unable to malloc store\n");
      vertex_free();
      return;
    }

    store_s_ = malloc(sizeof(struct fp) * MAX_VERTEX);

    if (store_s_ == NULL)
    {
      vertex_free();
      printf("unable to malloc store_s\n");
      return;
    }
  }

  switch (out_mode_)
  {
    case GL_TRIANGLES:
    {
      if (counter_ >= MAX_VERTEX)
         print_out_of_space();
      else
        output_triangle_f(n0, n1, n2);

      return;
    }

    case GL_TRIANGLE_STRIP:
    case GL_QUAD_STRIP:
    {
      if (counter_s_ + 2 >= MAX_VERTEX)
        print_out_of_space();
      else
      {
        if (strip_counter_++ == 0 && counter_s_ > 0) // we are starting a strip and there was a previous strip
        {
          // insert a degenerate triangle to restart strip (previous strip point plus this point)

          output_strip(store_s_[counter_s_-1].vert[0], store_s_[counter_s_-1].vert[1], store_s_[counter_s_-1].vert[2]);
          output_strip_f(n0, n1, n2);
        }

        output_strip_f(n0, n1, n2);
      }

      return;
    }
    
    case GL_QUADS:
    {
      if (counter_ + 6 >= MAX_VERTEX)
        print_out_of_space();
      else
      {
        if (strip_counter_ < 3)
          output_triangle_f(n0, n1, n2);
        else
        {
          output_triangle(store_[counter_-2].vert[0], store_[counter_-2].vert[1], store_[counter_-2].vert[2]);
          output_triangle(store_[counter_-2].vert[0], store_[counter_-2].vert[1], store_[counter_-2].vert[2]);
          output_triangle_f(n0, n1, n2);
        }

        if (++strip_counter_ == 4)
          strip_counter_ = 0;
      }

      return;
    }

    case GL_TRIANGLE_FAN:
    {
      if (fan_counter_ == 0)
      {
        fan_start_[0] = n0;
        fan_start_[1] = n1;
        fan_start_[2] = n2;
        fan_start_normal_[0] = current_normal_[0];
        fan_start_normal_[1] = current_normal_[1];
        fan_start_normal_[2] = current_normal_[2];

        ++fan_counter_;
        return;
      }

      if (fan_counter_ >= 2)
      {
        if (counter_ + 3 >= MAX_VERTEX)
        {
          print_out_of_space();
          return;
        }
       
        store_[counter_].vert[0] = to_short(fan_start_[0]);
        store_[counter_].vert[1] = to_short(fan_start_[1]);
        store_[counter_].vert[2] = to_short(fan_start_[2]);

        store_[counter_].norm[0] = fan_start_normal_[0];
        store_[counter_].norm[1] = fan_start_normal_[1];
        store_[counter_].norm[2] = fan_start_normal_[2];

        store_[counter_].col[0] = current_colour_s_[0];
        store_[counter_].col[1] = current_colour_s_[1];
        store_[counter_].col[2] = current_colour_s_[2];
        store_[counter_].col[3] = current_colour_s_[3];

        ++counter_;

        store_[counter_].vert[0] = to_short(fan_prev_[0]);
        store_[counter_].vert[1] = to_short(fan_prev_[1]);
        store_[counter_].vert[2] = to_short(fan_prev_[2]);

        store_[counter_].norm[0] = fan_prev_normal_[0];
        store_[counter_].norm[1] = fan_prev_normal_[1];
        store_[counter_].norm[2] = fan_prev_normal_[2];

        store_[counter_].col[0] = current_colour_s_[0];
        store_[counter_].col[1] = current_colour_s_[1];
        store_[counter_].col[2] = current_colour_s_[2];
        store_[counter_].col[3] = current_colour_s_[3];

        ++counter_;

        store_[counter_].vert[0] = to_short(n0);
        store_[counter_].vert[1] = to_short(n1);
        store_[counter_].vert[2] = to_short(n2);

        store_[counter_].norm[0] = current_normal_[0];
        store_[counter_].norm[1] = current_normal_[1];
        store_[counter_].norm[2] = current_normal_[2];

        store_[counter_].col[0] = current_colour_s_[0];
        store_[counter_].col[1] = current_colour_s_[1];
        store_[counter_].col[2] = current_colour_s_[2];
        store_[counter_].col[3] = current_colour_s_[3];

        ++counter_;
      }

      fan_prev_[0] = n0;
      fan_prev_[1] = n1;
      fan_prev_[2] = n2;
      fan_prev_normal_[0] = current_normal_[0];
      fan_prev_normal_[1] = current_normal_[1];
      fan_prev_normal_[2] = current_normal_[2];

      ++fan_counter_;
      return;
    }

    default:
      printf("Bad out_mode: %d\n", out_mode_);
  }
}

// -------


static void set_colour_property (colour *c)
{
  assert (c);

  colour_copy_to_rgb (&current_rgb, c);
  current_alpha = 1.0 - current_state->transparency;
  glColor4d (current_rgb.x, current_rgb.y, current_rgb.z, current_alpha);
}


static void set_colour_property_if_different (colour *c)
{
  colour rgb;
  GLdouble alpha = 1.0 - current_state->transparency;

  assert (c);

  colour_copy_to_rgb (&rgb, c);
  if (colour_unequal (&rgb, &current_rgb) || (current_alpha != alpha)) {
    current_rgb = rgb;
    current_alpha = alpha;
    glColor4d (current_rgb.x, current_rgb.y, current_rgb.z, current_alpha);
  }
}



void vertex_first_plot (void)
{
  while (ext3d_depth() > 1) ext3d_pop (FALSE);
  while (count_atom_selections()) pop_atom_selection();
  while (count_residue_selections()) pop_residue_selection();

  exit_on_error = (input_filename == NULL);
}


void vertex_start_plot (void)
{
  counter_   = 0;
  counter_s_ = 0;
  strip_counter_ = 0;
}


void vertex_finish_plot (void)
{
  out_finished();
}


void vertex_start_plot_general (void)
{
  colour_copy_to_rgb (&background_colour, &black_colour);

  set_area_values (0.0, 0.0,
		   (double) (output_width - 1),
		   (double) (output_height - 1));
}


void vertex_render_init (void)
{
  set_area_values (0.0, 0.0,
		   (double) (output_width - 1),
		   (double) (output_height - 1));
}


void vertex_set_background (void)
{
  colour_copy_to_rgb (&background_colour, &given_colour);
}


void vertex_coil (void)
{
  int slot;
  vector3 normal;
  coil_segment *cs;

  if (current_state->colourparts) {
    colour rgb;

    glBegin (GL_QUADS);
    colour_to_rgb (&(coil_segments[0].c));
    rgb = coil_segments[0].c;
    set_colour_property (&rgb);
    v3_difference (&normal, &(coil_segments[0].p), &(coil_segments[1].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments;
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glEnd();

    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      colour_to_rgb (&(cs->c));
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
	glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
	glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
	glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
	glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
	glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
	glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
	glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
	glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
	glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      if (colour_unequal (&rgb, &(cs->c))) {
	glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
	glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
	glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
	glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = cs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    }
    glEnd();

    glBegin (GL_QUADS);
    v3_difference (&normal, &(coil_segments[coil_segment_count-1].p),
		            &(coil_segments[coil_segment_count-2].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments + coil_segment_count - 1;
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glEnd();

  } else {
    set_colour_property (&(current_state->planecolour));

    glBegin (GL_QUADS);
    v3_difference (&normal, &(coil_segments[0].p), &(coil_segments[1].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments;
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glEnd();

    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n2.x, cs->n2.y, cs->n2.z);
      glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n3.x, cs->n3.y, cs->n3.z);
      glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    }
    glEnd();
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < coil_segment_count; slot++) {
      cs = coil_segments + slot;
      glNormal3d (cs->n4.x, cs->n4.y, cs->n4.z);
      glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
      glNormal3d (cs->n1.x, cs->n1.y, cs->n1.z);
      glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    }
    glEnd();

    glBegin (GL_QUADS);
    v3_difference (&normal, &(coil_segments[coil_segment_count-1].p),
		            &(coil_segments[coil_segment_count-2].p));
    v3_normalize (&normal);
    glNormal3d (normal.x, normal.y, normal.z);
    cs = coil_segments + coil_segment_count - 1;
    glVertex3d (cs->p1.x, cs->p1.y, cs->p1.z);
    glVertex3d (cs->p2.x, cs->p2.y, cs->p2.z);
    glVertex3d (cs->p3.x, cs->p3.y, cs->p3.z);
    glVertex3d (cs->p4.x, cs->p4.y, cs->p4.z);
    glEnd();
  }
}


void vertex_cylinder (vector3 *v1, vector3 *v2)
{
  int segments = 3 * current_state->segments + 5;

  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  set_colour_property (&(current_state->planecolour));
  //ogl_cylinder_faces (v1, v2, current_state->cylinderradius, segments, TRUE);
  ogl_cylinder_faces (v1, v2, current_state->cylinderradius, segments - 2, TRUE);
}


void vertex_helix (void)
{
  int slot;
  colour rgb;
  helix_segment *hs;

  if (current_state->colourparts) {

    glBegin (GL_TRIANGLE_STRIP);
    colour_to_rgb (&(helix_segments[0].c));
    rgb = helix_segments[0].c;
    set_colour_property (&rgb);
    for (slot = 0; slot < helix_segment_count; slot++) {
      hs = helix_segments + slot;
      colour_to_rgb (&(hs->c));
      if (colour_unequal (&rgb, &(hs->c))) {
	glNormal3d (hs->n.x, hs->n.y, hs->n.z);
	glVertex3d (hs->p1.x, hs->p1.y, hs->p1.z);
	glVertex3d (hs->p2.x, hs->p2.y, hs->p2.z);
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = hs->c;
	set_colour_property (&rgb);
      }
      glNormal3d (hs->n.x, hs->n.y, hs->n.z);
      glVertex3d (hs->p1.x, hs->p1.y, hs->p1.z);
      glVertex3d (hs->p2.x, hs->p2.y, hs->p2.z);
    }
    glEnd();

  } else {			/* not colourparts */
    GLfloat glcol[4];


    colour_copy_to_rgb (&rgb, &(current_state->planecolour));
    glcol[0] = rgb.x;
    glcol[1] = rgb.y;
    glcol[2] = rgb.z;
    glcol[3] = 1.0 - current_state->transparency;

    colour_copy_to_rgb (&rgb, &(current_state->plane2colour));
    glcol[0] = rgb.x;
    glcol[1] = rgb.y;
    glcol[2] = rgb.z;
    glcol[3] = 1.0 - current_state->transparency;

    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < helix_segment_count; slot++) {
      hs = helix_segments + slot;
      glNormal3d (hs->n.x, hs->n.y, hs->n.z);
      glVertex3d (hs->p1.x, hs->p1.y, hs->p1.z);
      glVertex3d (hs->p2.x, hs->p2.y, hs->p2.z);
    }
    glEnd();

  }

}


void
vertex_label (vector3 *p, char *label, colour *c)
{
}


void vertex_line (boolean polylines)
{
/*
  int slot;
  line_segment *ls;

  if (line_segment_count < 2) return;

  glDisable (GL_LIGHTING);
  glShadeModel (GL_FLAT);
  glLineWidth (current_state->linewidth);
  if (current_state->linedash > 0.5) {
    glLineStipple ((GLint) (current_state->linedash + 0.5), 0xAAAA);
    glEnable (GL_LINE_STIPPLE);
  }

  if (current_state->colourparts) {

    if (polylines) {

      glBegin (GL_LINE_STRIP);
      set_colour_property (&(line_segments[0].c));
      glVertex3d (line_segments->p.x, line_segments->p.y, line_segments->p.z);
      for (slot = 1; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	if (ls->new) {
	  glEnd();
	  glBegin (GL_LINE_STRIP);
	}
	set_colour_property_if_different (&(ls->c));
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();

    } else {			// not polylines

      glBegin (GL_LINES);
      set_colour_property (&(line_segments[0].c));
      for (slot = 0; slot < line_segment_count; slot += 2) {
	ls = line_segments + slot;
	set_colour_property_if_different (&(ls->c));
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
	ls++;
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();
    }

  } else {			// not colourparts

    set_colour_property (&(line_segments[0].c));

    if (polylines) {

      glBegin (GL_LINE_STRIP);
      glVertex3d (line_segments->p.x, line_segments->p.y, line_segments->p.z);
      for (slot = 1; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	if (ls->new) {
	  glEnd();
	  glBegin (GL_LINE_STRIP);
	}
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();

    } else {			// not polylines

      glBegin (GL_LINES);
      for (slot = 0; slot < line_segment_count; slot++) {
	ls = line_segments + slot;
	glVertex3d (ls->p.x, ls->p.y, ls->p.z);
      }
      glEnd();
    }
  }

  if (current_state->linedash > 0.5) glDisable (GL_LINE_STIPPLE);
  glShadeModel (GL_SMOOTH);
  glEnable (GL_LIGHTING);
  */
}


/*------------------------------------------------------------*/
void
vertex_sphere (at3d *at, double radius)
{
  assert (at);
  assert (radius > 0.0);

  set_colour_property (&(at->colour));
  //ogl_sphere_faces_globe (&(at->xyz), radius, 2 * current_state->segments);
  ogl_sphere_faces_globe (&(at->xyz), radius, current_state->segments - 2);
}


/*------------------------------------------------------------*/
void
vertex_stick (vector3 *v1, vector3 *v2, double r1, double r2, colour *c)
{
  assert (v1);
  assert (v2);
  assert (v3_distance (v1, v2) > 0.0);

  if (c) {
    set_colour_property (c);
  } else {
    set_colour_property (&(current_state->planecolour));
  }
  //ogl_cylinder_faces (v1, v2, current_state->stickradius, current_state->segments + 5, FALSE);
  ogl_cylinder_faces (v1, v2, current_state->stickradius, current_state->segments, FALSE);
}


void vertex_strand (void)
{
  int slot;
  colour rgb;
  strand_segment *ss;
  strand_segment *ssO;
  boolean thickness = current_state->strandthickness >= 0.01;

  if (!thickness) {
  }

  if (current_state->colourparts) {

    glBegin (GL_TRIANGLE_STRIP); /* strand face 1, colourparts */
    colour_to_rgb (&(strand_segments[0].c));
    rgb = strand_segments[0].c;
    set_colour_property (&rgb);
    for (slot = 0; slot < strand_segment_count - 3; slot++) {
      ss = strand_segments + slot;
      glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      colour_to_rgb (&(ss->c));
      if (colour_unequal (&rgb, &(ss->c))) {
	glEnd();
	glBegin (GL_TRIANGLE_STRIP);
	rgb = ss->c;
	set_colour_property (&rgb);
	glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
	glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      }
    }
    glEnd();

    if (thickness) {		/* strand face 2, colourparts */
      glBegin (GL_TRIANGLE_STRIP);
      rgb = strand_segments[0].c;
      set_colour_property (&rgb);
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	if (colour_unequal (&rgb, &(ss->c))) {
	  glEnd();
	  glBegin (GL_TRIANGLE_STRIP);
	  rgb = ss->c;
	  set_colour_property (&rgb);
	  glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	  glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	}
      }
      glEnd();
    }

  } else {

    glBegin (GL_TRIANGLE_STRIP); /* strand face 1, single colour */
    set_colour_property (&(current_state->planecolour));
    for (slot = 0; slot < strand_segment_count - 3; slot++) {
      ss = strand_segments + slot;
      glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    }
    glEnd();

    if (thickness) {		/* strand face 2, single colour */
      glBegin (GL_TRIANGLE_STRIP);
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
      }
      glEnd();
    }
  }

  ss = strand_segments + strand_segment_count - 3; /* arrow face 1, high */
  ssO = ss;
  glBegin (GL_TRIANGLE_STRIP);
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z); 
  ss--;
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z); 
  ss += 2;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  ss -= 2;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  ss += 2;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  ss--;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  glEnd();

  if (thickness) {
    ss = strand_segments + strand_segment_count - 3; /* arrow face 1, low */
    glBegin (GL_TRIANGLE_STRIP);
    ssO = ss;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    ss--;
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    ss += 2;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    ss -= 2;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    ss += 2;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    ss--;
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glEnd();
  }

  if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
    rgb = ss->c;
    set_colour_property (&rgb);
  }

  ss = strand_segments + strand_segment_count - 2; /* arrow face 2, high */
  glBegin (GL_TRIANGLES);
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
  ss++;
  glNormal3d (ss->n1.x, ss->n1.y, ss->n1.z);
  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
  glEnd();

  if (thickness) {
    ss = strand_segments + strand_segment_count - 2; /* arrow face 2, low */
    glBegin (GL_TRIANGLES);
    glNormal3d (ss->n3.x, ss->n3.y, ss->n3.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    ss++;
    glNormal3d (ss->n2.x, ss->n2.y, ss->n2.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glEnd();
  }

  if (thickness) {
    vector3 dir1, dir2, normal, normal2;
    strand_segment *ss2;
				/* strand base normal */
    ss = strand_segments;
    v3_difference (&dir1, &(ss->p3), &(ss->p2));
    v3_difference (&dir2, &(ss->p1), &(ss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    if (current_state->colourparts) {

      glBegin (GL_TRIANGLE_STRIP); /* strand base, colourparts */
      rgb = strand_segments[0].c;
      set_colour_property (&rgb);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
      glEnd();

      glBegin (GL_TRIANGLE_STRIP); /* strand side 1, colourparts */
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n2.x, ss->n2.y, ss->n2.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
	if (colour_unequal (&rgb, &(ss->c))) {
	  glEnd();
	  glBegin (GL_TRIANGLE_STRIP);
	  rgb = ss->c;
	  set_colour_property (&rgb);
	  glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	  glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
	}
      }
      glEnd();

      glBegin (GL_TRIANGLE_STRIP); /* strand side 2, colourparts */
      rgb = strand_segments[0].c;
      set_colour_property (&rgb);
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n4.x, ss->n4.y, ss->n4.z);
	glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	if (colour_unequal (&rgb, &(ss->c))) {
	  glEnd();
	  glBegin (GL_TRIANGLE_STRIP);
	  rgb = ss->c;
	  set_colour_property (&rgb);
	  glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
	  glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
	}
      }
      glEnd();

    } else {
      set_colour_property (&(current_state->plane2colour));

      glBegin (GL_TRIANGLE_STRIP); /* strand base, single colour */
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
      glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
      glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
      glEnd();

      glBegin (GL_TRIANGLE_STRIP); /* strand side 1, single colour */
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n2.x, ss->n2.y, ss->n2.z);
	glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
	glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
      }
      glEnd();

      glBegin (GL_TRIANGLE_STRIP); /* strand side 2, single colour */
      for (slot = 0; slot < strand_segment_count - 3; slot++) {
	ss = strand_segments + slot;
	glNormal3d (ss->n4.x, ss->n4.y, ss->n4.z);
	glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
	glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
      }
      glEnd();
    }
				/* arrow base, either colour */
    ss = strand_segments + strand_segment_count - 3;
    ss2 = strand_segments + strand_segment_count - 4;

    v3_difference (&dir1, &(ss->p3), &(ss->p1));
    v3_difference (&dir2, &(ss->p4), &(ss->p2));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    glBegin (GL_TRIANGLES);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    glVertex3d (ss2->p3.x, ss2->p3.y, ss2->p3.z);
    glVertex3d (ss2->p3.x, ss2->p3.y, ss2->p3.z);
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    glVertex3d (ss2->p4.x, ss2->p4.y, ss2->p4.z);
    glEnd();
				/* arrow first part 1, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p1), &(ss->p2));
    v3_difference (&dir2, &(ss2->p2), &(ss->p1));
    v3_cross_product (&normal2, &dir1, &dir2);
    v3_normalize (&normal2);

    ss = strand_segments + strand_segment_count - 3;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p1), &(ss->p2));
    v3_difference (&dir2, &(ss2->p2), &(ss->p1));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    glBegin (GL_TRIANGLE_STRIP);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);
    glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
    glNormal3d (normal2.x, normal2.y, normal2.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glEnd();
				/* arrow last part 1, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    glBegin (GL_TRIANGLE_STRIP);
    if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
      rgb = ss->c;
      set_colour_property (&rgb);
    }
    glNormal3d (normal2.x, normal2.y, normal2.z); /* not quite correct, */
    glVertex3d (ss->p2.x, ss->p2.y, ss->p2.z);    /* but what the hell... */
    glVertex3d (ss->p1.x, ss->p1.y, ss->p1.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glEnd();
				/* arrow first part 2, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p2), &(ss->p4));
    v3_difference (&dir2, &(ss2->p1), &(ss->p3));
    v3_cross_product (&normal2, &dir1, &dir2);
    v3_normalize (&normal2);

    ss = strand_segments + strand_segment_count - 3;
    ss2 = ss + 1;

    v3_difference (&dir1, &(ss2->p3), &(ss->p4));
    v3_difference (&dir2, &(ss2->p4), &(ss->p3));
    v3_cross_product (&normal, &dir1, &dir2);
    v3_normalize (&normal);

    glBegin (GL_TRIANGLE_STRIP);
    if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
      rgb = ss->c;
      set_colour_property (&rgb);
    }
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glNormal3d (normal2.x, normal2.y, normal2.z);
    glVertex3d (ss2->p4.x, ss2->p4.y, ss2->p4.z);
    glVertex3d (ss2->p3.x, ss2->p3.y, ss2->p3.z);
    glEnd();
				/* arrow last part 2, either colour */
    ss = strand_segments + strand_segment_count - 2;
    ss2 = ss + 1;

    glBegin (GL_TRIANGLE_STRIP);
    if (current_state->colourparts && colour_unequal (&rgb, &(ss->c))) {
      rgb = ss->c;
      set_colour_property (&rgb);
    }
    glNormal3d (normal2.x, normal2.y, normal2.z); /* not quite correct, */
    glVertex3d (ss->p4.x, ss->p4.y, ss->p4.z);    /* but what the hell... */
    glVertex3d (ss->p3.x, ss->p3.y, ss->p3.z);
    glVertex3d (ss2->p1.x, ss2->p1.y, ss2->p1.z);
    glVertex3d (ss2->p2.x, ss2->p2.y, ss2->p2.z);
    glEnd();
  }

  if (!thickness) {
  }
}


void vertex_start_object (void)
{
}


void vertex_object (int code, vector3 *triplets, int count)
{
/*
  switch (code) {

  case OBJ_POINTS:
    obj_set_state (FALSE, GL_FLAT);
    glPointSize (current_state->linewidth);
    set_colour_property (&(current_state->linecolour));
    glBegin (GL_POINTS);
    for (slot = 0; slot < count; slot++) {
      v = triplets + slot;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_POINTS_COLOURS:
    obj_set_state (FALSE, GL_FLAT);
    glPointSize (current_state->linewidth);
    col.x = -1.0;
    glBegin (GL_POINTS);
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_LINES:
    obj_set_state (FALSE, GL_FLAT);
    if (current_state->linedash > 0.5) {
    } else {
    }
    set_colour_property (&(current_state->linecolour));
    glBegin (GL_LINE_STRIP);
    for (slot = 0; slot < count; slot++) {
      v = triplets + slot;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_LINES_COLOURS:
    obj_set_state (FALSE, GL_SMOOTH);
    col.x = -1.0;
    if (current_state->linedash > 0.5) {
    } else {
    }
    glBegin (GL_LINE_STRIP);
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_TRIANGLES:
    obj_set_state (TRUE, GL_FLAT);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLES);
    for (slot = 0; slot < count; slot += 3) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v, v + 1, v + 2);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (v->x, v->y, v->z);
      v++;
      glVertex3d (v->x, v->y, v->z);
      v++;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_TRIANGLES_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLES);
    for (slot = 0; slot < count; slot += 6) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v, v + 2, v + 4);
      glNormal3d (normal.x, normal.y, normal.z);
      v++;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
      v += 3;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
      v += 3;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_TRIANGLES_NORMALS:
    obj_set_state (TRUE, GL_SMOOTH);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLES);
    for (slot = 1; slot < count; slot += 2) {
      v = triplets + slot;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_TRIANGLES_NORMALS_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLES);
    for (slot = 2; slot < count; slot += 3) {
      v = triplets + slot;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_STRIP:
    obj_set_state (TRUE, GL_FLAT);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLE_STRIP);
    v = triplets;
    v3_triangle_normal (&normal, v, v + 1, v + 2);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (v->x, v->y, v->z);
    v++;
    glVertex3d (v->x, v->y, v->z);
    v++;
    glVertex3d (v->x, v->y, v->z);
    for (slot = 3; slot < count; slot++) {
      v = triplets + slot;
      v3_triangle_normal (&normal, v - 2, v - 1, v);
      if (slot % 2) v3_reverse (&normal);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_STRIP_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLE_STRIP);
    v = triplets + 1;
    if (v3_different (&col, v)) {
      col = *v;
      glColor4d (col.x, col.y, col.z, alpha);
    }
    v--;
    v3_triangle_normal (&normal, v, v + 2, v + 4);
    glNormal3d (normal.x, normal.y, normal.z);
    glVertex3d (v->x, v->y, v->z);
    v += 3;
    if (v3_different (&col, v)) {
      col = *v;
      glColor4d (col.x, col.y, col.z, alpha);
    }
    v--;
    glVertex3d (v->x, v->y, v->z);
    v += 3;
    if (v3_different (&col, v)) {
      col = *v;
      glColor4d (col.x, col.y, col.z, alpha);
    }
    v--;
    glVertex3d (v->x, v->y, v->z);
    for (slot = 6; slot < count; slot += 2) {
      v = triplets + slot + 1;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      v3_triangle_normal (&normal, v - 4, v - 2, v);
      if (slot % 4) v3_reverse (&normal);
      glNormal3d (normal.x, normal.y, normal.z);
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_STRIP_NORMALS:
    obj_set_state (TRUE, GL_SMOOTH);
    set_colour_property (&(current_state->planecolour));
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < count; slot += 2) {
      v = triplets + slot + 1;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  case OBJ_STRIP_NORMALS_COLOURS:
    obj_set_state (TRUE, GL_SMOOTH);
    col.x = -1.0;
    glBegin (GL_TRIANGLE_STRIP);
    for (slot = 0; slot < count; slot += 3) {
      v = triplets + slot + 2;
      if (v3_different (&col, v)) {
	col = *v;
	glColor4d (col.x, col.y, col.z, alpha);
      }
      v--;
      glNormal3d (v->x, v->y, v->z);
      v--;
      glVertex3d (v->x, v->y, v->z);
    }
    glEnd();
    break;

  }
  */
}


void vertex_pickable (at3d *atom)
{
}


void vertex_set (void)
{
  output_first_plot = vertex_first_plot;
  output_finish_output = do_nothing;
  output_start_plot = vertex_start_plot;
  output_finish_plot = vertex_finish_plot;

  set_area = do_nothing;
  set_background = vertex_set_background;
  anchor_start = do_nothing_str;
  anchor_description = do_nothing_str;
  anchor_parameter = do_nothing_str;
  anchor_start_geometry = do_nothing;
  anchor_finish = do_nothing;
  lod_start = do_nothing;
  lod_finish = do_nothing;
  lod_start_group = do_nothing;
  lod_finish_group = do_nothing;
  viewpoint_start = do_nothing_str;
  viewpoint_output = do_nothing;
  output_directionallight = do_nothing;
  output_pointlight = do_nothing;
  output_spotlight = do_nothing;
  output_comment = do_nothing_str;

  output_coil = vertex_coil;
  output_cylinder = vertex_cylinder;
  output_helix = vertex_helix;
  output_label = vertex_label;
  output_line = vertex_line;
  output_sphere = vertex_sphere;
  output_stick = vertex_stick;
  output_strand = vertex_strand;

  output_start_object = vertex_start_object;
  output_object = vertex_object;
  output_finish_object = do_nothing;

  output_pickable = vertex_pickable;

  constant_colours_to_rgb();

  output_mode = OPENGL_MODE;
}

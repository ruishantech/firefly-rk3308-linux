#ifndef KMS_SINK_H
#define KMS_SINK_H

#include <sys/poll.h> 
#include <stdbool.h>

#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include "buffers.h"

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

#define VIDEO_MAX_PLANES 4
#define MEM_NUM 2

typedef struct _VideoInfo {
  //const GstVideoFormatInfo *finfo;
  unsigned int format;

  //GstVideoInterlaceMode     interlace_mode;
  //GstVideoFlags             flags;
  int                      width;
  int                      height;
  size_t                   size;
  int                      views;

  //GstVideoChromaSite        chroma_site;
  //GstVideoColorimetry       colorimetry;

  int                      par_n;
  int                      par_d;
  int                      fps_n;
  int                      fps_d;

  size_t                   offset[VIDEO_MAX_PLANES];
  int                      stride[VIDEO_MAX_PLANES];
} VideoInfo;

typedef struct _KMSMEM {
  struct bo *bo;
  int fb_id;
} KMSMEM;

typedef struct _KMSSink {
  /*< private >*/
  int fd;
  int conn_id;
  int crtc_id;
  int plane_id;
  uint32_t pipe;
  uint32_t saved_zpos;

  /* crtc data */
  uint16_t hdisplay, vdisplay;
  uint32_t buffer_id;

  /* capabilities */
  bool has_prime_import;
  bool has_prime_export;
  bool has_async_page_flip;
  bool can_scale;

  bool modesetting_enabled;
  bool restore_crtc;

  char *devname;
  char *bus_id;

  uint32_t mm_width, mm_height;
  void* saved_crtc;

  bool reconfigure;

  struct Rect render_rect;
  struct Rect pending_rect;

  struct pollfd pollfd;

  drmModeCrtcPtr original_crtc;

  VideoInfo vinfo;
  KMSMEM *mem[MEM_NUM];
  int current_num;
  KMSMEM *last_buffer;
  KMSMEM *tmp_kmsmem;
}KMSSINK;

bool kms_sink_start(KMSSINK *self, VideoInfo *vinfo);
bool kms_sink_stop(KMSSINK *self);
void kms_sink_set_render_rectangle(KMSSINK * self,
       int x, int y, int width, int height);

KMSMEM *kms_sink_get_mem(KMSSINK *self, VideoInfo *vinfo);
int kms_sink_show_frame (KMSSINK * self, KMSMEM *mem);

#endif /* KMS_SINK_H */

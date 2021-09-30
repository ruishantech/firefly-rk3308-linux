#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "kmssink.h"
#include "buffers.h"
#include <inttypes.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
static int
kms_open (char ** driver)
{
  static const char *drivers[] = { "i915", "radeon", "nouveau", "vmwgfx",
    "exynos", "amdgpu", "imx-drm", "rockchip", "atmel-hlcdc", "msm",
    "xlnx", "vc4", "meson", "sun4i-drm", "mxsfb-drm",
    "xilinx_drm",               /* DEPRECATED. Replaced by xlnx */
  };
  int i, fd = -1;

  for (i = 0; i < ARRAY_SIZE (drivers); i++) {
    fd = drmOpen (drivers[i], NULL);
    if (fd >= 0) {
      if (driver)
        *driver = strdup (drivers[i]);
      break;
    }
  }

  return fd;
}

static drmModePlane *
find_plane_for_crtc (int fd, drmModeRes * res, drmModePlaneRes * pres,
    int crtc_id)
{
  drmModePlane *plane;
  int i, pipe;

  plane = NULL;
  pipe = -1;
  for (i = 0; i < res->count_crtcs; i++) {
    if (crtc_id == res->crtcs[i]) {
      pipe = i;
      break;
    }
  }

  if (pipe == -1)
    return NULL;

  for (i = 0; i < pres->count_planes; i++) {
    plane = drmModeGetPlane (fd, pres->planes[i]);
    if (plane->possible_crtcs & (1 << pipe))
      return plane;
    drmModeFreePlane (plane);
  }

  return NULL;
}

static drmModeCrtc *
find_crtc_for_connector (int fd, drmModeRes * res, drmModeConnector * conn,
    uint32_t * pipe)
{
  int i;
  int crtc_id;
  drmModeEncoder *enc;
  drmModeCrtc *crtc;
  int crtcs_for_connector = 0;

  crtc_id = -1;
  for (i = 0; i < res->count_encoders; i++) {
    enc = drmModeGetEncoder (fd, res->encoders[i]);
    if (enc) {
      if (enc->encoder_id == conn->encoder_id) {
        crtc_id = enc->crtc_id;
        drmModeFreeEncoder (enc);
        break;
      }
      drmModeFreeEncoder (enc);
    }
  }

  /* If no active crtc was found, pick the first possible crtc */
  if (crtc_id == -1) {
    for (i = 0; i < conn->count_encoders; i++) {
      enc = drmModeGetEncoder (fd, conn->encoders[i]);
      crtcs_for_connector |= enc->possible_crtcs;
      drmModeFreeEncoder (enc);
    }

    if (crtcs_for_connector != 0)
      crtc_id = res->crtcs[ffs (crtcs_for_connector) - 1];
  }

  if (crtc_id == -1)
    return NULL;

  for (i = 0; i < res->count_crtcs; i++) {
    crtc = drmModeGetCrtc (fd, res->crtcs[i]);
    if (crtc) {
      if (crtc_id == crtc->crtc_id) {
        if (pipe)
          *pipe = i;
        return crtc;
      }
      drmModeFreeCrtc (crtc);
    }
  }

  return NULL;
}

static bool
connector_is_used (int fd, drmModeRes * res, drmModeConnector * conn)
{
  bool result;
  drmModeCrtc *crtc;

  result = false;
  crtc = find_crtc_for_connector (fd, res, conn, NULL);
  if (crtc) {
    result = crtc->buffer_id != 0;
    drmModeFreeCrtc (crtc);
  }

  return result;
}

static drmModeConnector *
find_used_connector_by_type (int fd, drmModeRes * res, int type)
{
  int i;
  drmModeConnector *conn;

  conn = NULL;
  for (i = 0; i < res->count_connectors; i++) {
    conn = drmModeGetConnector (fd, res->connectors[i]);
    if (conn) {
      if ((conn->connector_type == type) && connector_is_used (fd, res, conn))
        return conn;
      drmModeFreeConnector (conn);
    }
  }

  return NULL;
}

static drmModeConnector *
find_first_used_connector (int fd, drmModeRes * res)
{
  int i;
  drmModeConnector *conn;

  conn = NULL;
  for (i = 0; i < res->count_connectors; i++) {
    conn = drmModeGetConnector (fd, res->connectors[i]);
    if (conn) {
      if (connector_is_used (fd, res, conn))
        return conn;
      drmModeFreeConnector (conn);
    }
  }

  return NULL;
}

static drmModeConnector *
find_main_monitor (int fd, drmModeRes * res)
{
  /* Find the LVDS and eDP connectors: those are the main screens. */
  static const int priority[] = { DRM_MODE_CONNECTOR_LVDS,
    DRM_MODE_CONNECTOR_eDP
  };
  int i;
  drmModeConnector *conn;

  conn = NULL;
  for (i = 0; !conn && i < ARRAY_SIZE (priority); i++)
    conn = find_used_connector_by_type (fd, res, priority[i]);

  /* if we didn't find a connector, grab the first one in use */
  if (!conn)
    conn = find_first_used_connector (fd, res);

  /* if no connector is used, grab the first one */
  if (!conn)
    conn = drmModeGetConnector (fd, res->connectors[0]);

  return conn;
}

static bool
get_drm_caps (KMSSINK * self)
{
  int ret;
  uint64_t has_dumb_buffer;
  uint64_t has_prime;
  uint64_t has_async_page_flip;

  has_dumb_buffer = 0;
  ret = drmGetCap (self->fd, DRM_CAP_DUMB_BUFFER, &has_dumb_buffer);
  if (ret)
    printf("could not get dumb buffer capability");
  if (has_dumb_buffer == 0) {
    printf("driver cannot handle dumb buffers");
    return false;
  }

  has_prime = 0;
  ret = drmGetCap (self->fd, DRM_CAP_PRIME, &has_prime);
  if (ret)
    printf("could not get prime capability");
  else {
    self->has_prime_import = (bool) (has_prime & DRM_PRIME_CAP_IMPORT);
    self->has_prime_export = (bool) (has_prime & DRM_PRIME_CAP_EXPORT);
  }

  has_async_page_flip = 0;
  ret = drmGetCap (self->fd, DRM_CAP_ASYNC_PAGE_FLIP, &has_async_page_flip);
  if (ret)
    printf("could not get async page flip capability");
  else
    self->has_async_page_flip = (bool) has_async_page_flip;

  printf("prime import (%s) / prime export (%s) / async page flip (%s)",
      self->has_prime_import ? "✓" : "✗",
      self->has_prime_export ? "✓" : "✗",
      self->has_async_page_flip ? "✓" : "✗");

  return true;
}

static void
restore_original_crtc (KMSSINK * self)
{
  if (!self->original_crtc)
    return;

  drmModeSetCrtc(self->fd, self->original_crtc->crtc_id,
      self->original_crtc->buffer_id, self->original_crtc->x,
      self->original_crtc->y, (uint32_t *) &self->conn_id, 1,
      &self->original_crtc->mode);
  drmModeFreeCrtc(self->original_crtc);
  self->original_crtc = NULL;
}

static KMSMEM* kms_allocator_bo_alloc (KMSSINK * self, VideoInfo *vinfo) {
  uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
  KMSMEM *kmsmem;
  int ret;

  kmsmem = calloc(sizeof(KMSMEM), 1);
  kmsmem->bo = bo_create(self->fd, vinfo->format,
                   vinfo->width, vinfo->height,
                   handles, pitches,
                   offsets, UTIL_PATTERN_TILES);
  
  printf("bo handles: %d, %d, %d, %d, ptr %p\n", handles[0],
      handles[1], handles[2], handles[3], kmsmem->bo->ptr);

  ret = drmModeAddFB2 (self->fd, vinfo->width, vinfo->height,
                        vinfo->format, handles, pitches,
                        offsets, &kmsmem->fb_id, 0);
  if (ret) {
    printf("Failed to bind to framebuffer: %s (%d)",
        strerror (errno), errno);
    return NULL;
  }
  return kmsmem;
}

static bool
configure_mode_setting (KMSSINK * self, VideoInfo * vinfo)
{
  bool ret;
  drmModeConnector *conn;
  int err;
  drmModeFB *fb;
  int i;
  drmModeModeInfo *mode;
  int32_t fb_id;
  KMSMEM *kmsmem;
  VideoInfo rinfo;

  ret = false;
  conn = NULL;
  fb = NULL;
  mode = NULL;
  kmsmem = NULL;

  if (self->conn_id < 0)
    goto bail;

  printf("configuring mode setting");

  conn = drmModeGetConnector (self->fd, self->conn_id);
  if (!conn)
    goto connector_failed;

  for (i = 0; i < conn->count_modes; i++) {
    if (conn->modes[i].vdisplay == vinfo->height &&
        conn->modes[i].hdisplay == vinfo->width) {
      mode = &conn->modes[i];
      break;
    }
  }
  if (!mode)
    mode = &conn->modes[0];//just use the first mode, can improved strategy
    //goto mode_failed;
  
  kmsmem = kms_allocator_bo_alloc (self, vinfo);
  if (!kmsmem)
    goto bo_failed;
  fb_id = kmsmem->fb_id;

  if (!self->original_crtc)
    self->original_crtc = drmModeGetCrtc(self->fd, self->crtc_id);

  err = drmModeSetCrtc (self->fd, self->crtc_id, fb_id, 0, 0,
      (uint32_t *) & self->conn_id, 1, mode);
  if (err)
    goto modesetting_failed;

  self->hdisplay = mode->hdisplay;
  self->vdisplay = mode->vdisplay;

  self->tmp_kmsmem = kmsmem;

  ret = true;

bail:
  if (fb)
    drmModeFreeFB (fb);
  if (conn)
    drmModeFreeConnector (conn);

  return ret;
  /* ERRORS */
bo_failed:
  {
    printf("failed to allocate buffer object for mode setting");
    goto bail;
  }
connector_failed:
  {
    printf("Could not find a valid monitor connector");
    goto bail;
  }
framebuffer_failed:
  {
    printf("drmModeGetFB failed: %s (%d)",
        strerror (errno), errno);
    goto bail;
  }
mode_failed:
  {
    printf("cannot find appropriate mode");
    goto bail;
  }
modesetting_failed:
  {
    restore_original_crtc(self);
    printf("Failed to set mode: %s", strerror (errno));
    goto bail;
  }
}

#if 0
static bool
ensure_allowed_caps (KMSSINK * self, drmModeConnector * conn,
    drmModePlane * plane, drmModeRes * res)
{
  int i, j;
  const char *format;
  drmModeModeInfo *mode;
  int count_modes;

  if (conn && self->modesetting_enabled)
    count_modes = conn->count_modes;
  else
    count_modes = 1;

  for (i = 0; i < count_modes; i++) {
    mode = NULL;
    if (conn && self->modesetting_enabled)
      mode = &conn->modes[i];

    for (j = 0; j < plane->count_formats; j++) {
      struct util_format_info *info = util_format_info_find(plane->formats[j]);
      if (!info) {
        printf("ignoring format %d", (plane->formats[j]));
        continue;
      }
      format = info->name;
      printf("ensure_allowed_caps format %d", format);
    }

  }
  return true;
}
#endif

static void
kms_sink_configure_plane_zpos (KMSSINK * self, bool restore)
{
  drmModeObjectPropertiesPtr props = NULL;
  drmModePropertyPtr prop = NULL;
  drmModeResPtr res = NULL;
  char *buf;
  int i;
  uint64_t min, max, zpos = 0;

  if (self->plane_id <= 0)
    return;

  if (drmSetClientCap (self->fd, DRM_CLIENT_CAP_ATOMIC, 1))
    return;

  res = drmModeGetResources (self->fd);
  if (!res)
    return;

  props = drmModeObjectGetProperties (self->fd, self->plane_id,
      DRM_MODE_OBJECT_PLANE);
  if (!props)
    goto out;

  for (i = 0; i < props->count_props; i++) {
    prop = drmModeGetProperty (self->fd, props->props[i]);
    if (prop && !strcmp (prop->name, "ZPOS"))
      break;
    drmModeFreeProperty (prop);
    prop = NULL;
  }

  if (!prop)
    goto out;

  min = prop->values[0];
  max = prop->values[1];

  printf("%s min %"PRIu64", max %"PRIu64"\n", __func__, min, max);
  if (restore) {
    if (self->saved_zpos < 0)
      goto out;

    zpos = self->saved_zpos;
  } else {
    zpos = min + 1;

    buf = getenv ("KMSSINK_PLANE_ZPOS");
    if (buf)
      zpos = atoi(buf);
    else if (getenv ("KMSSINK_PLANE_ON_TOP"))
      zpos = max;
    else if (getenv ("KMSSINK_PLANE_ON_BOTTOM"))
      zpos = min;
    printf ("zpos %"PRIu64"\n", zpos);
  }

  printf("set plane zpos = %"PRIu64" (%"PRIu64"~%"PRIu64")\n", zpos, min, max);

  if (self->saved_zpos < 0)
    self->saved_zpos = props->prop_values[i];

  drmModeObjectSetProperty (self->fd, self->plane_id,
      DRM_MODE_OBJECT_PLANE, props->props[i], zpos);

out:
  drmModeFreeProperty (prop);
  drmModeFreeObjectProperties (props);
  drmModeFreeResources (res);
}

bool kms_sink_start (KMSSINK *self, VideoInfo *vinfo)
{
  drmModeRes *res;
  drmModeConnector *conn;
  drmModeCrtc *crtc;
  drmModePlaneRes *pres;
  drmModePlane *plane;
  bool universal_planes;
  bool ret;

  universal_planes = false;
  ret = false;
  res = NULL;
  conn = NULL;
  crtc = NULL;
  pres = NULL;
  plane = NULL;

  memset(self, 0, sizeof(*self));
  self->fd = -1;
  self->conn_id = -1;
  self->plane_id = -1;
  self->saved_zpos = -1;
  self->can_scale = true;
  self->vinfo = *vinfo;

  if (self->devname || self->bus_id)
    self->fd = drmOpen (self->devname, self->bus_id);
  else
    self->fd = kms_open (&self->devname);

  if (self->fd < 0)
    self->fd = open ("/dev/dri/card0", O_RDWR);

  if (self->fd < 0)
    goto open_failed;

  res = drmModeGetResources (self->fd);
  if (!res)
    goto resources_failed;

  if (self->conn_id == -1)
    conn = find_main_monitor (self->fd, res);
  else
    conn = drmModeGetConnector (self->fd, self->conn_id);
  if (!conn)
    goto connector_failed;

  crtc = find_crtc_for_connector (self->fd, res, conn, &self->pipe);
  if (!crtc)
    goto crtc_failed;

  if (!crtc->mode_valid || self->modesetting_enabled) {
    printf("enabling modesetting\n");
    self->modesetting_enabled = true;
    universal_planes = true;
  }

  if (crtc->mode_valid && self->modesetting_enabled && self->restore_crtc) {
    self->saved_crtc = (drmModeCrtc *) crtc;
  }

retry_find_plane:
  if (universal_planes &&
      drmSetClientCap (self->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1))
    goto set_cap_failed;

  pres = drmModeGetPlaneResources (self->fd);
  if (!pres)
    goto plane_resources_failed;

  if (self->plane_id == -1)
    plane = find_plane_for_crtc (self->fd, res, pres, crtc->crtc_id);
  else
    plane = drmModeGetPlane (self->fd, self->plane_id);
  if (!plane)
    goto plane_failed;

  self->conn_id = conn->connector_id;
  self->crtc_id = crtc->crtc_id;
  self->plane_id = plane->plane_id;

  printf("connector id = %d / crtc id = %d / plane id = %d\n",
      self->conn_id, self->crtc_id, self->plane_id);

  self->hdisplay = crtc->mode.hdisplay;
  self->vdisplay = crtc->mode.vdisplay;

  if (self->render_rect.w == 0 || self->render_rect.h == 0) {
    self->render_rect.x = 0;
    self->render_rect.y = 0;
    self->render_rect.w = self->hdisplay;
    self->render_rect.h = self->vdisplay;
  }

  self->pending_rect = self->render_rect;

  self->buffer_id = crtc->buffer_id;

  self->mm_width = conn->mmWidth;
  self->mm_height = conn->mmHeight;

  printf("display size: pixels = %dx%d / millimeters = %dx%d\n",
      self->hdisplay, self->vdisplay, self->mm_width, self->mm_height);

  self->pollfd.fd = self->fd;
  self->pollfd.events = POLLIN;
  ret = true;

  if (self->modesetting_enabled && !configure_mode_setting (self, vinfo))
    printf("mode seting failed\n");
bail:
  if (plane)
    drmModeFreePlane (plane);
  if (pres)
    drmModeFreePlaneResources (pres);
  if (crtc != self->saved_crtc)
    drmModeFreeCrtc (crtc);
  if (conn)
    drmModeFreeConnector (conn);
  if (res)
    drmModeFreeResources (res);

  if (!ret && self->fd >= 0) {
    drmClose (self->fd);
    self->fd = -1;
  }

  return ret;

  /* ERRORS */
open_failed:
  {
    printf("Could not open DRM module reason: %s (%d)", strerror(errno), errno);
    return false;
  }

resources_failed:
  {
    printf("drmModeGetResources failed,reason: %s (%d)", strerror (errno), errno);
    goto bail;
  }

connector_failed:
  {
    printf("Could not find a valid monitor connector");
    goto bail;
  }

crtc_failed:
  {
    printf("Could not find a crtc for connector");
    goto bail;
  }

set_cap_failed:
  {
    printf("Could not set universal planes capability bit");
    goto bail;
  }

plane_resources_failed:
  {
    printf("drmModeGetPlaneResources failed, reason: %s (%d)", strerror (errno), errno);
    goto bail;
  }

plane_failed:
  {
    if (universal_planes) {
      printf("Could not find a plane for crtc");
      goto bail;
    } else {
      universal_planes = true;
      goto retry_find_plane;
    }
  }
}

bool kms_sink_stop (KMSSINK *self)
{
  int err;

  if (self->saved_zpos >= 0) {
    kms_sink_configure_plane_zpos (self, true);
    self->saved_zpos = -1;
  }

  if (self->saved_crtc) {
    drmModeCrtc *crtc = (drmModeCrtc *) self->saved_crtc;

    err = drmModeSetCrtc (self->fd, crtc->crtc_id, crtc->buffer_id, crtc->x,
        crtc->y, (uint32_t *) & self->conn_id, 1, &crtc->mode);
    if (err)
      printf("Failed to restore previous CRTC mode: %s", strerror (errno));

    drmModeFreeCrtc (crtc);
    self->saved_crtc = NULL;
  }

  if (self->fd >= 0) {
    drmClose (self->fd);
    self->fd = -1;
  }

  self->hdisplay = 0;
  self->vdisplay = 0;
  self->pending_rect.x = 0;
  self->pending_rect.y = 0;
  self->pending_rect.w = 0;
  self->pending_rect.h = 0;
  self->render_rect = self->pending_rect;

  return true;
}

static void
sync_handler (int fd, uint32_t frame, uint32_t sec, uint32_t usec, void * data)
{
  bool *waiting;

  waiting = data;
  *waiting = false;
}

static bool
kms_sink_sync (KMSSINK * self)
{
  int ret;
  bool waiting;
  drmEventContext evctxt = {
    .version = DRM_EVENT_CONTEXT_VERSION,
    .page_flip_handler = sync_handler,
    .vblank_handler = sync_handler,
  };
  drmVBlank vbl = {
    .request = {
          .type = DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT,
          .sequence = 1,
          .signal = (unsigned long) & waiting,
        },
  };

  if (self->pipe == 1)
    vbl.request.type |= DRM_VBLANK_SECONDARY;
  else if (self->pipe > 1)
    vbl.request.type |= self->pipe << DRM_VBLANK_HIGH_CRTC_SHIFT;

  waiting = true;
  if (!self->has_async_page_flip && !self->modesetting_enabled) {
    ret = drmWaitVBlank (self->fd, &vbl);
    if (ret)
      goto vblank_failed;
  } else {
    ret = drmModePageFlip (self->fd, self->crtc_id, self->buffer_id,
        DRM_MODE_PAGE_FLIP_EVENT, &waiting);
    if (ret)
      goto pageflip_failed;
  }

  while (waiting) {
    do {
        self->pollfd.revents = 0;
        ret = poll(&self->pollfd, 1, 3 * 1000);
        if (ret < 0) {
            printf("DRM poll error, ret %d\n", ret);
        }

        if (ret == 0) {
            printf("DRM poll timeout\n");
        }

        if (self->pollfd.revents & (POLLHUP | POLLERR)) {
            printf("DRM poll hup or error, ret %d\n", ret);
        }

        if (self->pollfd.revents & POLLIN) {
            /* Page flip? If so, drmHandleEvent will unset wdata->waiting_for_flip */
            //printf("DRM page flip, ret %d\n", ret);
        } else {
            /* Timed out and page flip didn't happen */
            printf("Dropping frame while waiting_for_flip\n");
        }
    } while (ret == -1 && (errno == EAGAIN || errno == EINTR));

    ret = drmHandleEvent (self->fd, &evctxt);
    if (ret)
      goto event_failed;
  }

  return true;

  /* ERRORS */
vblank_failed:
  {
    printf("drmWaitVBlank failed: %s (%d)",
        strerror (errno), errno);
    return false;
  }
pageflip_failed:
  {
    printf("drmModePageFlip failed: %s (%d)",strerror (errno), errno);
    return false;
  }
event_failed:
  {
    printf("drmHandleEvent failed: %s (%d)",strerror (errno), errno);
    return false;
  }
}

KMSMEM *kms_sink_get_mem(KMSSINK *self, VideoInfo *vinfo) {

  if (self->mem[0] == NULL) {
    for (int i = 0; i < MEM_NUM; i++) {
      self->mem[i] = kms_allocator_bo_alloc (self, vinfo);
      if (!self->mem[i])
        goto bo_failed;
    }
    self->current_num = -1;
  }
  self->current_num = (self->current_num + 1) % MEM_NUM;
  return self->mem[self->current_num];
bo_failed:
  printf("alloc bo failed\n");
  return NULL;
}


void kms_sink_set_render_rectangle (KMSSINK * self,
    int x, int y, int width, int height)
{
  printf("Setting render rectangle to (%d,%d) %dx%d\n", x, y,
      width, height);

  if (width == -1 && height == -1) {
    x = 0;
    y = 0;
    width = self->hdisplay;
    height = self->vdisplay;
  }

  if (width <= 0 || height <= 0)
    return;

  self->pending_rect.x = x;
  self->pending_rect.y = y;
  self->pending_rect.w = width;
  self->pending_rect.h = height;

  if (self->can_scale ||
      (self->render_rect.w == width && self->render_rect.h == height)) {
    self->render_rect = self->pending_rect;
  } else {
    self->reconfigure = true;
    printf("Waiting for new caps to apply render rectangle\n");
  }
}

static void
video_sink_center_rect (struct Rect src, struct Rect dst,
    struct Rect * result, bool scaling)
{
  if (!scaling) {
    result->w = MIN (src.w, dst.w);
    result->h = MIN (src.h, dst.h);
    result->x = dst.x + (dst.w - result->w) / 2;
    result->y = dst.y + (dst.h - result->h) / 2;
  } else {
    double src_ratio, dst_ratio;

    src_ratio = (double) src.w / src.h;
    dst_ratio = (double) dst.w / dst.h;

    if (src_ratio > dst_ratio) {
      result->w = dst.w;
      result->h = dst.w / src_ratio;
      result->x = dst.x;
      result->y = dst.y + (dst.h - result->h) / 2;
    } else if (src_ratio < dst_ratio) {
      result->w = dst.h * src_ratio;
      result->h = dst.h;
      result->x = dst.x + (dst.w - result->w) / 2;
      result->y = dst.y;
    } else {
      result->x = dst.x;
      result->y = dst.y;
      result->w = dst.w;
      result->h = dst.h;
    }
  }

  //printf("source is %dx%d dest is %dx%d, result is %dx%d with x,y %dx%d\n",
      //src.w, src.h, dst.w, dst.h, result->w, result->h, result->x, result->y);
}

int kms_sink_show_frame (KMSSINK * self, KMSMEM *buffer)
{
  int ret;
  int res;
  int fb_id;
  struct Rect src = { 0, };
  struct Rect dst = { 0, };
  struct Rect result;

  res = -1;

  if (!self->can_scale) {
    printf("Applying new render rectangle");
    /* size of the rectangle does not change, only the (x,y) position changes */
    self->render_rect = self->pending_rect;
  }

  if (!self->last_buffer)
    kms_sink_configure_plane_zpos (self, false);

  if (!buffer)
    return -1;

  fb_id = buffer->fb_id;
  if (fb_id == 0)
    goto buffer_invalid;

  //printf("displaying fb %d\n", fb_id);

  if (self->modesetting_enabled) {
    self->buffer_id = fb_id;

    if (!self->render_rect.w || !self->render_rect.h)
      goto sync_frame;

    if (!self->render_rect.x && !self->render_rect.y &&
        self->render_rect.w == self->hdisplay &&
        self->render_rect.h == self->vdisplay)
      goto sync_frame;
  }

  src.w = self->hdisplay;//GST_VIDEO_SINK_WIDTH (self);
  src.h = self->vdisplay;//GST_VIDEO_SINK_HEIGHT (self);

  dst.w = self->render_rect.w;
  dst.h = self->render_rect.h;

retry_set_plane:
  video_sink_center_rect (src, dst, &result, self->can_scale);

  result.x += self->render_rect.x;
  result.y += self->render_rect.y;

  src.w = self->vinfo.width;
  src.h = self->vinfo.height;

  /* handle out of screen case */
  if ((result.x + result.w) > self->hdisplay)
    result.w = self->hdisplay - result.x;

  if ((result.y + result.h) > self->vdisplay)
    result.h = self->vdisplay - result.y;

  if (result.w <= 0 || result.h <= 0) {
    printf("video is out of display range");
    goto sync_frame;
  }

  /* to make sure it can be show when driver don't support scale */
  if (!self->can_scale) {
    src.w = result.w;
    src.h = result.h;
  }

  //printf("drmModeSetPlane at (%i,%i) %ix%i sourcing at (%i,%i) %ix%i\n",
  //    result.x, result.y, result.w, result.h, src.x, src.y, src.w, src.h);

  ret = drmModeSetPlane (self->fd, self->plane_id, self->crtc_id, fb_id, 0,
      result.x, result.y, result.w, result.h,
      /* source/cropping coordinates are given in Q16 */
      src.x << 16, src.y << 16, src.w << 16, src.h << 16);
  if (ret) {
    if (self->can_scale) {
      self->can_scale = false;
      goto retry_set_plane;
    }
    goto set_plane_failed;
  }

sync_frame:
  /* Wait for the previous frame to complete redraw */
  if (!kms_sink_sync (self)) {
    goto bail;
  }

  if (buffer != self->last_buffer)
    self->last_buffer = buffer;

  res = 0;

bail:
  return res;

  /* ERRORS */
buffer_invalid:
  {
    printf("invalid buffer: it doesn't have a fb id");
    goto bail;
  }
set_plane_failed:
  {
    printf("result = { %d, %d, %d, %d} / "
        "src = { %d, %d, %d %d } / dst = { %d, %d, %d %d }", result.x, result.y,
        result.w, result.h, src.x, src.y, src.w, src.h, dst.x, dst.y, dst.w,
        dst.h);
    printf("drmModeSetPlane failed: %s (%d)", strerror (errno), errno);
    goto bail;
  }
no_disp_ratio:
  {
    printf("Error calculating the output display ratio of the video.");
    goto bail;
  }
}

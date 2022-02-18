#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
#error "Disabling v4l2 code for linux version < 4"
#endif

int main(void) {
  struct v4l2_pix_format pf;
  pf.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
  pf.quantization = V4L2_QUANTIZATION_DEFAULT;
  pf.xfer_func = V4L2_XFER_FUNC_DEFAULT;

  open ("/", O_NONBLOCK | O_RDWR);
  ioctl (0, VIDIOC_QUERYCAP, NULL);
  ioctl (0, VIDIOC_ENUMOUTPUT, NULL);
  ioctl (0, VIDIOC_S_OUTPUT, NULL);
  ioctl (0, VIDIOC_S_FMT, NULL);
  select (0, NULL, NULL, NULL, NULL);
  write (0, NULL, 0);

  return 0;
}

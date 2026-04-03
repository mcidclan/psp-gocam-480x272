#include <pspuser.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspusb.h>
#include <pspusbacc.h>
#include <pspusbcam.h>
#include <pspjpeg.h>
#include <psppower.h>
#include <psputility_usbmodules.h>
#include <psputility_avmodules.h>

#include <string.h>

PSP_MODULE_INFO("gocam", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define UNCACHED_USER_SEG       0x40000000
#define VIDEO_MAX_BUFFER_SIZE   (4*8192)
#define MAX_OUTPUT_FRAME_SIZE   (512*272)

static u8  buffer[MAX_OUTPUT_FRAME_SIZE] __attribute__((aligned(64)));
static u32 FRAME_BUFFER[MAX_OUTPUT_FRAME_SIZE] __attribute__((aligned(64)));
static u8  work[128*1024] __attribute__((aligned(64)));

void init() {
  sceUtilityLoadUsbModule(PSP_USB_MODULE_ACC);
  sceUtilityLoadUsbModule(PSP_USB_MODULE_CAM);
  sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
  
  sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
  sceUsbStart(PSP_USBACC_DRIVERNAME, 0, 0);
  sceUsbStart(PSP_USBCAM_DRIVERNAME, 0, 0);
  sceUsbStart(PSP_USBCAMMIC_DRIVERNAME, 0, 0);
  
  sceUsbActivate(PSP_USBCAM_PID);
  
  sceJpegInitMJpeg();
  if (sceJpegCreateMJpeg(512, 272) < 0) { // making sure buffer width fits with the output stride
    sceKernelExitGame();
  }
}

void finish() {
  sceJpegDeleteMJpeg();
  sceJpegFinishMJpeg();
  sceUsbDeactivate(PSP_USBCAM_PID);
  sceUsbStop(PSP_USBCAMMIC_DRIVERNAME, 0, 0);
  sceUsbStop(PSP_USBCAM_DRIVERNAME, 0, 0);
  sceUsbStop(PSP_USBACC_DRIVERNAME, 0, 0);
  sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);
  sceUtilityUnloadUsbModule(PSP_USB_MODULE_CAM);
  sceUtilityUnloadUsbModule(PSP_USB_MODULE_ACC);
  sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
}

int getVideoFrame() {
  int retry = 10, res;
  
  while ((res = sceUsbCamReadVideoFrameBlocking(buffer, VIDEO_MAX_BUFFER_SIZE)) < 0) {
    sceKernelDelayThread(10);
    if (--retry <= 0) {
      return -1;
    }
  };
  return res;
}

int main() {
  init();

  pspDebugScreenInitEx((void*)(UNCACHED_USER_SEG | (u32)FRAME_BUFFER),
    PSP_DISPLAY_PIXEL_FORMAT_8888, 1);
  pspDebugScreenEnableBackColor(0);
  
  int res;

  PspUsbCamSetupVideoParam param;
  memset(&param, 0, sizeof(param));
  param.size            = sizeof(param);
  param.resolution      = PSP_USBCAM_RESOLUTION_480_272;
  param.framerate       = PSP_USBCAM_FRAMERATE_30_FPS;
  param.wb              = PSP_USBCAM_WB_AUTO;
  param.saturation      = 64;
  param.brightness      = 144;
  param.contrast        = 48;
  param.sharpness       = 0;
  //param.unk           = 1;
  //param.reverseflags  = PSP_USBCAM_MIRROR;
  param.framesize       = VIDEO_MAX_BUFFER_SIZE;
  param.evlevel         = PSP_USBCAM_EVLEVEL_0_0;

  SceCtrlData ctl;
  while (!(ctl.Buttons & PSP_CTRL_HOME)) {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    pspDebugScreenSetXY(0, 0);
    
    if ((sceUsbGetState() & 0xF) == PSP_USB_CONNECTION_ESTABLISHED) {
      
      pspDebugScreenPrintf("connection established\n");
      res = sceUsbCamSetupVideo(&param, work, sizeof(work));
      if (res < 0) {
        continue;
      }
      
      pspDebugScreenPrintf("usb cam video starting...\n");
      sceUsbCamAutoImageReverseSW(1);
      res = sceUsbCamStartVideo();
      if (res < 0) {
        continue;
      }
      
      do {
        sceCtrlPeekBufferPositive(&ctl, 1);

        pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf("running...\n");
       
        if ((res = getVideoFrame()) >= 0) {
          
          sceJpegDecodeMJpeg(buffer, res, (void*)(UNCACHED_USER_SEG | (u32)FRAME_BUFFER), 0);
          sceDisplayWaitVblankStart();
          scePowerTick(PSP_POWER_TICK_DISPLAY);
        } else {
          
          break;
        }
      } while (!(ctl.Buttons & PSP_CTRL_HOME));
      
      sceUsbCamStopVideo();
    }
  }

  finish();
  sceKernelExitGame();
  return 0;
}


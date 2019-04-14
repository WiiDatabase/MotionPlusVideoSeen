#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "sysconf.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

    // Initialise the video system
    VIDEO_Init();

    // This function initialises the attached controllers
    WPAD_Init();

    // Obtain the preferred video mode from the system
    // This will correspond to the settings in the Wii menu
    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate memory for the display in the uncached region
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialise the console, required for printf
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    // Set up the video registers with the chosen mode
    VIDEO_Configure(rmode);

    // Tell the video hardware where our display memory is
    VIDEO_SetNextFramebuffer(xfb);

    // Make the display visible
    VIDEO_SetBlack(FALSE);

    // Flush the video register changes to the hardware
    VIDEO_Flush();

    // Wait for Video setup to complete
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    // The console understands VT terminal escape codes
    // This positions the cursor on row 2, column 0
    // we can use variables for this with format codes too
    // e.g. printf ("\x1b[%d;%dH", row, column );
    printf("\x1b[2;0H");
    
    // Initialize SYSCONF
    printf("Init SYSCONF... ");
    s32 sysconf_inited = SYSCONF_Init();
    if (sysconf_inited == 0) {
        printf("success\n");
    } else {
        printf("failed, exiting\n");
        exit(0);
    }
    
    // Get MPLS.MOVIE setting from SYSCONF
    s32 MovieSeen = SYSCONF_GetMovieSeen();
    if (MovieSeen < 0 || MovieSeen > 1) {
        printf("Got error %d while checking MPLS.MOVIE, exiting...", MovieSeen);
        exit(0);
    }
    printf("Motion Plus Video seen: ");
    if (MovieSeen == 0) {
        printf("NO\n");
    } else {
        printf("YES\n");
    }
    printf("\nPress A to change the status and exit.\n");
    printf("Press HOME to exit without changing anything.\n");

    while(1) {

        // Call WPAD_ScanPads each loop, this reads the latest controller states
        WPAD_ScanPads();

        // WPAD_ButtonsDown tells us which buttons were pressed in this loop
        // this is a "one shot" state which will not fire again until the button has been released
        u32 pressed = WPAD_ButtonsDown(0);
        
        // A is pressed, swap state
        if (pressed & WPAD_BUTTON_A) {
            // Set MPLS.MOVIE setting
            if (MovieSeen == 0) {
                printf("\nSetting 'Motion Plus Video seen' to YES...\n");
                SYSCONF_SetMovieSeen(1);
            } else {
                printf("\nSetting 'Motion Plus Video seen' to NO...\n");
                SYSCONF_SetMovieSeen(0);
            }
            
            // Save settings
            if (SYSCONF_SaveChanges() == SYSCONF_ERR_OK) {
                printf("Settings saved, exiting...");
            } else {
                printf("Saving failed, exiting...");
            }
            
            // and exit
            sleep(3);
            exit(0);
        }

        // We return to the launcher application via exit
        if (pressed & WPAD_BUTTON_HOME) {
            printf("\nExiting without changing...");
            exit(0);
        }

        // Wait for the next frame
        VIDEO_WaitVSync();
    }

    return 0;
}

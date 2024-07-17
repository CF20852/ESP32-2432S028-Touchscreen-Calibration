/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-lvgl-ebook/
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
		
		Modified by Chip Fleming 8 July 2024 to test ESP32-2432S028 resistive touchscreen calibration                                                            */

/*  Install the "lvgl" library version 9.X by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

// The following lines were added for retrieval of resistive touchscreen to
   display coordinate transformation equation coefficients.
#include <Preferences.h>
Preferences ts_cal_coeffs;

String s;

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

// The following line was added to declare variables used to compute
// resistive touchscreen to display coordinates.
float alphaX, betaX, alphaY, betaY, deltaX, deltaY;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

static lv_obj_t * text_label_touch;
// Get the Touchscreen data
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height (RNT method)
    //x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    //y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    // The following six lines of code (ignoring whitespace and comments) were
    // added for conversion of resistive touchscreen coordinates to display coordinates
    // first, transform the x coordinate
    x = alphaX * p.x + betaX * p.y + deltaX;

    //clamp x between 0 and SCREEN_WIDTH - 1
    x = max(0, x);
    x = min(SCREEN_WIDTH - 1, x);

    // transform y and clamp it between 0 and SCREEN_HEIGHT - 1
    y = alphaY * p.x + betaY * p.y + deltaY;
    y = max(0, y);
    y = min(SCREEN_HEIGHT - 1, y); 

    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    Serial.print("X = ");
    Serial.print(x);
    Serial.print(" |");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();
    String touch_data = "X = " + String(x) + "\nY = " + String(y) + "\nZ = " + String(z);
    lv_label_set_text(text_label_touch, touch_data.c_str());
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void lv_create_main_gui(void) {
  // Create a text label aligned center: https://docs.lvgl.io/master/widgets/label.html
  text_label_touch = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_touch, "Touch screen to test");
  lv_obj_align(text_label_touch, LV_ALIGN_CENTER, 0, 0);

  // Set font type and size. More information: https://docs.lvgl.io/master/overview/font.html
  static lv_style_t style_text_label;
  lv_style_init(&style_text_label);
  lv_style_set_text_font(&style_text_label, &lv_font_montserrat_18);
  lv_obj_add_style(text_label_touch, &style_text_label, 0); 
}

// The following function was added to retrieve the resistive touchscreen to display coordinate
// transform equation coefficients from non-volatile storage.
void get_ts_cal_coefficients(void) {
  /* start the NVS storage functionality in read-only mode */
  if ( !ts_cal_coeffs.begin("ts_cal_coeff_ns", true) ) {
    Serial.println("ts_cal_coeffs initialization failed in get_ts_cal_coeffs!");
  }
  
  alphaX = ts_cal_coeffs.getFloat("alphaX");
  betaX = ts_cal_coeffs.getFloat("betaX");
  deltaX = ts_cal_coeffs.getFloat("deltaX");
	alphaY = ts_cal_coeffs.getFloat("alphaY");
  betaY = ts_cal_coeffs.getFloat("betaY");
  deltaY = ts_cal_coeffs.getFloat("deltaY");
  
  ts_cal_coeffs.end();
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);
  
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 1: touchscreen.setRotation(1);
  touchscreen.setRotation(3);

  // The following lines were added to retrieve and print coefficients used to compute
  // resistive touchscreen to display coordinates.
  // Get the resistive touchscreen to display coordinates conversion coefficients
  get_ts_cal_coefficients();
  s = String("X:  alpha = " + String(alphaX, 3) + ", beta = " + String(betaX, 3) + ", delta = " + String(deltaX, 3) );
  Serial.println(s);
  s = String("Y:  alpha = " + String(alphaY, 3) + ", beta = " + String(betaY, 3) + ", delta = " + String(deltaY, 3) );
  Serial.println(s);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Function to draw the GUI with switches
  lv_create_main_gui();
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}

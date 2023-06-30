#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // eventuell 0x3D - benutze den I2C_Scanner
delay(1000);
}
void loop() {
display.clearDisplay();
display.setTextColor(WHITE);
display.setTextSize(2);
display.setCursor(0, 13);
display.println("Hi!");
display.setTextSize(1);
display.setCursor(0, 33);
display.println("Deine Spende wird    dringend gebraucht :(");
display.display();
}